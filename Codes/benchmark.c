#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <stdarg.h>

#define SHELL_MATH_CMD_FMT "%s -b %d '%s' > /dev/null"
#define BASH_MATH_CMD_FMT  "bash -c 'for ((i=0;i<%d;i++)); do echo $((%s)); done' > /dev/null"
#define SHELL_CMD_OVERHEAD_FMT "%s -e '1+1' > /dev/null"  
#define BASH_CMD_OVERHEAD_FMT "bash -c 'true' > /dev/null" 

#define VERIFY_CMD_FMT "%s -e '%s' 2>&1"

static long diff_nsec(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) * 1000000000L + (b.tv_nsec - a.tv_nsec);
}

void trim_whitespace(char *str) {
    char *end;
    while(isspace((unsigned char)*str)) str++;
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    *(end+1) = '\0';
}

int verify_result(const char *oshell, const char *expr, const char *bash_expr) {
    char cmd[512];
    FILE *fp;
    char oshell_result[256] = {0};
    char bash_result[256] = {0};

    // Get oShell result
    snprintf(cmd, sizeof(cmd), VERIFY_CMD_FMT, oshell, expr);
    if (!(fp = popen(cmd, "r"))) return 0;
    if (!fgets(oshell_result, sizeof(oshell_result), fp)) { 
        pclose(fp);
        return 0;
    }
    pclose(fp);
    
    // Get bash result
    snprintf(cmd, sizeof(cmd), "bash -c 'echo $((%s))'", bash_expr);
    if (!(fp = popen(cmd, "r"))) return 0;
    if (!fgets(bash_result, sizeof(bash_result), fp)) {
        pclose(fp);
        return 0;
    }
    pclose(fp);

    // Trim whitespace and compare
    trim_whitespace(oshell_result);
    trim_whitespace(bash_result);
    
    double osh_val = atof(oshell_result);
    double bash_val = atof(bash_result);
    
    return fabs(osh_val - bash_val) < 1e-9;  // Account for floating point precision
}
void run_test(const char *desc, const char *cmd_fmt, int iterations, ...) {
    struct timespec t1, t2;
    char cmd[512];
    int ret;
    va_list args;
    
    clock_gettime(CLOCK_MONOTONIC, &t1);
    for (int i = 0; i < iterations; i++) {
        va_start(args, iterations);
        vsnprintf(cmd, sizeof(cmd), cmd_fmt, args);
        va_end(args);
        
        ret = system(cmd);
        (void)ret;
    }
    clock_gettime(CLOCK_MONOTONIC, &t2);
    
    printf("%-25s: %f s\n", desc, diff_nsec(t1, t2) / 1e9);
}

void convert_to_bash_expr(char *dest, const char *src, size_t max_len) {
    size_t src_len = strlen(src);
    size_t dest_idx = 0;
    
    for (size_t i = 0; i < src_len && dest_idx < max_len - 2; i++) {
        if (src[i] == '^' && dest_idx < max_len - 2) {
            dest[dest_idx++] = '*';
            if (dest_idx < max_len - 1) dest[dest_idx++] = '*';
        } else {
            dest[dest_idx++] = src[i];
        }
    }
    dest[dest_idx] = '\0';
}

int main(int argc, char **argv) {
    if (argc != 5) {
        fprintf(stderr, "Usage: %s <oshell> <expr> <math_iters> <cmd_iters>\n"
                "Example: %s ./oshell '2^20' 10000 1000\n", argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    const char *oshell = argv[1];
    const char *expr = argv[2];
    int math_iters = atoi(argv[3]);
    int cmd_iters = atoi(argv[4]);
    
    // Convert expression to bash syntax (^ → **)
    char bash_expr[256];
    convert_to_bash_expr(bash_expr, expr, sizeof(bash_expr));
    
    if (!verify_result(oshell, expr, bash_expr)) {
        fprintf(stderr, "ERROR: oShell and Bash produce different results!\n");
        fprintf(stderr, "Check conversion: '%s' vs '%s'\n", expr, bash_expr);
        return EXIT_FAILURE;
    }   
    printf("Benchmarking: %s (oShell) vs %s (Bash)\n\n", expr, bash_expr);
    
    run_test("oShell math", SHELL_MATH_CMD_FMT, math_iters, oshell, math_iters, expr);
    run_test("Bash math", BASH_MATH_CMD_FMT, math_iters, math_iters, bash_expr);
    run_test("oShell command overhead", SHELL_CMD_OVERHEAD_FMT, cmd_iters, oshell);
    run_test("Bash command overhead", BASH_CMD_OVERHEAD_FMT, cmd_iters);

    return 0;
}