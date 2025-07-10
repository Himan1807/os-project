/*
Created by: 
1. Akshat Sharma
2. Himanshu Singh
3. Arnav Aanand
4. Himanshu Raj
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include <pthread.h>
#include <time.h>

#define MAX_LINE_LENGTH 1024
#define MAX_ARGS        64
#define MAX_COMMANDS    16
#define EXP_THREAD_THRESHOLD 1000000  /* ensure fast exponentiation always used */

typedef struct { double base; int exp; double result; } thread_data;

void sigint_handler(int signo) {
    printf("\nPlease use \"exit\" to terminate the shell.\n");
}

char *trim_whitespace(char *s) {
    while (isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    char *e = s + strlen(s) - 1;
    while (e > s && isspace((unsigned char)*e)) e--;
    *(e + 1) = '\0';
    return s;
}

void tokenize_command(char *command, char **args) {
    int i = 0;
    char *tok = strtok(command, " \t\r\n");
    while (tok && i < MAX_ARGS - 1) { args[i++] = tok; tok = strtok(NULL, " \t\r\n"); }
    args[i] = NULL;
}

/* Fast exponentiation (no threads) */
double parallel_pow(double base, int exp) {
    double result = 1;
    while (exp > 0) {
        if (exp & 1) result *= base;
        base *= base;
        exp >>= 1;
    }
    return result;
}

int parse_math_expression(const char *line, double *a, char *op, double *b) {
    return sscanf(line, "%lf %c %lf", a, op, b) == 3;
}

int eval_expr(const char *expr, double *out) {
    double x, y; char op;
    if (!parse_math_expression(expr, &x, &op, &y)) return 0;
    switch (op) {
        case '+': *out = x + y; break;
        case '-': *out = x - y; break;
        case '*': *out = x * y; break;
        case '/': if (y == 0) { fprintf(stderr, "Error: div by zero\n"); return -1; } *out = x / y; break;
        case '^': *out = parallel_pow(x, (int)y); break;
        default: fprintf(stderr, "Unsupported operator: %c\n", op); return -1;
    }
    return 1;
}

int main(int argc, char **argv) {
    /* Batch mode: evaluate expr N times in one process */
    if (argc == 4 && strcmp(argv[1], "-b") == 0) {
        int reps = atoi(argv[2]); double val;
        for (int i = 0; i < reps; i++) {
            if (eval_expr(argv[3], &val) <= 0) return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    /* Single-eval mode */
    if (argc == 3 && strcmp(argv[1], "-e") == 0) {
        double val;
        if (eval_expr(argv[2], &val) == 1) {
            printf("%lf\n", val);
            return EXIT_SUCCESS;
        }
        return EXIT_FAILURE;
    }

    /* Interactive shell */
    signal(SIGINT, sigint_handler);
    char line[MAX_LINE_LENGTH];
    while (1) {
        printf("OShell> "); fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) { printf("\n"); break; }
        line[strcspn(line, "\n")] = '\0';
        if (strcmp(line, "exit") == 0) { printf("Exiting shell...\n"); break; }
        double r;
        if (eval_expr(line, &r) == 1) {
            printf("Result: %lf\n", r);
            continue;
        }
        /* 2. Command Parsing: Split the input line into command segments by '|' */
        char *commands[MAX_COMMANDS];
        int num_commands = 0;
        char *command_token = strtok(line, "|");
        while(command_token != NULL && num_commands < MAX_COMMANDS) {
            commands[num_commands++] = trim_whitespace(command_token);
            command_token = strtok(NULL, "|");
        }
        
        /* 3. Handling Background Processes:
         * Check if the last command ends with '&' and remove it.
         */
        int background = 0;
        char *last_cmd = commands[num_commands - 1];
        char temp_cmd[MAX_LINE_LENGTH];
        strncpy(temp_cmd, last_cmd, MAX_LINE_LENGTH);
        temp_cmd[MAX_LINE_LENGTH - 1] = '\0';
        char *args_temp[MAX_ARGS];
        tokenize_command(temp_cmd, args_temp);
        int args_count = 0;
        while(args_temp[args_count] != NULL)
            args_count++;
        if(args_count > 0 && strcmp(args_temp[args_count - 1], "&") == 0) {
            background = 1;
            /* Remove '&' from the last command string */
            char *ampersand = strstr(last_cmd, "&");
            if(ampersand != NULL) {
                *ampersand = '\0';
                last_cmd = trim_whitespace(last_cmd);
                commands[num_commands - 1] = last_cmd;
            }
        }
        
        /* 4. Setup pipes if more than one command is present */
        int num_pipes = num_commands - 1;
        int pipefds[2 * num_pipes];  /* each pipe has 2 file descriptors */
        for(int i = 0; i < num_pipes; i++) {
            if(pipe(pipefds + i*2) < 0) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }
        }
        
        /* 5. Command Execution and Process Management */
        pid_t pid;
        int cmd;
        for(cmd = 0; cmd < num_commands; cmd++) {
            /* Tokenize the current command into arguments */
            char *args[MAX_ARGS];
            tokenize_command(commands[cmd], args);
            
            /* Handle Input/Output Redirection:
             * Scan tokens for '<' or '>' and record the file names.
             */
            int in_redirect = -1, out_redirect = -1;
            char *input_file = NULL;
            char *output_file = NULL;
            for(int j = 0; args[j] != NULL; j++) {
                if(strcmp(args[j], "<") == 0) {
                    in_redirect = j;
                    if(args[j+1] != NULL) {
                        input_file = args[j+1];
                    } else {
                        fprintf(stderr, "Error: no input file specified\n");
                    }
                }
                if(strcmp(args[j], ">") == 0) {
                    out_redirect = j;
                    if(args[j+1] != NULL) {
                        output_file = args[j+1];
                    } else {
                        fprintf(stderr, "Error: no output file specified\n");
                    }
                }
            }
            /* Remove redirection tokens from the argument list */
            if(in_redirect != -1)
                args[in_redirect] = NULL;
            if(out_redirect != -1)
                args[out_redirect] = NULL;
            
            pid = fork();
            if(pid == 0) {
                /* CHILD PROCESS */
                
                /* a. Setup Pipe Redirection if necessary */
                /* If not the first command, redirect standard input from the previous pipe */
                if(cmd > 0) {
                    if(dup2(pipefds[(cmd - 1)*2], 0) < 0) {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }
                /* If not the last command, redirect standard output to the next pipe */
                if(cmd < num_commands - 1) {
                    if(dup2(pipefds[cmd*2 + 1], 1) < 0) {
                        perror("dup2");
                        exit(EXIT_FAILURE);
                    }
                }
                /* Close all pipe file descriptors in the child */
                for(int i = 0; i < 2 * num_pipes; i++) {
                    close(pipefds[i]);
                }
                
                /* b. Input Redirection */
                if(input_file != NULL && in_redirect != -1) {
                    int fd_in = open(input_file, O_RDONLY);
                    if(fd_in < 0) {
                        perror("open input file");
                        exit(EXIT_FAILURE);
                    }
                    if(dup2(fd_in, 0) < 0) {
                        perror("dup2 input");
                        exit(EXIT_FAILURE);
                    }
                    close(fd_in);
                }
                /* c. Output Redirection */
                if(output_file != NULL && out_redirect != -1) {
                    int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if(fd_out < 0) {
                        perror("open output file");
                        exit(EXIT_FAILURE);
                    }
                    if(dup2(fd_out, 1) < 0) {
                        perror("dup2 output");
                        exit(EXIT_FAILURE);
                    }
                    close(fd_out);
                }
                
                /* d. Execute the command */
                if(execvp(args[0], args) < 0) {
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            } else if(pid < 0) {
                perror("fork");
            }
            /* Parent process continues to fork remaining commands */
        }  /* End of for each command */
        
        /* 6. Parent Process Cleanup: Close all pipe file descriptors */
        for(int i = 0; i < 2 * num_pipes; i++) {
            close(pipefds[i]);
        }
        
        /* Process Management: Wait for all child processes unless running in background */
        if(!background) {
            for(int i = 0; i < num_commands; i++) {
                wait(NULL);
            }
        } else {
            printf("Process running in background.\n");
        }
    }  /* End of while loop */
    
    return 0;
}