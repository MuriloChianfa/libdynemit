/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elifdef __aarch64__
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <dynemit/compiler.h>
#include <dynemit/min.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__x86_64__) || defined(__i386__)
DYNEMIT_TARGET_DEFAULT
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
min_u32_scalar(const uint32_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    uint32_t result = data[0];
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 1; i < n; i++) {
        if (data[i] < result) {
            result = data[i];
        }
    }
    return (double)result;
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static double
min_u32_sse2(const uint32_t *data, size_t n)
{
    return min_u32_scalar(data, n);
}

__attribute__((target("sse4.2")))
static double
min_u32_sse42(const uint32_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    size_t i = 0;
    __m128i vmin = _mm_set1_epi32((int)UINT32_MAX);
    for (; i + 4 <= n; i += 4) {
        vmin = _mm_min_epu32(vmin, _mm_loadu_si128((const __m128i *)(data + i)));
    }
    vmin = _mm_min_epu32(vmin, _mm_srli_si128(vmin, 8));
    vmin = _mm_min_epu32(vmin, _mm_srli_si128(vmin, 4));
    uint32_t result = (uint32_t)_mm_extract_epi32(vmin, 0);
    for (; i < n; i++) {
        if (data[i] < result) {
            result = data[i];
        }
    }
    return (double)result;
}

__attribute__((target("avx")))
static double
min_u32_avx(const uint32_t *data, size_t n)
{
    return min_u32_sse42(data, n);
}

__attribute__((target("avx2")))
static double
min_u32_avx2(const uint32_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    size_t i = 0;
    __m256i vmin = _mm256_set1_epi32((int)UINT32_MAX);
    for (; i + 8 <= n; i += 8) {
        vmin = _mm256_min_epu32(vmin, _mm256_loadu_si256((const __m256i *)(data + i)));
    }
    __m128i lo = _mm256_castsi256_si128(vmin);
    __m128i hi = _mm256_extracti128_si256(vmin, 1);
    __m128i m  = _mm_min_epu32(lo, hi);
    m = _mm_min_epu32(m, _mm_srli_si128(m, 8));
    m = _mm_min_epu32(m, _mm_srli_si128(m, 4));
    uint32_t result = (uint32_t)_mm_extract_epi32(m, 0);
    for (; i < n; i++) {
        if (data[i] < result) {
            result = data[i];
        }
    }
    return (double)result;
}

__attribute__((target("avx512f")))
static double
min_u32_avx512f(const uint32_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    size_t i = 0;
    __m512i vmin = _mm512_set1_epi32((int)UINT32_MAX);
    for (; i + 16 <= n; i += 16) {
        vmin = _mm512_min_epu32(vmin, _mm512_loadu_si512(data + i));
    }
    uint32_t result = _mm512_reduce_min_epu32(vmin);
    for (; i < n; i++) {
        if (data[i] < result) {
            result = data[i];
        }
    }
    return (double)result;
}
#endif /* x86 */

#ifdef __aarch64__

static double
min_u32_neon(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    uint32x4_t vmin = vdupq_n_u32(UINT32_MAX);
    for (; i + 4 <= n; i += 4)
        vmin = vminq_u32(vmin, vld1q_u32(data + i));
    uint32_t result = vminvq_u32(vmin);
    for (; i < n; i++)
        if (data[i] < result) result = data[i];
    return (double)result;
}

__attribute__((target("+sve")))
static double
min_u32_sve(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    svuint32_t vmin = svdup_u32(UINT32_MAX);
    svbool_t pg = svwhilelt_b32(i, (uint64_t)n);
    do {
        vmin = svmin_u32_m(pg, vmin, svld1_u32(pg, data + i));
        i += svcntw();
        pg = svwhilelt_b32(i, (uint64_t)n);
    } while (svptest_any(svptrue_b32(), pg));
    return (double)svminv_u32(svptrue_b32(), vmin);
}

__attribute__((target("+sve2")))
static double
min_u32_sve2(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    uint64_t vl = svcntw();
    svbool_t ptrue = svptrue_b32();
    svuint32_t vmin0 = svdup_u32(UINT32_MAX);
    svuint32_t vmin1 = svdup_u32(UINT32_MAX);
    for (; i + 2 * vl <= n; i += 2 * vl) {
        vmin0 = svmin_u32_x(ptrue, vmin0, svld1_u32(ptrue, data + i));
        vmin1 = svmin_u32_x(ptrue, vmin1, svld1_u32(ptrue, data + i + vl));
    }
    svuint32_t vmin = svminp_u32_x(ptrue, vmin0, vmin1);
    svbool_t pg = svwhilelt_b32(i, (uint64_t)n);
    while (svptest_any(ptrue, pg)) {
        vmin = svmin_u32_m(pg, vmin, svld1_u32(pg, data + i));
        i += vl;
        pg = svwhilelt_b32(i, (uint64_t)n);
    }
    return (double)svminv_u32(ptrue, vmin);
}

#endif /* aarch64 */

min_u32_fn_t
min_u32_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return min_u32_avx512f;
    case SIMD_AVX2:    return min_u32_avx2;
    case SIMD_AVX:     return min_u32_avx;
    case SIMD_SSE4_2:  return min_u32_sse42;
    case SIMD_SSE2:    return min_u32_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return min_u32_sve2;
    case SIMD_SVE:     return min_u32_sve;
    case SIMD_NEON:    return min_u32_neon;
#endif
    case SIMD_SCALAR:
    default:           return min_u32_scalar;
}
}

EXPLICIT_RUNTIME_RESOLVER(min_u32_resolver, min_u32_fn_t)
{
    return min_u32_select(detect_simd_level_ts());
}
DYNEMIT_IFUNC_SETUP(min_u32_fn_t, min_u32, min_u32_resolver)

#if defined(DYNEMIT_NO_IFUNC)
double min_u32(const uint32_t *data, size_t n)
{
    return DYNEMIT_IFUNC_INVOKE(min_u32, (data, n));
}
#else
#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
double min_u32(const uint32_t *data, size_t n)
    DYNEMIT_IFUNC_ATTR("min_u32_resolver");
#endif
