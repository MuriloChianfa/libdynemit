/* SPDX-License-Identifier: BSL-1.0 */
#ifdef __aarch64__
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <dynemit/compiler.h>
#include <dynemit/mean.h>
#include <dynemit/sum.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
mean_u32_impl(const uint32_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    return sum_u32(data, n) / (double)n;
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static double mean_u32_sse2(const uint32_t *d, size_t n) { return mean_u32_impl(d, n); }
__attribute__((target("sse4.2")))
static double mean_u32_sse42(const uint32_t *d, size_t n) { return mean_u32_impl(d, n); }
__attribute__((target("avx")))
static double mean_u32_avx(const uint32_t *d, size_t n) { return mean_u32_impl(d, n); }
__attribute__((target("avx2")))
static double mean_u32_avx2(const uint32_t *d, size_t n) { return mean_u32_impl(d, n); }
__attribute__((target("avx512f")))
static double mean_u32_avx512f(const uint32_t *d, size_t n) { return mean_u32_impl(d, n); }
#endif /* x86 */

#ifdef __aarch64__

static double
mean_u32_neon(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    float64x2_t vsum = vdupq_n_f64(0.0);
    for (; i + 4 <= n; i += 4) {
        uint32x4_t vi = vld1q_u32(data + i);
        uint64x2_t lo = vmovl_u32(vget_low_u32(vi));
        uint64x2_t hi = vmovl_u32(vget_high_u32(vi));
        vsum = vaddq_f64(vsum, vcvtq_f64_u64(lo));
        vsum = vaddq_f64(vsum, vcvtq_f64_u64(hi));
    }
    double sum = vgetq_lane_f64(vsum, 0) + vgetq_lane_f64(vsum, 1);
    for (; i < n; i++)
        sum += (double)data[i];
    return sum / (double)n;
}

__attribute__((target("+sve")))
static double
mean_u32_sve(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    svfloat64_t vsum = svdup_f64(0.0);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    do {
        svuint64_t vi = svld1uw_u64(pg, data + i);
        vsum = svadd_f64_m(pg, vsum, svcvt_f64_u64_x(pg, vi));
        i += svcntd();
        pg = svwhilelt_b64(i, (uint64_t)n);
    } while (svptest_any(svptrue_b64(), pg));
    return svaddv_f64(svptrue_b64(), vsum) / (double)n;
}

__attribute__((target("+sve2")))
static double
mean_u32_sve2(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    uint64_t vl = svcntd();
    svbool_t ptrue = svptrue_b64();
    svfloat64_t vsum0 = svdup_f64(0.0);
    svfloat64_t vsum1 = svdup_f64(0.0);
    for (; i + 2 * vl <= n; i += 2 * vl) {
        svuint64_t vi0 = svld1uw_u64(ptrue, data + i);
        svuint64_t vi1 = svld1uw_u64(ptrue, data + i + vl);
        vsum0 = svadd_f64_x(ptrue, vsum0, svcvt_f64_u64_x(ptrue, vi0));
        vsum1 = svadd_f64_x(ptrue, vsum1, svcvt_f64_u64_x(ptrue, vi1));
    }
    svfloat64_t vsum = svaddp_f64_x(ptrue, vsum0, vsum1);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    while (svptest_any(ptrue, pg)) {
        svuint64_t vi = svld1uw_u64(pg, data + i);
        vsum = svadd_f64_m(pg, vsum, svcvt_f64_u64_x(pg, vi));
        i += vl;
        pg = svwhilelt_b64(i, (uint64_t)n);
    }
    return svaddv_f64(ptrue, vsum) / (double)n;
}

#endif /* aarch64 */

mean_u32_fn_t
mean_u32_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return mean_u32_avx512f;
    case SIMD_AVX2:    return mean_u32_avx2;
    case SIMD_AVX:     return mean_u32_avx;
    case SIMD_SSE4_2:  return mean_u32_sse42;
    case SIMD_SSE2:    return mean_u32_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return mean_u32_sve2;
    case SIMD_SVE:     return mean_u32_sve;
    case SIMD_NEON:    return mean_u32_neon;
#endif
    case SIMD_SCALAR:
    default:           return mean_u32_impl;
}
}

EXPLICIT_RUNTIME_RESOLVER(mean_u32_resolver, mean_u32_fn_t)
{
    return mean_u32_select(detect_simd_level_ts());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
double mean_u32(const uint32_t *data, size_t n)
    __attribute__((ifunc("mean_u32_resolver")));
