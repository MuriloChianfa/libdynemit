/* SPDX-License-Identifier: BSL-1.0 */
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <dynemit/sum.h>
#include <dynemit/compiler.h>

__attribute__((target("default")))
DYNEMIT_NO_AUTOVECTORIZE
static double
sum_u16_scalar(const uint16_t *data, size_t n)
{
    double sum = 0.0;
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++)
        sum += (double)data[i];
    return sum;
}

__attribute__((target("sse2")))
static double
sum_u16_sse2(const uint16_t *data, size_t n)
{
    size_t i = 0;
    __m128d vsum = _mm_setzero_pd();
    for (; i + 8 <= n; i += 8) {
        __m128i v16 = _mm_loadu_si128((const __m128i *)(data + i));
        __m128i lo32 = _mm_unpacklo_epi16(v16, _mm_setzero_si128());
        __m128i hi32 = _mm_unpackhi_epi16(v16, _mm_setzero_si128());
        __m128d d0 = _mm_cvtepi32_pd(lo32);
        __m128d d1 = _mm_cvtepi32_pd(_mm_srli_si128(lo32, 8));
        __m128d d2 = _mm_cvtepi32_pd(hi32);
        __m128d d3 = _mm_cvtepi32_pd(_mm_srli_si128(hi32, 8));
        vsum = _mm_add_pd(vsum, d0);
        vsum = _mm_add_pd(vsum, d1);
        vsum = _mm_add_pd(vsum, d2);
        vsum = _mm_add_pd(vsum, d3);
    }
    __m128d h = _mm_unpackhi_pd(vsum, vsum);
    vsum = _mm_add_pd(vsum, h);
    double sum = _mm_cvtsd_f64(vsum);
    for (; i < n; i++)
        sum += (double)data[i];
    return sum;
}

__attribute__((target("sse4.2")))
static double
sum_u16_sse42(const uint16_t *data, size_t n)
{
    size_t i = 0;
    __m128d vsum = _mm_setzero_pd();
    for (; i + 8 <= n; i += 8) {
        __m128i v16 = _mm_loadu_si128((const __m128i *)(data + i));
        __m128i lo32 = _mm_cvtepu16_epi32(v16);
        __m128i hi32 = _mm_cvtepu16_epi32(_mm_srli_si128(v16, 8));
        __m128d d0 = _mm_cvtepi32_pd(lo32);
        __m128d d1 = _mm_cvtepi32_pd(_mm_srli_si128(lo32, 8));
        __m128d d2 = _mm_cvtepi32_pd(hi32);
        __m128d d3 = _mm_cvtepi32_pd(_mm_srli_si128(hi32, 8));
        vsum = _mm_add_pd(vsum, d0);
        vsum = _mm_add_pd(vsum, d1);
        vsum = _mm_add_pd(vsum, d2);
        vsum = _mm_add_pd(vsum, d3);
    }
    __m128d h = _mm_unpackhi_pd(vsum, vsum);
    vsum = _mm_add_pd(vsum, h);
    double sum = _mm_cvtsd_f64(vsum);
    for (; i < n; i++)
        sum += (double)data[i];
    return sum;
}

__attribute__((target("avx")))
static double
sum_u16_avx(const uint16_t *data, size_t n)
{
    return sum_u16_sse42(data, n);
}

__attribute__((target("avx2")))
static double
sum_u16_avx2(const uint16_t *data, size_t n)
{
    size_t i = 0;
    __m256d vsum = _mm256_setzero_pd();
    for (; i + 16 <= n; i += 16) {
        __m256i v16 = _mm256_loadu_si256((const __m256i *)(data + i));
        __m128i lo16 = _mm256_castsi256_si128(v16);
        __m128i hi16 = _mm256_extracti128_si256(v16, 1);
        __m256i lo32 = _mm256_cvtepu16_epi32(lo16);
        __m256i hi32 = _mm256_cvtepu16_epi32(hi16);
        __m128i a = _mm256_castsi256_si128(lo32);
        __m128i b = _mm256_extracti128_si256(lo32, 1);
        __m128i c = _mm256_castsi256_si128(hi32);
        __m128i d = _mm256_extracti128_si256(hi32, 1);
        vsum = _mm256_add_pd(vsum, _mm256_cvtepi32_pd(a));
        vsum = _mm256_add_pd(vsum, _mm256_cvtepi32_pd(b));
        vsum = _mm256_add_pd(vsum, _mm256_cvtepi32_pd(c));
        vsum = _mm256_add_pd(vsum, _mm256_cvtepi32_pd(d));
    }
    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    __m128d s  = _mm_add_pd(lo, hi);
    __m128d sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double sum = _mm_cvtsd_f64(s);
    for (; i < n; i++)
        sum += (double)data[i];
    return sum;
}

__attribute__((target("avx512f")))
static double
sum_u16_avx512f(const uint16_t *data, size_t n)
{
    return sum_u16_avx2(data, n);
}

sum_u16_fn_t
sum_u16_select(simd_level_t level)
{
    switch (level) {
    case SIMD_AVX512F: return sum_u16_avx512f;
    case SIMD_AVX2:    return sum_u16_avx2;
    case SIMD_AVX:     return sum_u16_avx;
    case SIMD_SSE4_2:  return sum_u16_sse42;
    case SIMD_SSE2:    return sum_u16_sse2;
    case SIMD_SCALAR:
    default:           return sum_u16_scalar;
    }
}

static sum_u16_fn_t
sum_u16_resolver(void)
{
    return sum_u16_select(detect_simd_level());
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
double sum_u16(const uint16_t *data, size_t n)
    __attribute__((ifunc("sum_u16_resolver")));
