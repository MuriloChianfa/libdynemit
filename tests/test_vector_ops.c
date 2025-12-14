#include <stdio.h>
#include <math.h>
#include <dynemit.h>

#define N 16

int main(void)
{
    printf("Testing vector operations:\n");
    printf("===========================\n\n");
    
    // Test data
    float a[N], b[N], result[N];
    
    // Initialize test vectors
    for (int i = 0; i < N; i++) {
        a[i] = (float)i;
        b[i] = (float)(i + 1);
    }
    
    // Test vector_add
    vector_add_f32(a, b, result, N);
    printf("vector_add_f32 test:\n");
    printf("  a[0..3] = [%.1f, %.1f, %.1f, %.1f]\n", a[0], a[1], a[2], a[3]);
    printf("  b[0..3] = [%.1f, %.1f, %.1f, %.1f]\n", b[0], b[1], b[2], b[3]);
    printf("  result  = [%.1f, %.1f, %.1f, %.1f]\n", result[0], result[1], result[2], result[3]);
    
    // Verify correctness
    int add_ok = 1;
    for (int i = 0; i < N; i++) {
        if (fabsf(result[i] - (a[i] + b[i])) > 1e-6f) {
            add_ok = 0;
            break;
        }
    }
    printf("  Status: %s\n\n", add_ok ? "OK" : "FAILED");
    
    // Test vector_sub
    vector_sub_f32(a, b, result, N);
    printf("vector_sub_f32 test:\n");
    printf("  a[0..3] = [%.1f, %.1f, %.1f, %.1f]\n", a[0], a[1], a[2], a[3]);
    printf("  b[0..3] = [%.1f, %.1f, %.1f, %.1f]\n", b[0], b[1], b[2], b[3]);
    printf("  result  = [%.1f, %.1f, %.1f, %.1f]\n", result[0], result[1], result[2], result[3]);
    
    // Verify correctness
    int sub_ok = 1;
    for (int i = 0; i < N; i++) {
        if (fabsf(result[i] - (a[i] - b[i])) > 1e-6f) {
            sub_ok = 0;
            break;
        }
    }
    printf("  Status: %s\n\n", sub_ok ? "OK" : "FAILED");
    
    // Test vector_mul
    vector_mul_f32(a, b, result, N);
    printf("vector_mul_f32 test:\n");
    printf("  a[0..3] = [%.1f, %.1f, %.1f, %.1f]\n", a[0], a[1], a[2], a[3]);
    printf("  b[0..3] = [%.1f, %.1f, %.1f, %.1f]\n", b[0], b[1], b[2], b[3]);
    printf("  result  = [%.1f, %.1f, %.1f, %.1f]\n", result[0], result[1], result[2], result[3]);
    
    // Verify correctness
    int mul_ok = 1;
    for (int i = 0; i < N; i++) {
        if (fabsf(result[i] - (a[i] * b[i])) > 1e-6f) {
            mul_ok = 0;
            break;
        }
    }
    printf("  Status: %s\n\n", mul_ok ? "OK" : "FAILED");
    
    // Overall result
    if (add_ok && sub_ok && mul_ok) {
        printf("All tests PASSED!\n");
        return 0;
    } else {
        printf("Some tests FAILED!\n");
        return 1;
    }
}
