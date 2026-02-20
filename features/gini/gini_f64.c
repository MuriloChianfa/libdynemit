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
 * Gini coefficient for pre-sorted ascending f64 data.
 * G = (2 * sum(i * x_i) for i=1..n) / (n * sum(x_i)) - (n+1)/n
 * Result clamped to [0, 1].
 */

__attribute__((target("default")))
DYNEMIT_NO_AUTOVECTORIZE
static double
gini_f64_scalar(const double *sorted_data, size_t n)
{
    if (n == 0) return 0.0;
    double weighted_sum = 0.0;
    double total_sum = 0.0;
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        weighted_sum += (double)(i + 1) * sorted_data[i];
        total_sum += sorted_data[i];
    }
    if (total_sum == 0.0) return 0.0;
    double g = (2.0 * weighted_sum) / ((double)n * total_sum) - ((double)(n + 1) / (double)n);
    if (g < 0.0) g = 0.0;
    if (g > 1.0) g = 1.0;
    return g;
}

__attribute__((target("sse2")))
static double
gini_f64_sse2(const double *sorted_data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m128d vwsum = _mm_setzero_pd();
    __m128d vtsum = _mm_setzero_pd();
    __m128d vidx = _mm_set_pd(2.0, 1.0);
    __m128d vstep = _mm_set1_pd(2.0);
    for (; i + 2 <= n; i += 2) {
        __m128d vdata = _mm_loadu_pd(sorted_data + i);
        vwsum = _mm_add_pd(vwsum, _mm_mul_pd(vidx, vdata));
        vtsum = _mm_add_pd(vtsum, vdata);
        vidx = _mm_add_pd(vidx, vstep);
    }
    __m128d hi;
    hi = _mm_unpackhi_pd(vwsum, vwsum);
    vwsum = _mm_add_pd(vwsum, hi);
    double weighted_sum = _mm_cvtsd_f64(vwsum);
    hi = _mm_unpackhi_pd(vtsum, vtsum);
    vtsum = _mm_add_pd(vtsum, hi);
    double total_sum = _mm_cvtsd_f64(vtsum);
    for (; i < n; i++) {
        weighted_sum += (double)(i + 1) * sorted_data[i];
        total_sum += sorted_data[i];
    }
    if (total_sum == 0.0) return 0.0;
    double g = (2.0 * weighted_sum) / ((double)n * total_sum) - ((double)(n + 1) / (double)n);
    if (g < 0.0) g = 0.0;
    if (g > 1.0) g = 1.0;
    return g;
}

__attribute__((target("sse4.2")))
static double
gini_f64_sse42(const double *sorted_data, size_t n)
{
    return gini_f64_sse2(sorted_data, n);
}

__attribute__((target("avx")))
static double
gini_f64_avx(const double *sorted_data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m256d vwsum = _mm256_setzero_pd();
    __m256d vtsum = _mm256_setzero_pd();
    __m256d vidx = _mm256_set_pd(4.0, 3.0, 2.0, 1.0);
    __m256d vstep = _mm256_set1_pd(4.0);
    for (; i + 4 <= n; i += 4) {
        __m256d vdata = _mm256_loadu_pd(sorted_data + i);
        vwsum = _mm256_add_pd(vwsum, _mm256_mul_pd(vidx, vdata));
        vtsum = _mm256_add_pd(vtsum, vdata);
        vidx = _mm256_add_pd(vidx, vstep);
    }
    /* Horizontal reduction */
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
        weighted_sum += (double)(i + 1) * sorted_data[i];
        total_sum += sorted_data[i];
    }
    if (total_sum == 0.0) return 0.0;
    double g = (2.0 * weighted_sum) / ((double)n * total_sum) - ((double)(n + 1) / (double)n);
    if (g < 0.0) g = 0.0;
    if (g > 1.0) g = 1.0;
    return g;
}

__attribute__((target("avx2")))
static double
gini_f64_avx2(const double *sorted_data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m256d vwsum = _mm256_setzero_pd();
    __m256d vtsum = _mm256_setzero_pd();
    __m256d vidx = _mm256_set_pd(4.0, 3.0, 2.0, 1.0);
    __m256d vstep = _mm256_set1_pd(4.0);
    for (; i + 4 <= n; i += 4) {
        __m256d vdata = _mm256_loadu_pd(sorted_data + i);
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
        weighted_sum += (double)(i + 1) * sorted_data[i];
        total_sum += sorted_data[i];
    }
    if (total_sum == 0.0) return 0.0;
    double g = (2.0 * weighted_sum) / ((double)n * total_sum) - ((double)(n + 1) / (double)n);
    if (g < 0.0) g = 0.0;
    if (g > 1.0) g = 1.0;
    return g;
}

__attribute__((target("avx512f")))
static double
gini_f64_avx512f(const double *sorted_data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m512d vwsum = _mm512_setzero_pd();
    __m512d vtsum = _mm512_setzero_pd();
    __m512d vidx = _mm512_set_pd(8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0);
    __m512d vstep = _mm512_set1_pd(8.0);
    for (; i + 8 <= n; i += 8) {
        __m512d vdata = _mm512_loadu_pd(sorted_data + i);
        vwsum = _mm512_fmadd_pd(vidx, vdata, vwsum);
        vtsum = _mm512_add_pd(vtsum, vdata);
        vidx = _mm512_add_pd(vidx, vstep);
    }
    double weighted_sum = _mm512_reduce_add_pd(vwsum);
    double total_sum = _mm512_reduce_add_pd(vtsum);
    for (; i < n; i++) {
        weighted_sum += (double)(i + 1) * sorted_data[i];
        total_sum += sorted_data[i];
    }
    if (total_sum == 0.0) return 0.0;
    double g = (2.0 * weighted_sum) / ((double)n * total_sum) - ((double)(n + 1) / (double)n);
    if (g < 0.0) g = 0.0;
    if (g > 1.0) g = 1.0;
    return g;
}

gini_f64_fn_t
gini_f64_select(simd_level_t level)
{
    switch (level) {
    case SIMD_AVX512F: return gini_f64_avx512f;
    case SIMD_AVX2:    return gini_f64_avx2;
    case SIMD_AVX:     return gini_f64_avx;
    case SIMD_SSE4_2:  return gini_f64_sse42;
    case SIMD_SSE2:    return gini_f64_sse2;
    case SIMD_SCALAR:
    default:           return gini_f64_scalar;
    }
}

static gini_f64_fn_t
gini_f64_resolver(void)
{
    return gini_f64_select(detect_simd_level());
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
double gini_f64(const double *sorted_data, size_t n)
    __attribute__((ifunc("gini_f64_resolver")));
