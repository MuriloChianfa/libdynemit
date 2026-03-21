/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <float.h>
#include <dynemit/max.h>
#include <dynemit/compiler.h>

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
max_f64_scalar(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    double result = data[0];
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 1; i < n; i++)
        if (data[i] > result) result = data[i];
    return result;
}

#if defined(__x86_64__) || defined(__i386__)

__attribute__((target("sse2")))
static double
max_f64_sse2(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m128d vmax = _mm_set1_pd(-DBL_MAX);
    for (; i + 2 <= n; i += 2)
        vmax = _mm_max_pd(vmax, _mm_loadu_pd(data + i));
    __m128d hi = _mm_unpackhi_pd(vmax, vmax);
    vmax = _mm_max_pd(vmax, hi);
    double result = _mm_cvtsd_f64(vmax);
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return result;
}

__attribute__((target("sse4.2")))
static double
max_f64_sse42(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m128d vmax = _mm_set1_pd(-DBL_MAX);
    for (; i + 2 <= n; i += 2)
        vmax = _mm_max_pd(vmax, _mm_loadu_pd(data + i));
    __m128d hi = _mm_unpackhi_pd(vmax, vmax);
    vmax = _mm_max_pd(vmax, hi);
    double result = _mm_cvtsd_f64(vmax);
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return result;
}

__attribute__((target("avx")))
static double
max_f64_avx(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m256d vmax = _mm256_set1_pd(-DBL_MAX);
    for (; i + 4 <= n; i += 4)
        vmax = _mm256_max_pd(vmax, _mm256_loadu_pd(data + i));
    __m128d lo = _mm256_castpd256_pd128(vmax);
    __m128d hi = _mm256_extractf128_pd(vmax, 1);
    __m128d m  = _mm_max_pd(lo, hi);
    __m128d mh = _mm_unpackhi_pd(m, m);
    m = _mm_max_pd(m, mh);
    double result = _mm_cvtsd_f64(m);
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return result;
}

__attribute__((target("avx2")))
static double
max_f64_avx2(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m256d vmax = _mm256_set1_pd(-DBL_MAX);
    for (; i + 4 <= n; i += 4)
        vmax = _mm256_max_pd(vmax, _mm256_loadu_pd(data + i));
    __m128d lo = _mm256_castpd256_pd128(vmax);
    __m128d hi = _mm256_extractf128_pd(vmax, 1);
    __m128d m  = _mm_max_pd(lo, hi);
    __m128d mh = _mm_unpackhi_pd(m, m);
    m = _mm_max_pd(m, mh);
    double result = _mm_cvtsd_f64(m);
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return result;
}

__attribute__((target("avx512f")))
static double
max_f64_avx512f(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m512d vmax = _mm512_set1_pd(-DBL_MAX);
    for (; i + 8 <= n; i += 8)
        vmax = _mm512_max_pd(vmax, _mm512_loadu_pd(data + i));
    double result = _mm512_reduce_max_pd(vmax);
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return result;
}

#endif /* x86 */

#if defined(__aarch64__)

static double
max_f64_neon(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    float64x2_t vmax = vdupq_n_f64(-DBL_MAX);
    for (; i + 2 <= n; i += 2)
        vmax = vmaxq_f64(vmax, vld1q_f64(data + i));
    double result = vgetq_lane_f64(vmax, 0);
    double tmp = vgetq_lane_f64(vmax, 1);
    if (tmp > result) result = tmp;
    for (; i < n; i++)
        if (data[i] > result) result = data[i];
    return result;
}

__attribute__((target("+sve")))
static double
max_f64_sve(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    svfloat64_t vmax = svdup_f64(-DBL_MAX);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    do {
        vmax = svmax_f64_m(pg, vmax, svld1_f64(pg, data + i));
        i += svcntd();
        pg = svwhilelt_b64(i, (uint64_t)n);
    } while (svptest_any(svptrue_b64(), pg));
    return svmaxv_f64(svptrue_b64(), vmax);
}

__attribute__((target("+sve2")))
static double
max_f64_sve2(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    uint64_t vl = svcntd();
    svbool_t ptrue = svptrue_b64();
    svfloat64_t vmax0 = svdup_f64(-DBL_MAX);
    svfloat64_t vmax1 = svdup_f64(-DBL_MAX);
    for (; i + 2 * vl <= n; i += 2 * vl) {
        vmax0 = svmax_f64_x(ptrue, vmax0, svld1_f64(ptrue, data + i));
        vmax1 = svmax_f64_x(ptrue, vmax1, svld1_f64(ptrue, data + i + vl));
    }
    svfloat64_t vmax = svmaxp_f64_x(ptrue, vmax0, vmax1);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    while (svptest_any(ptrue, pg)) {
        vmax = svmax_f64_m(pg, vmax, svld1_f64(pg, data + i));
        i += vl;
        pg = svwhilelt_b64(i, (uint64_t)n);
    }
    return svmaxv_f64(ptrue, vmax);
}

#endif /* aarch64 */

max_f64_fn_t
max_f64_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512F: return max_f64_avx512f;
    case SIMD_AVX2:    return max_f64_avx2;
    case SIMD_AVX:     return max_f64_avx;
    case SIMD_SSE4_2:  return max_f64_sse42;
    case SIMD_SSE2:    return max_f64_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return max_f64_sve2;
    case SIMD_SVE:     return max_f64_sve;
    case SIMD_NEON:    return max_f64_neon;
#endif
    case SIMD_SCALAR:
    default:           return max_f64_scalar;
    }
}

static max_f64_fn_t
max_f64_resolver(void)
{
    return max_f64_select(detect_simd_level());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
double max_f64(const double *data, size_t n)
    __attribute__((ifunc("max_f64_resolver")));
