// benchmark_vector_mul.c
// build:
//   gcc -O3 vector_mul.c dynemit.c benchmark_vector_mul.c -o benchmark_vector_mul
// run:
//   ./benchmark_vector_mul
//
// This will:
// 1. detect CPU SIMD level
// 2. say "expected impl: AVX2" (for example)
// 3. run a small benchmark

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <dynemit/core.h>
#include <dynemit/vector_mul.h>

/* ---------- timing helper ---------- */
static double
now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

int
main(void)
{
    const size_t n = 1u << 20;       // 1,048,576 floats
    const size_t bytes = n * sizeof(float);
    const int iters = 2000;           // number of repeats in benchmark

    float *a   = aligned_alloc(64, bytes);
    float *b   = aligned_alloc(64, bytes);
    float *out = aligned_alloc(64, bytes);

    if (!a || !b || !out) {
        fprintf(stderr, "alloc failed\n");
        return 1;
    }

    for (size_t i = 0; i < n; i++) {
        a[i] = (float)i * 0.5f;
        b[i] = (float)i * 0.25f + 1.0f;
    }

    simd_level_t lvl = detect_simd_level();
    printf("Detected SIMD level: %s\n", simd_level_name(lvl));
    printf("(this is the version the ifunc dispatcher will pick)\n");

    // warmup
    for (int w = 0; w < 10; w++)
        vector_mul_f32(a, b, out, n);

    double t0 = now_sec();
    for (int i = 0; i < iters; i++) {
        vector_mul_f32(a, b, out, n);
    }
    double t1 = now_sec();

    double elapsed = t1 - t0;
    double bytes_processed = (double)iters * (double)bytes * 3.0; // a+b+out
    double gbps = bytes_processed / elapsed / 1e9;

    // each element = 1 mul
    double ops = (double)iters * (double)n;
    double gflops = ops / elapsed / 1e9;

    printf("n = %zu, iters = %d\n", n, iters);
    printf("elapsed = %.6f s\n", elapsed);
    printf("throughput ~= %.2f GB/s (counting a+b+out)\n", gbps);
    printf("GFLOP/s   ~= %.2f\n", gflops);

    // tiny correctness check
    int bad = 0;
    for (size_t i = 0; i < 16; i++) {
        float expect = a[i] * b[i];
        if (out[i] != expect) {
            printf("mismatch at %zu: got %f, expect %f\n", i, out[i], expect);
            bad = 1;
        }
    }
    if (!bad)
        printf("correctness check: OK (first 16 elements)\n");

    free(a);
    free(b);
    free(out);

    return 0;
}


