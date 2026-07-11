/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elifdef __aarch64__
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <dynemit/compiler.h>
#include <dynemit/sum.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__x86_64__) || defined(__i386__)
DYNEMIT_TARGET_DEFAULT
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
sum_u64_scalar(const uint64_t *data, size_t n)
{
    double sum = 0.0;
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        sum += (double)data[i];
    }
    return sum;
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static double
sum_u64_sse2(const uint64_t *data, size_t n)
{
    return sum_u64_scalar(data, n);
}

__attribute__((target("sse4.2")))
static double
sum_u64_sse42(const uint64_t *data, size_t n)
{
    return sum_u64_scalar(data, n);
}

__attribute__((target("avx")))
static double
sum_u64_avx(const uint64_t *data, size_t n)
{
    return sum_u64_scalar(data, n);
}

__attribute__((target("avx2")))
static double
sum_u64_avx2(const uint64_t *data, size_t n)
{
    return sum_u64_scalar(data, n);
}

__attribute__((target("avx512f")))
static double
sum_u64_avx512f(const uint64_t *data, size_t n)
{
    size_t i = 0;
    __m512i vsum = _mm512_setzero_si512();
    for (; i + 8 <= n; i += 8) {
        vsum = _mm512_add_epi64(vsum, _mm512_loadu_si512(data + i));
    }
    alignas(64) uint64_t tmp[8];
    _mm512_storeu_si512(tmp, vsum);
    double sum = 0.0;
    for (int j = 0; j < 8; j++) {
        sum += (double)tmp[j];
    }
    for (; i < n; i++) {
        sum += (double)data[i];
    }
    return sum;
}
#endif

#ifdef __aarch64__

static double
sum_u64_neon(const uint64_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    size_t i = 0;
    float64x2_t vsum = vdupq_n_f64(0.0);
    for (; i + 2 <= n; i += 2) {
        uint64x2_t vi = vld1q_u64(data + i);
        vsum = vaddq_f64(vsum, vcvtq_f64_u64(vi));
    }
    double sum = vgetq_lane_f64(vsum, 0) + vgetq_lane_f64(vsum, 1);
    for (; i < n; i++)
        sum += (double)data[i];
    return sum;
}

__attribute__((target("+sve")))
static double
sum_u64_sve(const uint64_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    uint64_t i = 0;
    svfloat64_t vsum = svdup_f64(0.0);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    do {
        svuint64_t vi = svld1_u64(pg, data + i);
        vsum = svadd_f64_m(pg, vsum, svcvt_f64_u64_x(pg, vi));
        i += svcntd();
        pg = svwhilelt_b64(i, (uint64_t)n);
    } while (svptest_any(svptrue_b64(), pg));
    return svaddv_f64(svptrue_b64(), vsum);
}

__attribute__((target("+sve2")))
static double
sum_u64_sve2(const uint64_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    uint64_t i = 0;
    uint64_t vl = svcntd();
    svbool_t ptrue = svptrue_b64();
    svfloat64_t vsum0 = svdup_f64(0.0);
    svfloat64_t vsum1 = svdup_f64(0.0);
    for (; i + 2 * vl <= n; i += 2 * vl) {
        svuint64_t vi0 = svld1_u64(ptrue, data + i);
        svuint64_t vi1 = svld1_u64(ptrue, data + i + vl);
        vsum0 = svadd_f64_x(ptrue, vsum0, svcvt_f64_u64_x(ptrue, vi0));
        vsum1 = svadd_f64_x(ptrue, vsum1, svcvt_f64_u64_x(ptrue, vi1));
    }
    svfloat64_t vsum = svaddp_f64_x(ptrue, vsum0, vsum1);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    while (svptest_any(ptrue, pg)) {
        svuint64_t vi = svld1_u64(pg, data + i);
        vsum = svadd_f64_m(pg, vsum, svcvt_f64_u64_x(pg, vi));
        i += vl;
        pg = svwhilelt_b64(i, (uint64_t)n);
    }
    return svaddv_f64(ptrue, vsum);
}

#endif /* aarch64 */

sum_u64_fn_t
sum_u64_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return sum_u64_avx512f;
    case SIMD_AVX2:    return sum_u64_avx2;
    case SIMD_AVX:     return sum_u64_avx;
    case SIMD_SSE4_2:  return sum_u64_sse42;
    case SIMD_SSE2:    return sum_u64_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return sum_u64_sve2;
    case SIMD_SVE:     return sum_u64_sve;
    case SIMD_NEON:    return sum_u64_neon;
#endif
    case SIMD_SCALAR:
    default:           return sum_u64_scalar;
}
}

EXPLICIT_RUNTIME_RESOLVER(sum_u64_resolver, sum_u64_fn_t)
{
    return sum_u64_select(detect_simd_level_ts());
}
DYNEMIT_IFUNC_SETUP(sum_u64_fn_t, sum_u64, sum_u64_resolver)

#if defined(DYNEMIT_NO_IFUNC)
double sum_u64(const uint64_t *data, size_t n)
{
    return DYNEMIT_IFUNC_INVOKE(sum_u64, (data, n));
}
#else
#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
double sum_u64(const uint64_t *data, size_t n)
    DYNEMIT_IFUNC_ATTR("sum_u64_resolver");
#endif
