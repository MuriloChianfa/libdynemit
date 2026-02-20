/* SPDX-License-Identifier: BSL-1.0 */
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <dynemit/max.h>
#include <dynemit/compiler.h>

__attribute__((target("default")))
DYNEMIT_NO_AUTOVECTORIZE
static double
max_u32_scalar(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint32_t result = data[0];
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 1; i < n; i++)
        if (data[i] > result) result = data[i];
    return (double)result;
}

__attribute__((target("sse2")))
static double
max_u32_sse2(const uint32_t *data, size_t n)
{
    return max_u32_scalar(data, n);
}

__attribute__((target("sse4.2")))
static double
max_u32_sse42(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m128i vmax = _mm_setzero_si128();
    for (; i + 4 <= n; i += 4)
        vmax = _mm_max_epu32(vmax, _mm_loadu_si128((const __m128i *)(data + i)));
    vmax = _mm_max_epu32(vmax, _mm_srli_si128(vmax, 8));
    vmax = _mm_max_epu32(vmax, _mm_srli_si128(vmax, 4));
    uint32_t result = (uint32_t)_mm_extract_epi32(vmax, 0);
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return (double)result;
}

__attribute__((target("avx")))
static double
max_u32_avx(const uint32_t *data, size_t n)
{
    return max_u32_sse42(data, n);
}

__attribute__((target("avx2")))
static double
max_u32_avx2(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m256i vmax = _mm256_setzero_si256();
    for (; i + 8 <= n; i += 8)
        vmax = _mm256_max_epu32(vmax, _mm256_loadu_si256((const __m256i *)(data + i)));
    __m128i lo = _mm256_castsi256_si128(vmax);
    __m128i hi = _mm256_extracti128_si256(vmax, 1);
    __m128i m  = _mm_max_epu32(lo, hi);
    m = _mm_max_epu32(m, _mm_srli_si128(m, 8));
    m = _mm_max_epu32(m, _mm_srli_si128(m, 4));
    uint32_t result = (uint32_t)_mm_extract_epi32(m, 0);
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return (double)result;
}

__attribute__((target("avx512f")))
static double
max_u32_avx512f(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m512i vmax = _mm512_setzero_si512();
    for (; i + 16 <= n; i += 16)
        vmax = _mm512_max_epu32(vmax, _mm512_loadu_si512(data + i));
    uint32_t result = _mm512_reduce_max_epu32(vmax);
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return (double)result;
}

max_u32_fn_t
max_u32_select(simd_level_t level)
{
    switch (level) {
    case SIMD_AVX512F: return max_u32_avx512f;
    case SIMD_AVX2:    return max_u32_avx2;
    case SIMD_AVX:     return max_u32_avx;
    case SIMD_SSE4_2:  return max_u32_sse42;
    case SIMD_SSE2:    return max_u32_sse2;
    case SIMD_SCALAR:
    default:           return max_u32_scalar;
    }
}

static max_u32_fn_t
max_u32_resolver(void)
{
    return max_u32_select(detect_simd_level());
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
double max_u32(const uint32_t *data, size_t n)
    __attribute__((ifunc("max_u32_resolver")));
