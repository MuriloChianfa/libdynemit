/* SPDX-License-Identifier: BSL-1.0 */
#include <math.h>
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dynemit/compiler.h>
#include <dynemit/entropy.h>

static int
cmp_u32(const void *a, const void *b)
{
    uint32_t va = *(const uint32_t *)a;
    uint32_t vb = *(const uint32_t *)b;
    return (va > vb) - (va < vb);
}

__attribute__((target("default")))
DYNEMIT_NO_AUTOVECTORIZE
static double
entropy_u32_scalar(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint32_t *sorted = malloc(n * sizeof(uint32_t));
    if (!sorted) return 0.0;
    memcpy(sorted, data, n * sizeof(uint32_t));
    qsort(sorted, n, sizeof(uint32_t), cmp_u32);

    size_t cap = 256;
    uint64_t *cnts = malloc(cap * sizeof(uint64_t));
    if (!cnts) { free(sorted); return 0.0; }
    size_t num_cnts = 0;
    uint64_t run = 1;
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 1; i < n; i++) {
        if (sorted[i] == sorted[i - 1]) {
            run++;
        } else {
            if (num_cnts >= cap) {
                cap *= 2;
                uint64_t *tmp = realloc(cnts, cap * sizeof(uint64_t));
                if (!tmp) { free(cnts); free(sorted); return 0.0; }
                cnts = tmp;
            }
            cnts[num_cnts++] = run;
            run = 1;
        }
    }
    if (num_cnts >= cap) {
        cap *= 2;
        uint64_t *tmp = realloc(cnts, cap * sizeof(uint64_t));
        if (!tmp) { free(cnts); free(sorted); return 0.0; }
        cnts = tmp;
    }
    cnts[num_cnts++] = run;

    double h = entropy_histogram(cnts, num_cnts);
    free(cnts);
    free(sorted);
    return h;
}

__attribute__((target("sse2")))
static double
entropy_u32_sse2(const uint32_t *data, size_t n)
{
    return entropy_u32_scalar(data, n);
}

__attribute__((target("sse4.2")))
static double
entropy_u32_sse42(const uint32_t *data, size_t n)
{
    return entropy_u32_scalar(data, n);
}

__attribute__((target("avx")))
static double
entropy_u32_avx(const uint32_t *data, size_t n)
{
    return entropy_u32_scalar(data, n);
}

__attribute__((target("avx2")))
static double
entropy_u32_avx2(const uint32_t *data, size_t n)
{
    return entropy_u32_scalar(data, n);
}

__attribute__((target("avx512f")))
static double
entropy_u32_avx512f(const uint32_t *data, size_t n)
{
    return entropy_u32_scalar(data, n);
}

entropy_u32_fn_t
entropy_u32_select(simd_level_t level)
{
    switch (level) {
    case SIMD_AVX512F: return entropy_u32_avx512f;
    case SIMD_AVX2:    return entropy_u32_avx2;
    case SIMD_AVX:     return entropy_u32_avx;
    case SIMD_SSE4_2:  return entropy_u32_sse42;
    case SIMD_SSE2:    return entropy_u32_sse2;
    case SIMD_SCALAR:
    default:           return entropy_u32_scalar;
    }
}

static entropy_u32_fn_t
entropy_u32_resolver(void)
{
    return entropy_u32_select(detect_simd_level());
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
double entropy_u32(const uint32_t *data, size_t n)
    __attribute__((ifunc("entropy_u32_resolver")));
