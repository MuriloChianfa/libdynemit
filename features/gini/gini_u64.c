/* SPDX-License-Identifier: BSL-1.0 */
#include <immintrin.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dynemit/gini.h>
#include <dynemit/compiler.h>

/*
 * Gini coefficient for pre-sorted ascending u64 data.
 * Converts to double before computation.
 * G = (2 * sum(i * x_i) for i=1..n) / (n * sum(x_i)) - (n+1)/n
 * Result clamped to [0, 1].
 */

__attribute__((target("default")))
DYNEMIT_NO_AUTOVECTORIZE
static double
gini_u64_scalar(const uint64_t *sorted_data, size_t n)
{
    if (n == 0) return 0.0;
    double weighted_sum = 0.0;
    double total_sum = 0.0;
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        double val = (double)sorted_data[i];
        weighted_sum += (double)(i + 1) * val;
        total_sum += val;
    }
    if (total_sum == 0.0) return 0.0;
    double g = (2.0 * weighted_sum) / ((double)n * total_sum) - ((double)(n + 1) / (double)n);
    if (g < 0.0) g = 0.0;
    if (g > 1.0) g = 1.0;
    return g;
}

__attribute__((target("sse2")))
static double
gini_u64_sse2(const uint64_t *sorted_data, size_t n)
{
    /* No efficient u64->f64 conversion in SSE2; use scalar */
    return gini_u64_scalar(sorted_data, n);
}

__attribute__((target("sse4.2")))
static double
gini_u64_sse42(const uint64_t *sorted_data, size_t n)
{
    return gini_u64_scalar(sorted_data, n);
}

__attribute__((target("avx")))
static double
gini_u64_avx(const uint64_t *sorted_data, size_t n)
{
    return gini_u64_scalar(sorted_data, n);
}

__attribute__((target("avx2")))
static double
gini_u64_avx2(const uint64_t *sorted_data, size_t n)
{
    /* AVX2 lacks native u64->f64; convert manually in groups of 4 */
    if (n == 0) return 0.0;
    size_t i = 0;
    __m256d vwsum = _mm256_setzero_pd();
    __m256d vtsum = _mm256_setzero_pd();
    __m256d vidx = _mm256_set_pd(4.0, 3.0, 2.0, 1.0);
    __m256d vstep = _mm256_set1_pd(4.0);
    for (; i + 4 <= n; i += 4) {
        /* Manual u64 to f64 conversion */
        __m256d vdata = _mm256_set_pd(
            (double)sorted_data[i + 3],
            (double)sorted_data[i + 2],
            (double)sorted_data[i + 1],
            (double)sorted_data[i]
        );
        vwsum = _mm256_add_pd(vwsum, _mm256_mul_pd(vidx, vdata));
        vtsum = _mm256_add_pd(vtsum, vdata);
        vidx = _mm256_add_pd(vidx, vstep);
    }
    __m128d lo, hi, s, sh;
    lo = _mm256_castpd256_pd128(vwsum);
    hi = _mm256_extractf128_pd(vwsum, 1);
    s = _mm_add_pd(lo, hi);
    sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double weighted_sum = _mm_cvtsd_f64(s);
    lo = _mm256_castpd256_pd128(vtsum);
    hi = _mm256_extractf128_pd(vtsum, 1);
    s = _mm_add_pd(lo, hi);
    sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double total_sum = _mm_cvtsd_f64(s);
    for (; i < n; i++) {
        double val = (double)sorted_data[i];
        weighted_sum += (double)(i + 1) * val;
        total_sum += val;
    }
    if (total_sum == 0.0) return 0.0;
    double g = (2.0 * weighted_sum) / ((double)n * total_sum) - ((double)(n + 1) / (double)n);
    if (g < 0.0) g = 0.0;
    if (g > 1.0) g = 1.0;
    return g;
}

__attribute__((target("avx512f")))
static double
gini_u64_avx512f(const uint64_t *sorted_data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m512d vwsum = _mm512_setzero_pd();
    __m512d vtsum = _mm512_setzero_pd();
    __m512d vidx = _mm512_set_pd(8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0);
    __m512d vstep = _mm512_set1_pd(8.0);
    for (; i + 8 <= n; i += 8) {
        /* AVX512F lacks native u64->f64; convert via temporary */
        __m512d vdata = _mm512_set_pd(
            (double)sorted_data[i + 7], (double)sorted_data[i + 6],
            (double)sorted_data[i + 5], (double)sorted_data[i + 4],
            (double)sorted_data[i + 3], (double)sorted_data[i + 2],
            (double)sorted_data[i + 1], (double)sorted_data[i]
        );
        vwsum = _mm512_fmadd_pd(vidx, vdata, vwsum);
        vtsum = _mm512_add_pd(vtsum, vdata);
        vidx = _mm512_add_pd(vidx, vstep);
    }
    double weighted_sum = _mm512_reduce_add_pd(vwsum);
    double total_sum = _mm512_reduce_add_pd(vtsum);
    for (; i < n; i++) {
        double val = (double)sorted_data[i];
        weighted_sum += (double)(i + 1) * val;
        total_sum += val;
    }
    if (total_sum == 0.0) return 0.0;
    double g = (2.0 * weighted_sum) / ((double)n * total_sum) - ((double)(n + 1) / (double)n);
    if (g < 0.0) g = 0.0;
    if (g > 1.0) g = 1.0;
    return g;
}

gini_u64_fn_t
gini_u64_select(simd_level_t level)
{
    switch (level) {
    case SIMD_AVX512F: return gini_u64_avx512f;
    case SIMD_AVX2:    return gini_u64_avx2;
    case SIMD_AVX:     return gini_u64_avx;
    case SIMD_SSE4_2:  return gini_u64_sse42;
    case SIMD_SSE2:    return gini_u64_sse2;
    case SIMD_SCALAR:
    default:           return gini_u64_scalar;
    }
}

static gini_u64_fn_t
gini_u64_resolver(void)
{
    return gini_u64_select(detect_simd_level());
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
double gini_u64(const uint64_t *sorted_data, size_t n)
    __attribute__((ifunc("gini_u64_resolver")));
