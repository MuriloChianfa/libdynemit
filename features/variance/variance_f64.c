/* SPDX-License-Identifier: BSL-1.0 */
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <dynemit/variance.h>
#include <dynemit/compiler.h>
#include <dynemit/mean.h>

__attribute__((target("default")))
DYNEMIT_NO_AUTOVECTORIZE
static double
variance_f64_scalar(const double *data, size_t n)
{
    if (n < 2) return 0.0;
    double m = mean_f64(data, n);
    double sum2 = 0.0;
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        double d = data[i] - m;
        sum2 += d * d;
    }
    return sum2 / (double)(n - 1);
}

__attribute__((target("sse2")))
static double
variance_f64_sse2(const double *data, size_t n)
{
    if (n < 2) return 0.0;
    double m = mean_f64(data, n);
    size_t i = 0;
    __m128d vmean = _mm_set1_pd(m);
    __m128d vsum2 = _mm_setzero_pd();
    for (; i + 2 <= n; i += 2) {
        __m128d v = _mm_loadu_pd(data + i);
        __m128d d = _mm_sub_pd(v, vmean);
        vsum2 = _mm_add_pd(vsum2, _mm_mul_pd(d, d));
    }
    __m128d hi = _mm_unpackhi_pd(vsum2, vsum2);
    vsum2 = _mm_add_pd(vsum2, hi);
    double sum2 = _mm_cvtsd_f64(vsum2);
    for (; i < n; i++) {
        double d = data[i] - m;
        sum2 += d * d;
    }
    return sum2 / (double)(n - 1);
}

__attribute__((target("sse4.2")))
static double
variance_f64_sse42(const double *data, size_t n)
{
    return variance_f64_sse2(data, n);
}

__attribute__((target("avx")))
static double
variance_f64_avx(const double *data, size_t n)
{
    if (n < 2) return 0.0;
    double m = mean_f64(data, n);
    size_t i = 0;
    __m256d vmean = _mm256_set1_pd(m);
    __m256d vsum2 = _mm256_setzero_pd();
    for (; i + 4 <= n; i += 4) {
        __m256d v = _mm256_loadu_pd(data + i);
        __m256d d = _mm256_sub_pd(v, vmean);
        vsum2 = _mm256_add_pd(vsum2, _mm256_mul_pd(d, d));
    }
    __m128d lo = _mm256_castpd256_pd128(vsum2);
    __m128d hi = _mm256_extractf128_pd(vsum2, 1);
    __m128d s  = _mm_add_pd(lo, hi);
    __m128d sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double sum2 = _mm_cvtsd_f64(s);
    for (; i < n; i++) {
        double d = data[i] - m;
        sum2 += d * d;
    }
    return sum2 / (double)(n - 1);
}

__attribute__((target("avx2")))
static double
variance_f64_avx2(const double *data, size_t n)
{
    return variance_f64_avx(data, n);
}

__attribute__((target("avx512f")))
static double
variance_f64_avx512f(const double *data, size_t n)
{
    if (n < 2) return 0.0;
    double m = mean_f64(data, n);
    size_t i = 0;
    __m512d vmean = _mm512_set1_pd(m);
    __m512d vsum2 = _mm512_setzero_pd();
    for (; i + 8 <= n; i += 8) {
        __m512d v = _mm512_loadu_pd(data + i);
        __m512d d = _mm512_sub_pd(v, vmean);
        vsum2 = _mm512_fmadd_pd(d, d, vsum2);
    }
    double sum2 = _mm512_reduce_add_pd(vsum2);
    for (; i < n; i++) {
        double d = data[i] - m;
        sum2 += d * d;
    }
    return sum2 / (double)(n - 1);
}

variance_f64_fn_t
variance_f64_select(simd_level_t level)
{
    switch (level) {
    case SIMD_AVX512F: return variance_f64_avx512f;
    case SIMD_AVX2:    return variance_f64_avx2;
    case SIMD_AVX:     return variance_f64_avx;
    case SIMD_SSE4_2:  return variance_f64_sse42;
    case SIMD_SSE2:    return variance_f64_sse2;
    case SIMD_SCALAR:
    default:           return variance_f64_scalar;
    }
}

static variance_f64_fn_t
variance_f64_resolver(void)
{
    return variance_f64_select(detect_simd_level());
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
double variance_f64(const double *data, size_t n)
    __attribute__((ifunc("variance_f64_resolver")));
