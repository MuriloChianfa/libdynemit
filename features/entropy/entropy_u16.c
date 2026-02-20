/* SPDX-License-Identifier: BSL-1.0 */
#include <math.h>
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <dynemit/entropy.h>
#include <dynemit/compiler.h>

__attribute__((target("default")))
DYNEMIT_NO_AUTOVECTORIZE
static double
entropy_u16_scalar(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t *hist = calloc(65536, sizeof(uint64_t));
    if (!hist) return 0.0;
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++)
        hist[data[i]]++;
    double inv_n = 1.0 / (double)n;
    double h = 0.0;
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < 65536; i++) {
        if (hist[i] == 0) continue;
        double p = (double)hist[i] * inv_n;
        h -= p * log2(p);
    }
    free(hist);
    return h;
}

__attribute__((target("sse2")))
static double
entropy_u16_sse2(const uint16_t *data, size_t n)
{
    return entropy_u16_scalar(data, n);
}

__attribute__((target("sse4.2")))
static double
entropy_u16_sse42(const uint16_t *data, size_t n)
{
    return entropy_u16_scalar(data, n);
}

__attribute__((target("avx")))
static double
entropy_u16_avx(const uint16_t *data, size_t n)
{
    return entropy_u16_scalar(data, n);
}

__attribute__((target("avx2")))
static double
entropy_u16_avx2(const uint16_t *data, size_t n)
{
    return entropy_u16_scalar(data, n);
}

__attribute__((target("avx512f")))
static double
entropy_u16_avx512f(const uint16_t *data, size_t n)
{
    return entropy_u16_scalar(data, n);
}

entropy_u16_fn_t
entropy_u16_select(simd_level_t level)
{
    switch (level) {
    case SIMD_AVX512F: return entropy_u16_avx512f;
    case SIMD_AVX2:    return entropy_u16_avx2;
    case SIMD_AVX:     return entropy_u16_avx;
    case SIMD_SSE4_2:  return entropy_u16_sse42;
    case SIMD_SSE2:    return entropy_u16_sse2;
    case SIMD_SCALAR:
    default:           return entropy_u16_scalar;
    }
}

static entropy_u16_fn_t
entropy_u16_resolver(void)
{
    return entropy_u16_select(detect_simd_level());
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
double entropy_u16(const uint16_t *data, size_t n)
    __attribute__((ifunc("entropy_u16_resolver")));
