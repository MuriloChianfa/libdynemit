/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <dynemit/max.h>
#include <dynemit/compiler.h>

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
max_u16_scalar(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint16_t result = data[0];
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 1; i < n; i++)
        if (data[i] > result) result = data[i];
    return (double)result;
}

#if defined(__x86_64__) || defined(__i386__)

__attribute__((target("sse2")))
static double
max_u16_sse2(const uint16_t *data, size_t n)
{
    return max_u16_scalar(data, n);
}

__attribute__((target("sse4.2")))
static double
max_u16_sse42(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m128i vmax = _mm_setzero_si128();
    for (; i + 8 <= n; i += 8)
        vmax = _mm_max_epu16(vmax, _mm_loadu_si128((const __m128i *)(data + i)));
    vmax = _mm_max_epu16(vmax, _mm_srli_si128(vmax, 8));
    vmax = _mm_max_epu16(vmax, _mm_srli_si128(vmax, 4));
    vmax = _mm_max_epu16(vmax, _mm_srli_si128(vmax, 2));
    uint16_t result = (uint16_t)(_mm_extract_epi16(vmax, 0) & 0xFFFF);
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return (double)result;
}

__attribute__((target("avx")))
static double
max_u16_avx(const uint16_t *data, size_t n)
{
    return max_u16_sse42(data, n);
}

__attribute__((target("avx2")))
static double
max_u16_avx2(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m256i vmax = _mm256_setzero_si256();
    for (; i + 16 <= n; i += 16)
        vmax = _mm256_max_epu16(vmax, _mm256_loadu_si256((const __m256i *)(data + i)));
    __m128i lo = _mm256_castsi256_si128(vmax);
    __m128i hi = _mm256_extracti128_si256(vmax, 1);
    __m128i m  = _mm_max_epu16(lo, hi);
    m = _mm_max_epu16(m, _mm_srli_si128(m, 8));
    m = _mm_max_epu16(m, _mm_srli_si128(m, 4));
    m = _mm_max_epu16(m, _mm_srli_si128(m, 2));
    uint16_t result = (uint16_t)(_mm_extract_epi16(m, 0) & 0xFFFF);
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return (double)result;
}

__attribute__((target("avx512f")))
static double
max_u16_avx512f(const uint16_t *data, size_t n)
{
    return max_u16_avx2(data, n);
}

#endif /* x86 */

#if defined(__aarch64__)

static double
max_u16_neon(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    uint16x8_t vmax = vdupq_n_u16(0);
    for (; i + 8 <= n; i += 8)
        vmax = vmaxq_u16(vmax, vld1q_u16(data + i));
    uint16_t result = vmaxvq_u16(vmax);
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return (double)result;
}

__attribute__((target("+sve")))
static double
max_u16_sve(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    svuint16_t vmax = svdup_u16(0);
    svbool_t pg = svwhilelt_b16(i, (uint64_t)n);
    do {
        vmax = svmax_u16_m(pg, vmax, svld1_u16(pg, data + i));
        i += svcnth();
        pg = svwhilelt_b16(i, (uint64_t)n);
    } while (svptest_any(svptrue_b16(), pg));
    return (double)svmaxv_u16(svptrue_b16(), vmax);
}

__attribute__((target("+sve2")))
static double
max_u16_sve2(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    uint64_t vl = svcnth();
    svbool_t ptrue = svptrue_b16();
    svuint16_t vmax0 = svdup_u16(0);
    svuint16_t vmax1 = svdup_u16(0);
    for (; i + 2 * vl <= n; i += 2 * vl) {
        vmax0 = svmax_u16_x(ptrue, vmax0, svld1_u16(ptrue, data + i));
        vmax1 = svmax_u16_x(ptrue, vmax1, svld1_u16(ptrue, data + i + vl));
    }
    svuint16_t vmax = svmaxp_u16_x(ptrue, vmax0, vmax1);
    svbool_t pg = svwhilelt_b16(i, (uint64_t)n);
    while (svptest_any(ptrue, pg)) {
        vmax = svmax_u16_m(pg, vmax, svld1_u16(pg, data + i));
        i += vl;
        pg = svwhilelt_b16(i, (uint64_t)n);
    }
    return (double)svmaxv_u16(ptrue, vmax);
}

#endif /* aarch64 */

max_u16_fn_t
max_u16_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512F: return max_u16_avx512f;
    case SIMD_AVX2:    return max_u16_avx2;
    case SIMD_AVX:     return max_u16_avx;
    case SIMD_SSE4_2:  return max_u16_sse42;
    case SIMD_SSE2:    return max_u16_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return max_u16_sve2;
    case SIMD_SVE:     return max_u16_sve;
    case SIMD_NEON:    return max_u16_neon;
#endif
    case SIMD_SCALAR:
    default:           return max_u16_scalar;
    }
}

static max_u16_fn_t
max_u16_resolver(void)
{
    return max_u16_select(detect_simd_level());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
double max_u16(const uint16_t *data, size_t n)
    __attribute__((ifunc("max_u16_resolver")));
