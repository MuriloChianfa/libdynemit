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
max_u32_scalar(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint32_t result = data[0];
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 1; i < n; i++)
        if (data[i] > result) result = data[i];
    return (double)result;
}

#if defined(__x86_64__) || defined(__i386__)

__attribute__((target("sse2")))
static double
max_u32_sse2(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    const __m128i bias = _mm_set1_epi32((int)0x80000000u);

    __m128i b0 = bias, b1 = bias;

    for (; i + 8 <= n; i += 8) {
        __m128i v0 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(data + i)),     bias);
        __m128i v1 = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(data + i + 4)), bias);

        __m128i c0 = _mm_cmpgt_epi32(v0, b0);
        __m128i c1 = _mm_cmpgt_epi32(v1, b1);

        b0 = _mm_or_si128(_mm_and_si128(c0, v0), _mm_andnot_si128(c0, b0));
        b1 = _mm_or_si128(_mm_and_si128(c1, v1), _mm_andnot_si128(c1, b1));
    }

    __m128i cf = _mm_cmpgt_epi32(b0, b1);
    __m128i bmax = _mm_or_si128(_mm_and_si128(cf, b0), _mm_andnot_si128(cf, b1));

    for (; i + 4 <= n; i += 4) {
        __m128i v = _mm_xor_si128(_mm_loadu_si128((const __m128i *)(data + i)), bias);
        __m128i c = _mm_cmpgt_epi32(v, bmax);
        bmax = _mm_or_si128(_mm_and_si128(c, v), _mm_andnot_si128(c, bmax));
    }

    __m128i t = _mm_srli_si128(bmax, 8);
    __m128i c = _mm_cmpgt_epi32(bmax, t);
    bmax = _mm_or_si128(_mm_and_si128(c, bmax), _mm_andnot_si128(c, t));
    t = _mm_srli_si128(bmax, 4);
    c = _mm_cmpgt_epi32(bmax, t);
    bmax = _mm_or_si128(_mm_and_si128(c, bmax), _mm_andnot_si128(c, t));

    uint32_t result = (uint32_t)_mm_cvtsi128_si32(_mm_xor_si128(bmax, bias));

    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return (double)result;
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
    if (n == 0) return 0.0;
    size_t i = 0;
    __m128i vmax = _mm_setzero_si128();
    for (; i + 8 <= n; i += 8) {
        __m256i wide = _mm256_loadu_si256((const __m256i *)(data + i));
        __m128i lo = _mm256_castsi256_si128(wide);
        __m128i hi = _mm256_extractf128_si256(wide, 1);
        vmax = _mm_max_epu32(vmax, _mm_max_epu32(lo, hi));
    }
    for (; i + 4 <= n; i += 4)
        vmax = _mm_max_epu32(vmax, _mm_loadu_si128((const __m128i *)(data + i)));
    vmax = _mm_max_epu32(vmax, _mm_srli_si128(vmax, 8));
    vmax = _mm_max_epu32(vmax, _mm_srli_si128(vmax, 4));
    uint32_t result = (uint32_t)_mm_extract_epi32(vmax, 0);
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return (double)result;
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

#endif /* x86 */

#if defined(__aarch64__)

static double
max_u32_neon(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    uint32x4_t vmax = vdupq_n_u32(0);
    for (; i + 4 <= n; i += 4)
        vmax = vmaxq_u32(vmax, vld1q_u32(data + i));
    uint32_t result = vmaxvq_u32(vmax);
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return (double)result;
}

__attribute__((target("+sve")))
static double
max_u32_sve(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    svuint32_t vmax = svdup_u32(0);
    svbool_t pg = svwhilelt_b32(i, (uint64_t)n);
    do {
        vmax = svmax_u32_m(pg, vmax, svld1_u32(pg, data + i));
        i += svcntw();
        pg = svwhilelt_b32(i, (uint64_t)n);
    } while (svptest_any(svptrue_b32(), pg));
    return (double)svmaxv_u32(svptrue_b32(), vmax);
}

__attribute__((target("+sve2")))
static double
max_u32_sve2(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    uint64_t vl = svcntw();
    svbool_t ptrue = svptrue_b32();
    svuint32_t vmax0 = svdup_u32(0);
    svuint32_t vmax1 = svdup_u32(0);
    for (; i + 2 * vl <= n; i += 2 * vl) {
        vmax0 = svmax_u32_x(ptrue, vmax0, svld1_u32(ptrue, data + i));
        vmax1 = svmax_u32_x(ptrue, vmax1, svld1_u32(ptrue, data + i + vl));
    }
    svuint32_t vmax = svmaxp_u32_x(ptrue, vmax0, vmax1);
    svbool_t pg = svwhilelt_b32(i, (uint64_t)n);
    while (svptest_any(ptrue, pg)) {
        vmax = svmax_u32_m(pg, vmax, svld1_u32(pg, data + i));
        i += vl;
        pg = svwhilelt_b32(i, (uint64_t)n);
    }
    return (double)svmaxv_u32(ptrue, vmax);
}

#endif /* aarch64 */

max_u32_fn_t
max_u32_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return max_u32_avx512f;
    case SIMD_AVX2:    return max_u32_avx2;
    case SIMD_AVX:     return max_u32_avx;
    case SIMD_SSE4_2:  return max_u32_sse42;
    case SIMD_SSE2:    return max_u32_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return max_u32_sve2;
    case SIMD_SVE:     return max_u32_sve;
    case SIMD_NEON:    return max_u32_neon;
#endif
    case SIMD_SCALAR:
    default:           return max_u32_scalar;
    }
}

EXPLICIT_RUNTIME_RESOLVER(max_u32_resolver, max_u32_fn_t)
{
    return max_u32_select(detect_simd_level_ts());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
double max_u32(const uint32_t *data, size_t n)
    __attribute__((ifunc("max_u32_resolver")));
