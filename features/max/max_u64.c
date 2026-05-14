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
max_u64_scalar(const uint64_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t result = data[0];
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 1; i < n; i++)
        if (data[i] > result) result = data[i];
    return (double)result;
}

#if defined(__x86_64__) || defined(__i386__)

__attribute__((target("sse2")))
static double
max_u64_sse2(const uint64_t *data, size_t n)
{
    return max_u64_scalar(data, n);
}

__attribute__((target("sse4.2")))
static double
max_u64_sse42(const uint64_t *data, size_t n)
{
    return max_u64_scalar(data, n);
}

__attribute__((target("avx")))
static double
max_u64_avx(const uint64_t *data, size_t n)
{
    return max_u64_scalar(data, n);
}

__attribute__((target("avx2")))
static double
max_u64_avx2(const uint64_t *data, size_t n)
{
    return max_u64_scalar(data, n);
}

__attribute__((target("avx512f")))
static double
max_u64_avx512f(const uint64_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m512i vmax = _mm512_setzero_si512();
    for (; i + 8 <= n; i += 8)
        vmax = _mm512_max_epu64(vmax, _mm512_loadu_si512(data + i));
    uint64_t result = _mm512_reduce_max_epu64(vmax);
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return (double)result;
}

#endif /* x86 */

#if defined(__aarch64__)

static double
max_u64_neon(const uint64_t *data, size_t n)
{
    return max_u64_scalar(data, n);
}

__attribute__((target("+sve")))
static double
max_u64_sve(const uint64_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    svuint64_t vmax = svdup_u64(0);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    do {
        vmax = svmax_u64_m(pg, vmax, svld1_u64(pg, data + i));
        i += svcntd();
        pg = svwhilelt_b64(i, (uint64_t)n);
    } while (svptest_any(svptrue_b64(), pg));
    return (double)svmaxv_u64(svptrue_b64(), vmax);
}

__attribute__((target("+sve2")))
static double
max_u64_sve2(const uint64_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    uint64_t vl = svcntd();
    svbool_t ptrue = svptrue_b64();
    svuint64_t vmax0 = svdup_u64(0);
    svuint64_t vmax1 = svdup_u64(0);
    for (; i + 2 * vl <= n; i += 2 * vl) {
        vmax0 = svmax_u64_x(ptrue, vmax0, svld1_u64(ptrue, data + i));
        vmax1 = svmax_u64_x(ptrue, vmax1, svld1_u64(ptrue, data + i + vl));
    }
    svuint64_t vmax = svmaxp_u64_x(ptrue, vmax0, vmax1);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    while (svptest_any(ptrue, pg)) {
        vmax = svmax_u64_m(pg, vmax, svld1_u64(pg, data + i));
        i += vl;
        pg = svwhilelt_b64(i, (uint64_t)n);
    }
    return (double)svmaxv_u64(ptrue, vmax);
}

#endif /* aarch64 */

max_u64_fn_t
max_u64_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return max_u64_avx512f;
    case SIMD_AVX2:    return max_u64_avx2;
    case SIMD_AVX:     return max_u64_avx;
    case SIMD_SSE4_2:  return max_u64_sse42;
    case SIMD_SSE2:    return max_u64_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return max_u64_sve2;
    case SIMD_SVE:     return max_u64_sve;
    case SIMD_NEON:    return max_u64_neon;
#endif
    case SIMD_SCALAR:
    default:           return max_u64_scalar;
    }
}

static max_u64_fn_t
max_u64_resolver(void)
{
    return max_u64_select(detect_simd_level());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
double max_u64(const uint64_t *data, size_t n)
    __attribute__((ifunc("max_u64_resolver")));
