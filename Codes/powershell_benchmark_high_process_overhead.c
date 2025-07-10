#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <windows.h>

#define POWERSHELL_CMD_FMT "powershell -Command \"%s\" > NUL"
#define ITERATIONS 1000

static long long get_current_time_ns() {
    LARGE_INTEGER frequency, time;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&time);
    return (long long)((time.QuadPart * 1000000000LL) / frequency.QuadPart);
}

void convert_to_powershell_expr(char *dest, const char *src, size_t max_len) {
    size_t src_len = strlen(src);
    size_t dest_idx = 0;
    
    // Convert ^ to PowerShell's exponent syntax [math]::pow()
    for (size_t i = 0; i < src_len && dest_idx < max_len - 20; i++) {
        if (src[i] == '^') {
            dest_idx += snprintf(dest + dest_idx, max_len - dest_idx, 
                               "[math]::pow(%.*s, ", (int)i, src);
            dest_idx += snprintf(dest + dest_idx, max_len - dest_idx, 
                               "%s)", src + i + 1);
            break;
        }
    }
    if (dest_idx == 0) {  // No exponent found
        strncpy(dest, src, max_len);
        dest[max_len-1] = '\0';
    }
}

void run_benchmark(const char *expr, int iterations) {
    char cmd[512];
    char ps_expr[256];
    long long t1, t2;

    convert_to_powershell_expr(ps_expr, expr, sizeof(ps_expr));

    // Warm up
    system("powershell -Command \"[math]::pow(1,1)\" > NUL");

    // Timed run
    t1 = get_current_time_ns();
    for (int i = 0; i < iterations; i++) {
        snprintf(cmd, sizeof(cmd), POWERSHELL_CMD_FMT, ps_expr);
        system(cmd);
    }
    t2 = get_current_time_ns();

    printf("PowerShell execution time for '%s' (%d iterations): %.3f ms\n",
           expr, iterations, (t2 - t1) / 1e6);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <expression> <iterations>\n"
                "Example: %s \"2^30\" 1000\n", 
                argv[0], argv[0]);
        return EXIT_FAILURE;
    }

    const char *expr = argv[1];
    int iterations = atoi(argv[2]);

    run_benchmark(expr, iterations);
    return 0;
}