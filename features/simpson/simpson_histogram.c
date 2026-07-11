/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elifdef __aarch64__
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <dynemit/compiler.h>
#include <dynemit/simpson.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
simpson_histogram_scalar(const uint64_t *counts, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    double total = 0.0;
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        total += (double)counts[i];
    }
    if (total == 0.0) {
        return 0.0;
    }
    double sum_sq = 0.0;
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        double p = (double)counts[i] / total;
        sum_sq += p * p;
    }
    return 1.0 - sum_sq;
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static double
simpson_histogram_sse2(const uint64_t *counts, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    double total = 0.0;
    for (size_t i = 0; i < n; i++) {
        total += (double)counts[i];
    }
    if (total == 0.0) {
        return 0.0;
    }
    __m128d vtotal = _mm_set1_pd(total);
    __m128d vsum = _mm_setzero_pd();
    size_t i = 0;
    for (; i + 2 <= n; i += 2) {
        __m128d v = _mm_setr_pd((double)counts[i], (double)counts[i + 1]);
        __m128d p = _mm_div_pd(v, vtotal);
        vsum = _mm_add_pd(vsum, _mm_mul_pd(p, p));
    }
    __m128d hi = _mm_unpackhi_pd(vsum, vsum);
    vsum = _mm_add_pd(vsum, hi);
    double sum_sq = _mm_cvtsd_f64(vsum);
    for (; i < n; i++) {
        double p = (double)counts[i] / total;
        sum_sq += p * p;
    }
    return 1.0 - sum_sq;
}

__attribute__((target("sse4.2")))
static double
simpson_histogram_sse42(const uint64_t *counts, size_t n)
{
    return simpson_histogram_sse2(counts, n);
}

__attribute__((target("avx")))
static double
simpson_histogram_avx(const uint64_t *counts, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    double total = 0.0;
    for (size_t i = 0; i < n; i++) {
        total += (double)counts[i];
    }
    if (total == 0.0) {
        return 0.0;
    }
    __m256d vtotal = _mm256_set1_pd(total);
    __m256d vsum = _mm256_setzero_pd();
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        __m256d v = _mm256_setr_pd(
            (double)counts[i],     (double)counts[i + 1],
            (double)counts[i + 2], (double)counts[i + 3]);
        __m256d p = _mm256_div_pd(v, vtotal);
        vsum = _mm256_add_pd(vsum, _mm256_mul_pd(p, p));
    }
    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    __m128d s  = _mm_add_pd(lo, hi);
    __m128d sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double sum_sq = _mm_cvtsd_f64(s);
    for (; i < n; i++) {
        double p = (double)counts[i] / total;
        sum_sq += p * p;
    }
    return 1.0 - sum_sq;
}

__attribute__((target("avx2")))
static double
simpson_histogram_avx2(const uint64_t *counts, size_t n)
{
    return simpson_histogram_avx(counts, n);
}

__attribute__((target("avx512f")))
static double
simpson_histogram_avx512f(const uint64_t *counts, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    double total = 0.0;
    for (size_t i = 0; i < n; i++) {
        total += (double)counts[i];
    }
    if (total == 0.0) {
        return 0.0;
    }
    __m512d vtotal = _mm512_set1_pd(total);
    __m512d vsum = _mm512_setzero_pd();
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m512d v = _mm512_setr_pd(
            (double)counts[i],     (double)counts[i + 1],
            (double)counts[i + 2], (double)counts[i + 3],
            (double)counts[i + 4], (double)counts[i + 5],
            (double)counts[i + 6], (double)counts[i + 7]);
        __m512d p = _mm512_div_pd(v, vtotal);
        vsum = _mm512_fmadd_pd(p, p, vsum);
    }
    double sum_sq = _mm512_reduce_add_pd(vsum);
    for (; i < n; i++) {
        double p = (double)counts[i] / total;
        sum_sq += p * p;
    }
    return 1.0 - sum_sq;
}
#endif

#ifdef __aarch64__

static double
simpson_histogram_neon(const uint64_t *counts, size_t n)
{
    return simpson_histogram_scalar(counts, n);
}

__attribute__((target("+sve")))
static double
simpson_histogram_sve(const uint64_t *counts, size_t n)
{
    return simpson_histogram_scalar(counts, n);
}

__attribute__((target("+sve2")))
static double
simpson_histogram_sve2(const uint64_t *counts, size_t n)
{
    return simpson_histogram_scalar(counts, n);
}

#endif /* aarch64 */

simpson_histogram_fn_t
simpson_histogram_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return simpson_histogram_avx512f;
    case SIMD_AVX2:    return simpson_histogram_avx2;
    case SIMD_AVX:     return simpson_histogram_avx;
    case SIMD_SSE4_2:  return simpson_histogram_sse42;
    case SIMD_SSE2:    return simpson_histogram_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return simpson_histogram_sve2;
    case SIMD_SVE:     return simpson_histogram_sve;
    case SIMD_NEON:    return simpson_histogram_neon;
#endif
    case SIMD_SCALAR:
    default:           return simpson_histogram_scalar;
}
}

EXPLICIT_RUNTIME_RESOLVER(simpson_histogram_resolver, simpson_histogram_fn_t)
{
    return simpson_histogram_select(detect_simd_level_ts());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
double simpson_histogram(const uint64_t *counts, size_t n)
    __attribute__((ifunc("simpson_histogram_resolver")));
