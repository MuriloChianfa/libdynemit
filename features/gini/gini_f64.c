/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elifdef __aarch64__
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <dynemit/compiler.h>
#include <dynemit/gini.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/*
 * Gini coefficient for pre-sorted ascending f64 data.
 * G = (2 * sum(i * x_i) for i=1..n) / (n * sum(x_i)) - (n+1)/n
 * Result clamped to [0, 1].
 */

#if defined(__x86_64__) || defined(__i386__)
DYNEMIT_TARGET_DEFAULT
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
gini_f64_scalar(const double *sorted_data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    double weighted_sum = 0.0;
    double total_sum = 0.0;
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        weighted_sum += (double)(i + 1) * sorted_data[i];
        total_sum += sorted_data[i];
    }
    if (total_sum == 0.0) {
        return 0.0;
    }
    double g = ((2.0 * weighted_sum) / ((double)n * total_sum)) - ((double)(n + 1) / (double)n);
    if (g < 0.0) {
        g = 0.0;
    }
    if (g > 1.0) {
        g = 1.0;
    }
    return g;
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static double
gini_f64_sse2(const double *sorted_data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    size_t i = 0;
    __m128d vwsum = _mm_setzero_pd();
    __m128d vtsum = _mm_setzero_pd();
    __m128d vidx = _mm_set_pd(2.0, 1.0);
    __m128d vstep = _mm_set1_pd(2.0);
    for (; i + 2 <= n; i += 2) {
        __m128d vdata = _mm_loadu_pd(sorted_data + i);
        vwsum = _mm_add_pd(vwsum, _mm_mul_pd(vidx, vdata));
        vtsum = _mm_add_pd(vtsum, vdata);
        vidx = _mm_add_pd(vidx, vstep);
    }
    __m128d hi;
    hi = _mm_unpackhi_pd(vwsum, vwsum);
    vwsum = _mm_add_pd(vwsum, hi);
    double weighted_sum = _mm_cvtsd_f64(vwsum);
    hi = _mm_unpackhi_pd(vtsum, vtsum);
    vtsum = _mm_add_pd(vtsum, hi);
    double total_sum = _mm_cvtsd_f64(vtsum);
    for (; i < n; i++) {
        weighted_sum += (double)(i + 1) * sorted_data[i];
        total_sum += sorted_data[i];
    }
    if (total_sum == 0.0) {
        return 0.0;
    }
    double g = ((2.0 * weighted_sum) / ((double)n * total_sum)) - ((double)(n + 1) / (double)n);
    if (g < 0.0) {
        g = 0.0;
    }
    if (g > 1.0) {
        g = 1.0;
    }
    return g;
}

__attribute__((target("sse4.2")))
static double
gini_f64_sse42(const double *sorted_data, size_t n)
{
    return gini_f64_sse2(sorted_data, n);
}

__attribute__((target("avx")))
static double
gini_f64_avx(const double *sorted_data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    size_t i = 0;
    __m256d vwsum = _mm256_setzero_pd();
    __m256d vtsum = _mm256_setzero_pd();
    __m256d vidx = _mm256_set_pd(4.0, 3.0, 2.0, 1.0);
    __m256d vstep = _mm256_set1_pd(4.0);
    for (; i + 4 <= n; i += 4) {
        __m256d vdata = _mm256_loadu_pd(sorted_data + i);
        vwsum = _mm256_add_pd(vwsum, _mm256_mul_pd(vidx, vdata));
        vtsum = _mm256_add_pd(vtsum, vdata);
        vidx = _mm256_add_pd(vidx, vstep);
    }
    /* Horizontal reduction */
    __m128d lo;
    __m128d hi;
    __m128d s;
    __m128d sh;
    lo = _mm256_castpd256_pd128(vwsum);
    hi = _mm256_extractf128_pd(vwsum, 1);
    s = _mm_add_pd(lo, hi);
    sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double weighted_sum = _mm_cvtsd_f64(s);
    lo = _mm256_castpd256_pd128(vtsum);
    hi = _mm256_extractf128_pd(vtsum, 1);
    s = _mm_add_pd(lo, hi);
    sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double total_sum = _mm_cvtsd_f64(s);
    for (; i < n; i++) {
        weighted_sum += (double)(i + 1) * sorted_data[i];
        total_sum += sorted_data[i];
    }
    if (total_sum == 0.0) {
        return 0.0;
    }
    double g = ((2.0 * weighted_sum) / ((double)n * total_sum)) - ((double)(n + 1) / (double)n);
    if (g < 0.0) {
        g = 0.0;
    }
    if (g > 1.0) {
        g = 1.0;
    }
    return g;
}

__attribute__((target("avx2")))
static double
gini_f64_avx2(const double *sorted_data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    size_t i = 0;
    __m256d vwsum = _mm256_setzero_pd();
    __m256d vtsum = _mm256_setzero_pd();
    __m256d vidx = _mm256_set_pd(4.0, 3.0, 2.0, 1.0);
    __m256d vstep = _mm256_set1_pd(4.0);
    for (; i + 4 <= n; i += 4) {
        __m256d vdata = _mm256_loadu_pd(sorted_data + i);
        vwsum = _mm256_add_pd(vwsum, _mm256_mul_pd(vidx, vdata));
        vtsum = _mm256_add_pd(vtsum, vdata);
        vidx = _mm256_add_pd(vidx, vstep);
    }
    __m128d lo;
    __m128d hi;
    __m128d s;
    __m128d sh;
    lo = _mm256_castpd256_pd128(vwsum);
    hi = _mm256_extractf128_pd(vwsum, 1);
    s = _mm_add_pd(lo, hi);
    sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double weighted_sum = _mm_cvtsd_f64(s);
    lo = _mm256_castpd256_pd128(vtsum);
    hi = _mm256_extractf128_pd(vtsum, 1);
    s = _mm_add_pd(lo, hi);
    sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double total_sum = _mm_cvtsd_f64(s);
    for (; i < n; i++) {
        weighted_sum += (double)(i + 1) * sorted_data[i];
        total_sum += sorted_data[i];
    }
    if (total_sum == 0.0) {
        return 0.0;
    }
    double g = ((2.0 * weighted_sum) / ((double)n * total_sum)) - ((double)(n + 1) / (double)n);
    if (g < 0.0) {
        g = 0.0;
    }
    if (g > 1.0) {
        g = 1.0;
    }
    return g;
}

__attribute__((target("avx512f")))
static double
gini_f64_avx512f(const double *sorted_data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    size_t i = 0;
    __m512d vwsum = _mm512_setzero_pd();
    __m512d vtsum = _mm512_setzero_pd();
    __m512d vidx = _mm512_set_pd(8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0);
    __m512d vstep = _mm512_set1_pd(8.0);
    for (; i + 8 <= n; i += 8) {
        __m512d vdata = _mm512_loadu_pd(sorted_data + i);
        vwsum = _mm512_fmadd_pd(vidx, vdata, vwsum);
        vtsum = _mm512_add_pd(vtsum, vdata);
        vidx = _mm512_add_pd(vidx, vstep);
    }
    double weighted_sum = _mm512_reduce_add_pd(vwsum);
    double total_sum = _mm512_reduce_add_pd(vtsum);
    for (; i < n; i++) {
        weighted_sum += (double)(i + 1) * sorted_data[i];
        total_sum += sorted_data[i];
    }
    if (total_sum == 0.0) {
        return 0.0;
    }
    double g = ((2.0 * weighted_sum) / ((double)n * total_sum)) - ((double)(n + 1) / (double)n);
    if (g < 0.0) {
        g = 0.0;
    }
    if (g > 1.0) {
        g = 1.0;
    }
    return g;
}
#endif

#ifdef __aarch64__

static double
gini_f64_neon(const double *sorted_data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    float64x2_t vwsum = vdupq_n_f64(0.0);
    float64x2_t vtsum = vdupq_n_f64(0.0);
    float64x2_t vidx = (float64x2_t){1.0, 2.0};
    float64x2_t vstep = vdupq_n_f64(2.0);
    for (; i + 2 <= n; i += 2) {
        float64x2_t vdata = vld1q_f64(sorted_data + i);
        vwsum = vfmaq_f64(vwsum, vidx, vdata);
        vtsum = vaddq_f64(vtsum, vdata);
        vidx = vaddq_f64(vidx, vstep);
    }
    double weighted_sum = vgetq_lane_f64(vwsum, 0) + vgetq_lane_f64(vwsum, 1);
    double total_sum = vgetq_lane_f64(vtsum, 0) + vgetq_lane_f64(vtsum, 1);
    for (; i < n; i++) {
        weighted_sum += (double)(i + 1) * sorted_data[i];
        total_sum += sorted_data[i];
    }
    if (total_sum == 0.0) return 0.0;
    double g = (2.0 * weighted_sum) / ((double)n * total_sum) - ((double)(n + 1) / (double)n);
    if (g < 0.0) g = 0.0;
    if (g > 1.0) g = 1.0;
    return g;
}

__attribute__((target("+sve")))
static double
gini_f64_sve(const double *sorted_data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    uint64_t vl = svcntd();
    svfloat64_t vwsum = svdup_f64(0.0);
    svfloat64_t vtsum = svdup_f64(0.0);
    svfloat64_t vidx = svcvt_f64_u64_x(svptrue_b64(), svindex_u64(1, 1));
    svfloat64_t vstep = svdup_f64((double)vl);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    do {
        svfloat64_t vdata = svld1_f64(pg, sorted_data + i);
        vwsum = svmla_f64_m(pg, vwsum, vidx, vdata);
        vtsum = svadd_f64_m(pg, vtsum, vdata);
        vidx = svadd_f64_x(pg, vidx, vstep);
        i += vl;
        pg = svwhilelt_b64(i, (uint64_t)n);
    } while (svptest_any(svptrue_b64(), pg));
    double weighted_sum = svaddv_f64(svptrue_b64(), vwsum);
    double total_sum = svaddv_f64(svptrue_b64(), vtsum);
    if (total_sum == 0.0) return 0.0;
    double g = (2.0 * weighted_sum) / ((double)n * total_sum) - ((double)(n + 1) / (double)n);
    if (g < 0.0) g = 0.0;
    if (g > 1.0) g = 1.0;
    return g;
}

__attribute__((target("+sve2")))
static double
gini_f64_sve2(const double *sorted_data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    uint64_t vl = svcntd();
    svbool_t ptrue = svptrue_b64();
    svfloat64_t vwsum0 = svdup_f64(0.0);
    svfloat64_t vwsum1 = svdup_f64(0.0);
    svfloat64_t vtsum0 = svdup_f64(0.0);
    svfloat64_t vtsum1 = svdup_f64(0.0);
    svfloat64_t vidx0 = svcvt_f64_u64_x(ptrue, svindex_u64(1, 1));
    svfloat64_t vidx1 = svcvt_f64_u64_x(ptrue, svindex_u64(vl + 1, 1));
    svfloat64_t vstep = svdup_f64((double)(2 * vl));
    for (; i + 2 * vl <= n; i += 2 * vl) {
        svfloat64_t vdata0 = svld1_f64(ptrue, sorted_data + i);
        svfloat64_t vdata1 = svld1_f64(ptrue, sorted_data + i + vl);
        vwsum0 = svmla_f64_x(ptrue, vwsum0, vidx0, vdata0);
        vwsum1 = svmla_f64_x(ptrue, vwsum1, vidx1, vdata1);
        vtsum0 = svadd_f64_x(ptrue, vtsum0, vdata0);
        vtsum1 = svadd_f64_x(ptrue, vtsum1, vdata1);
        vidx0 = svadd_f64_x(ptrue, vidx0, vstep);
        vidx1 = svadd_f64_x(ptrue, vidx1, vstep);
    }
    svfloat64_t vwsum = svaddp_f64_x(ptrue, vwsum0, vwsum1);
    svfloat64_t vtsum = svaddp_f64_x(ptrue, vtsum0, vtsum1);
    svfloat64_t vidx = svcvt_f64_u64_x(ptrue, svindex_u64(i + 1, 1));
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    while (svptest_any(ptrue, pg)) {
        svfloat64_t vdata = svld1_f64(pg, sorted_data + i);
        vwsum = svmla_f64_m(pg, vwsum, vidx, vdata);
        vtsum = svadd_f64_m(pg, vtsum, vdata);
        i += vl;
        vidx = svcvt_f64_u64_x(ptrue, svindex_u64(i + 1, 1));
        pg = svwhilelt_b64(i, (uint64_t)n);
    }
    double weighted_sum = svaddv_f64(ptrue, vwsum);
    double total_sum = svaddv_f64(ptrue, vtsum);
    if (total_sum == 0.0) return 0.0;
    double g = (2.0 * weighted_sum) / ((double)n * total_sum) - ((double)(n + 1) / (double)n);
    if (g < 0.0) g = 0.0;
    if (g > 1.0) g = 1.0;
    return g;
}

#endif /* aarch64 */

gini_f64_fn_t
gini_f64_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return gini_f64_avx512f;
    case SIMD_AVX2:    return gini_f64_avx2;
    case SIMD_AVX:     return gini_f64_avx;
    case SIMD_SSE4_2:  return gini_f64_sse42;
    case SIMD_SSE2:    return gini_f64_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return gini_f64_sve2;
    case SIMD_SVE:     return gini_f64_sve;
    case SIMD_NEON:    return gini_f64_neon;
#endif
    case SIMD_SCALAR:
    default:           return gini_f64_scalar;
}
}

EXPLICIT_RUNTIME_RESOLVER(gini_f64_resolver, gini_f64_fn_t)
{
    return gini_f64_select(detect_simd_level_ts());
}
DYNEMIT_IFUNC_SETUP(gini_f64_fn_t, gini_f64, gini_f64_resolver)

#if defined(DYNEMIT_NO_IFUNC)
double gini_f64(const double *sorted_data, size_t n)
{
    return DYNEMIT_IFUNC_INVOKE(gini_f64, (sorted_data, n));
}
#else
#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
double gini_f64(const double *sorted_data, size_t n)
    DYNEMIT_IFUNC_ATTR("gini_f64_resolver");
#endif
