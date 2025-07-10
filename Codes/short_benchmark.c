#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define SHELL_MATH_CMD_FMT "%s -b %d '%s' > /dev/null"
#define BASH_MATH_CMD_FMT  "bash -c 'echo $((%s))' > /dev/null"
#define ITERATIONS 1000  // Reduced from 10000
#define CMD_ITERATIONS 1
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

void run_math_test(const char *oshell, const char *expr, const char *bash_expr) {
    struct timespec t1, t2;
    char cmd[512];
    
    // Test oShell
    clock_gettime(CLOCK_MONOTONIC, &t1);
    snprintf(cmd, sizeof(cmd), SHELL_MATH_CMD_FMT, oshell, ITERATIONS, expr);
    system(cmd);
    clock_gettime(CLOCK_MONOTONIC, &t2);
    printf("OShell math: %.3f ms\n", diff_nsec(t1, t2) / 1e6);

    // Test Bash
    clock_gettime(CLOCK_MONOTONIC, &t1);
    for(int i=0; i<ITERATIONS; i++) {  // Batch in C code
        snprintf(cmd, sizeof(cmd), BASH_MATH_CMD_FMT, bash_expr);
        system(cmd);
    }
    clock_gettime(CLOCK_MONOTONIC, &t2);
    printf("Bash math:  %.3f ms\n", diff_nsec(t1, t2) / 1e6);
}

int main(int argc, char **argv) {
    if (argc != 2) {  // Simplified arguments
        fprintf(stderr, "Usage: %s <oshell>\nExample: %s ./oshell\n", argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    const char *expr = "2^30";  // Larger exponent, fewer iterations needed
    char bash_expr[256];
    convert_to_bash_expr(bash_expr, expr, sizeof(bash_expr));

    if (!verify_result(argv[1], expr, bash_expr)) {
        fprintf(stderr, "Result mismatch!\n");
        return EXIT_FAILURE;
    }

    printf("Benchmarking %s vs Bash...\n", expr);
    run_math_test(argv[1], expr, bash_expr);
    
    return 0;
}