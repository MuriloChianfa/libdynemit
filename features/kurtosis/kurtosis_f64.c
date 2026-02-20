/* SPDX-License-Identifier: BSL-1.0 */
#include <immintrin.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <dynemit/kurtosis.h>
#include <dynemit/compiler.h>
#include <dynemit/mean.h>

__attribute__((target("default")))
DYNEMIT_NO_AUTOVECTORIZE
static double
kurtosis_f64_scalar(const double *data, size_t n)
{
    if (n < 4) return 0.0;
    double m = mean_f64(data, n);
    double sum2 = 0.0, sum4 = 0.0;
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        double d  = data[i] - m;
        double d2 = d * d;
        sum2 += d2;
        sum4 += d2 * d2;
    }
    double var = sum2 / (double)n;
    if (var < 1e-30) return 0.0;
    return (sum4 / (double)n) / (var * var) - 3.0;
}

__attribute__((target("sse2")))
static double
kurtosis_f64_sse2(const double *data, size_t n)
{
    return kurtosis_f64_scalar(data, n);
}

__attribute__((target("sse4.2")))
static double
kurtosis_f64_sse42(const double *data, size_t n)
{
    return kurtosis_f64_scalar(data, n);
}

__attribute__((target("avx")))
static double
kurtosis_f64_avx(const double *data, size_t n)
{
    if (n < 4) return 0.0;
    double m = mean_f64(data, n);
    size_t i = 0;
    __m256d vmean = _mm256_set1_pd(m);
    __m256d vsum2 = _mm256_setzero_pd();
    __m256d vsum4 = _mm256_setzero_pd();
    for (; i + 4 <= n; i += 4) {
        __m256d v  = _mm256_loadu_pd(data + i);
        __m256d d  = _mm256_sub_pd(v, vmean);
        __m256d d2 = _mm256_mul_pd(d, d);
        vsum2 = _mm256_add_pd(vsum2, d2);
        vsum4 = _mm256_add_pd(vsum4, _mm256_mul_pd(d2, d2));
    }
    __m128d lo2 = _mm256_castpd256_pd128(vsum2);
    __m128d hi2 = _mm256_extractf128_pd(vsum2, 1);
    __m128d s2  = _mm_add_pd(lo2, hi2);
    s2 = _mm_add_pd(s2, _mm_unpackhi_pd(s2, s2));
    double sum2 = _mm_cvtsd_f64(s2);

    __m128d lo4 = _mm256_castpd256_pd128(vsum4);
    __m128d hi4 = _mm256_extractf128_pd(vsum4, 1);
    __m128d s4  = _mm_add_pd(lo4, hi4);
    s4 = _mm_add_pd(s4, _mm_unpackhi_pd(s4, s4));
    double sum4 = _mm_cvtsd_f64(s4);

    for (; i < n; i++) {
        double d  = data[i] - m;
        double d2 = d * d;
        sum2 += d2;
        sum4 += d2 * d2;
    }
    double var = sum2 / (double)n;
    if (var < 1e-30) return 0.0;
    return (sum4 / (double)n) / (var * var) - 3.0;
}

__attribute__((target("avx2")))
static double
kurtosis_f64_avx2(const double *data, size_t n)
{
    return kurtosis_f64_avx(data, n);
}

__attribute__((target("avx512f")))
static double
kurtosis_f64_avx512f(const double *data, size_t n)
{
    if (n < 4) return 0.0;
    double m = mean_f64(data, n);
    size_t i = 0;
    __m512d vmean = _mm512_set1_pd(m);
    __m512d vsum2 = _mm512_setzero_pd();
    __m512d vsum4 = _mm512_setzero_pd();
    for (; i + 8 <= n; i += 8) {
        __m512d v  = _mm512_loadu_pd(data + i);
        __m512d d  = _mm512_sub_pd(v, vmean);
        __m512d d2 = _mm512_mul_pd(d, d);
        vsum2 = _mm512_add_pd(vsum2, d2);
        vsum4 = _mm512_fmadd_pd(d2, d2, vsum4);
    }
    double sum2 = _mm512_reduce_add_pd(vsum2);
    double sum4 = _mm512_reduce_add_pd(vsum4);
    for (; i < n; i++) {
        double d  = data[i] - m;
        double d2 = d * d;
        sum2 += d2;
        sum4 += d2 * d2;
    }
    double var = sum2 / (double)n;
    if (var < 1e-30) return 0.0;
    return (sum4 / (double)n) / (var * var) - 3.0;
}

kurtosis_f64_fn_t
kurtosis_f64_select(simd_level_t level)
{
    switch (level) {
    case SIMD_AVX512F: return kurtosis_f64_avx512f;
    case SIMD_AVX2:    return kurtosis_f64_avx2;
    case SIMD_AVX:     return kurtosis_f64_avx;
    case SIMD_SSE4_2:  return kurtosis_f64_sse42;
    case SIMD_SSE2:    return kurtosis_f64_sse2;
    case SIMD_SCALAR:
    default:           return kurtosis_f64_scalar;
    }
}

static kurtosis_f64_fn_t
kurtosis_f64_resolver(void)
{
    return kurtosis_f64_select(detect_simd_level());
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
double kurtosis_f64(const double *data, size_t n)
    __attribute__((ifunc("kurtosis_f64_resolver")));
