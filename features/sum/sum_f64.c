/* SPDX-License-Identifier: BSL-1.0 */
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <dynemit/sum.h>
#include <dynemit/compiler.h>

__attribute__((target("default")))
DYNEMIT_NO_AUTOVECTORIZE
static double
sum_f64_scalar(const double *data, size_t n)
{
    double sum = 0.0;
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++)
        sum += data[i];
    return sum;
}

__attribute__((target("sse2")))
static double
sum_f64_sse2(const double *data, size_t n)
{
    size_t i = 0;
    __m128d vsum = _mm_setzero_pd();
    for (; i + 2 <= n; i += 2)
        vsum = _mm_add_pd(vsum, _mm_loadu_pd(data + i));
    __m128d hi = _mm_unpackhi_pd(vsum, vsum);
    vsum = _mm_add_pd(vsum, hi);
    double sum = _mm_cvtsd_f64(vsum);
    for (; i < n; i++)
        sum += data[i];
    return sum;
}

__attribute__((target("sse4.2")))
static double
sum_f64_sse42(const double *data, size_t n)
{
    return sum_f64_sse2(data, n);
}

__attribute__((target("avx")))
static double
sum_f64_avx(const double *data, size_t n)
{
    size_t i = 0;
    __m256d vsum = _mm256_setzero_pd();
    for (; i + 4 <= n; i += 4)
        vsum = _mm256_add_pd(vsum, _mm256_loadu_pd(data + i));
    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    __m128d s  = _mm_add_pd(lo, hi);
    __m128d sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double sum = _mm_cvtsd_f64(s);
    for (; i < n; i++)
        sum += data[i];
    return sum;
}

__attribute__((target("avx2")))
static double
sum_f64_avx2(const double *data, size_t n)
{
    return sum_f64_avx(data, n);
}

__attribute__((target("avx512f")))
static double
sum_f64_avx512f(const double *data, size_t n)
{
    size_t i = 0;
    __m512d vsum = _mm512_setzero_pd();
    for (; i + 8 <= n; i += 8)
        vsum = _mm512_add_pd(vsum, _mm512_loadu_pd(data + i));
    double sum = _mm512_reduce_add_pd(vsum);
    for (; i < n; i++)
        sum += data[i];
    return sum;
}

sum_f64_fn_t
sum_f64_select(simd_level_t level)
{
    switch (level) {
    case SIMD_AVX512F: return sum_f64_avx512f;
    case SIMD_AVX2:    return sum_f64_avx2;
    case SIMD_AVX:     return sum_f64_avx;
    case SIMD_SSE4_2:  return sum_f64_sse42;
    case SIMD_SSE2:    return sum_f64_sse2;
    case SIMD_SCALAR:
    default:           return sum_f64_scalar;
    }
}

static sum_f64_fn_t
sum_f64_resolver(void)
{
    return sum_f64_select(detect_simd_level());
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
double sum_f64(const double *data, size_t n)
    __attribute__((ifunc("sum_f64_resolver")));
