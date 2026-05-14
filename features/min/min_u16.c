/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <dynemit/min.h>
#include <dynemit/compiler.h>

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
min_u16_scalar(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint16_t result = data[0];
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 1; i < n; i++)
        if (data[i] < result) result = data[i];
    return (double)result;
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static double
min_u16_sse2(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m128i bias = _mm_set1_epi16((short)0x8000);
    __m128i vmin = _mm_set1_epi16((short)0x7FFF);
    for (; i + 8 <= n; i += 8) {
        __m128i v = _mm_loadu_si128((const __m128i *)(data + i));
        __m128i biased = _mm_xor_si128(v, bias);
        vmin = _mm_min_epi16(vmin, biased);
    }
    vmin = _mm_xor_si128(vmin, bias);
    vmin = _mm_min_epu8(vmin, _mm_srli_si128(vmin, 8));
    vmin = _mm_min_epu8(vmin, _mm_srli_si128(vmin, 4));
    vmin = _mm_min_epu8(vmin, _mm_srli_si128(vmin, 2));
    uint16_t result = (uint16_t)(_mm_extract_epi16(vmin, 0) & 0xFFFF);
    for (; i < n; i++)
        if (data[i] < result) result = data[i];
    return (double)result;
}

__attribute__((target("sse4.2")))
static double
min_u16_sse42(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m128i vmin = _mm_set1_epi16((short)UINT16_MAX);
    for (; i + 8 <= n; i += 8)
        vmin = _mm_min_epu16(vmin, _mm_loadu_si128((const __m128i *)(data + i)));
    vmin = _mm_min_epu16(vmin, _mm_srli_si128(vmin, 8));
    vmin = _mm_min_epu16(vmin, _mm_srli_si128(vmin, 4));
    vmin = _mm_min_epu16(vmin, _mm_srli_si128(vmin, 2));
    uint16_t result = (uint16_t)(_mm_extract_epi16(vmin, 0) & 0xFFFF);
    for (; i < n; i++)
        if (data[i] < result) result = data[i];
    return (double)result;
}

__attribute__((target("avx")))
static double
min_u16_avx(const uint16_t *data, size_t n)
{
    return min_u16_sse42(data, n);
}

__attribute__((target("avx2")))
static double
min_u16_avx2(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m256i vmin = _mm256_set1_epi16((short)UINT16_MAX);
    for (; i + 16 <= n; i += 16)
        vmin = _mm256_min_epu16(vmin, _mm256_loadu_si256((const __m256i *)(data + i)));
    __m128i lo = _mm256_castsi256_si128(vmin);
    __m128i hi = _mm256_extracti128_si256(vmin, 1);
    __m128i m  = _mm_min_epu16(lo, hi);
    m = _mm_min_epu16(m, _mm_srli_si128(m, 8));
    m = _mm_min_epu16(m, _mm_srli_si128(m, 4));
    m = _mm_min_epu16(m, _mm_srli_si128(m, 2));
    uint16_t result = (uint16_t)(_mm_extract_epi16(m, 0) & 0xFFFF);
    for (; i < n; i++)
        if (data[i] < result) result = data[i];
    return (double)result;
}

__attribute__((target("avx512f")))
static double
min_u16_avx512f(const uint16_t *data, size_t n)
{
    return min_u16_avx2(data, n);
}
#endif /* x86 */

#if defined(__aarch64__)

static double
min_u16_neon(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    uint16x8_t vmin = vdupq_n_u16(UINT16_MAX);
    for (; i + 8 <= n; i += 8)
        vmin = vminq_u16(vmin, vld1q_u16(data + i));
    uint16_t result = vminvq_u16(vmin);
    for (; i < n; i++)
        if (data[i] < result) result = data[i];
    return (double)result;
}

__attribute__((target("+sve")))
static double
min_u16_sve(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    svuint16_t vmin = svdup_u16(UINT16_MAX);
    svbool_t pg = svwhilelt_b16(i, (uint64_t)n);
    do {
        vmin = svmin_u16_m(pg, vmin, svld1_u16(pg, data + i));
        i += svcnth();
        pg = svwhilelt_b16(i, (uint64_t)n);
    } while (svptest_any(svptrue_b16(), pg));
    return (double)svminv_u16(svptrue_b16(), vmin);
}

__attribute__((target("+sve2")))
static double
min_u16_sve2(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    uint64_t vl = svcnth();
    svbool_t ptrue = svptrue_b16();
    svuint16_t vmin0 = svdup_u16(UINT16_MAX);
    svuint16_t vmin1 = svdup_u16(UINT16_MAX);
    for (; i + 2 * vl <= n; i += 2 * vl) {
        vmin0 = svmin_u16_x(ptrue, vmin0, svld1_u16(ptrue, data + i));
        vmin1 = svmin_u16_x(ptrue, vmin1, svld1_u16(ptrue, data + i + vl));
    }
    svuint16_t vmin = svminp_u16_x(ptrue, vmin0, vmin1);
    svbool_t pg = svwhilelt_b16(i, (uint64_t)n);
    while (svptest_any(ptrue, pg)) {
        vmin = svmin_u16_m(pg, vmin, svld1_u16(pg, data + i));
        i += vl;
        pg = svwhilelt_b16(i, (uint64_t)n);
    }
    return (double)svminv_u16(ptrue, vmin);
}

#endif /* aarch64 */

min_u16_fn_t
min_u16_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return min_u16_avx512f;
    case SIMD_AVX2:    return min_u16_avx2;
    case SIMD_AVX:     return min_u16_avx;
    case SIMD_SSE4_2:  return min_u16_sse42;
    case SIMD_SSE2:    return min_u16_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return min_u16_sve2;
    case SIMD_SVE:     return min_u16_sve;
    case SIMD_NEON:    return min_u16_neon;
#endif
    case SIMD_SCALAR:
    default:           return min_u16_scalar;
    }
}

static min_u16_fn_t
min_u16_resolver(void)
{
    return min_u16_select(detect_simd_level());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
double min_u16(const uint16_t *data, size_t n)
    __attribute__((ifunc("min_u16_resolver")));
