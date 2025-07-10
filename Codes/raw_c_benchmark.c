#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#define SHELL_MATH_CMD_FMT "%s -b %d '%s' > /dev/null"
#define BASH_MATH_CMD_FMT  "bash -c 'echo $((%s))' > /dev/null"
#define ITERATIONS 1000  // Reduced from 10000
#define CMD_ITERATIONS 100
#define VERIFY_CMD_FMT "%s -e '%s' 2>&1"

static long diff_nsec(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) * 1000000000L + (b.tv_nsec - a.tv_nsec);
}
double parallel_pow(double base, int exp) {
    double result = 1;
    while (exp > 0) {
        if (exp & 1) result *= base;
        base *= base;
        exp >>= 1;
    }
    return result;
}

// Add this to compare raw algorithm speed
void test_pow_performance() {
    const int iterations = 1000000;  // 1 million
    struct timespec start, end;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    for(int i=0; i<iterations; i++) {
        parallel_pow(2, 30);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    printf("Raw C pow: %.3f ms\n", diff_nsec(start,end)/1e6);
}

int main(int argc, char **argv) {
    if (argc != 2) {  // Simplified arguments
        fprintf(stderr, "Usage: %s <oshell>\nExample: %s ./oshell\n", argv[0], argv[0]);
        return EXIT_FAILURE;
    }
    test_pow_performance();
    
    return 0;
}