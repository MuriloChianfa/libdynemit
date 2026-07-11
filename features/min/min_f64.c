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
#include <dynemit/min.h>
#include <dynemit/compiler.h>

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
min_f64_scalar(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    double result = data[0];
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 1; i < n; i++)
        if (data[i] < result) result = data[i];
    return result;
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static double
min_f64_sse2(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m128d vmin = _mm_set1_pd(DBL_MAX);
    for (; i + 2 <= n; i += 2)
        vmin = _mm_min_pd(vmin, _mm_loadu_pd(data + i));
    __m128d hi = _mm_unpackhi_pd(vmin, vmin);
    vmin = _mm_min_pd(vmin, hi);
    double result = _mm_cvtsd_f64(vmin);
    for (; i < n; i++)
        if (data[i] < result) result = data[i];
    return result;
}

__attribute__((target("sse4.2")))
static double
min_f64_sse42(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m128d vmin = _mm_set1_pd(DBL_MAX);
    for (; i + 2 <= n; i += 2)
        vmin = _mm_min_pd(vmin, _mm_loadu_pd(data + i));
    __m128d hi = _mm_unpackhi_pd(vmin, vmin);
    vmin = _mm_min_pd(vmin, hi);
    double result = _mm_cvtsd_f64(vmin);
    for (; i < n; i++)
        if (data[i] < result) result = data[i];
    return result;
}

__attribute__((target("avx")))
static double
min_f64_avx(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m256d vmin = _mm256_set1_pd(DBL_MAX);
    for (; i + 4 <= n; i += 4)
        vmin = _mm256_min_pd(vmin, _mm256_loadu_pd(data + i));
    __m128d lo = _mm256_castpd256_pd128(vmin);
    __m128d hi = _mm256_extractf128_pd(vmin, 1);
    __m128d m  = _mm_min_pd(lo, hi);
    __m128d mh = _mm_unpackhi_pd(m, m);
    m = _mm_min_pd(m, mh);
    double result = _mm_cvtsd_f64(m);
    for (; i < n; i++)
        if (data[i] < result) result = data[i];
    return result;
}

__attribute__((target("avx2")))
static double
min_f64_avx2(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m256d vmin = _mm256_set1_pd(DBL_MAX);
    for (; i + 4 <= n; i += 4)
        vmin = _mm256_min_pd(vmin, _mm256_loadu_pd(data + i));
    __m128d lo = _mm256_castpd256_pd128(vmin);
    __m128d hi = _mm256_extractf128_pd(vmin, 1);
    __m128d m  = _mm_min_pd(lo, hi);
    __m128d mh = _mm_unpackhi_pd(m, m);
    m = _mm_min_pd(m, mh);
    double result = _mm_cvtsd_f64(m);
    for (; i < n; i++)
        if (data[i] < result) result = data[i];
    return result;
}

__attribute__((target("avx512f")))
static double
min_f64_avx512f(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m512d vmin = _mm512_set1_pd(DBL_MAX);
    for (; i + 8 <= n; i += 8)
        vmin = _mm512_min_pd(vmin, _mm512_loadu_pd(data + i));
    double result = _mm512_reduce_min_pd(vmin);
    for (; i < n; i++)
        if (data[i] < result) result = data[i];
    return result;
}
#endif

#if defined(__aarch64__)

static double
min_f64_neon(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    float64x2_t vmin = vdupq_n_f64(DBL_MAX);
    for (; i + 2 <= n; i += 2)
        vmin = vminq_f64(vmin, vld1q_f64(data + i));
    double result = vgetq_lane_f64(vmin, 0);
    double tmp = vgetq_lane_f64(vmin, 1);
    if (tmp < result) result = tmp;
    for (; i < n; i++)
        if (data[i] < result) result = data[i];
    return result;
}

__attribute__((target("+sve")))
static double
min_f64_sve(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    svfloat64_t vmin = svdup_f64(DBL_MAX);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    do {
        vmin = svmin_f64_m(pg, vmin, svld1_f64(pg, data + i));
        i += svcntd();
        pg = svwhilelt_b64(i, (uint64_t)n);
    } while (svptest_any(svptrue_b64(), pg));
    return svminv_f64(svptrue_b64(), vmin);
}

__attribute__((target("+sve2")))
static double
min_f64_sve2(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    uint64_t vl = svcntd();
    svbool_t ptrue = svptrue_b64();
    svfloat64_t vmin0 = svdup_f64(DBL_MAX);
    svfloat64_t vmin1 = svdup_f64(DBL_MAX);
    for (; i + 2 * vl <= n; i += 2 * vl) {
        vmin0 = svmin_f64_x(ptrue, vmin0, svld1_f64(ptrue, data + i));
        vmin1 = svmin_f64_x(ptrue, vmin1, svld1_f64(ptrue, data + i + vl));
    }
    svfloat64_t vmin = svminp_f64_x(ptrue, vmin0, vmin1);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    while (svptest_any(ptrue, pg)) {
        vmin = svmin_f64_m(pg, vmin, svld1_f64(pg, data + i));
        i += vl;
        pg = svwhilelt_b64(i, (uint64_t)n);
    }
    return svminv_f64(ptrue, vmin);
}

#endif /* aarch64 */

min_f64_fn_t
min_f64_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return min_f64_avx512f;
    case SIMD_AVX2:    return min_f64_avx2;
    case SIMD_AVX:     return min_f64_avx;
    case SIMD_SSE4_2:  return min_f64_sse42;
    case SIMD_SSE2:    return min_f64_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return min_f64_sve2;
    case SIMD_SVE:     return min_f64_sve;
    case SIMD_NEON:    return min_f64_neon;
#endif
    case SIMD_SCALAR:
    default:           return min_f64_scalar;
    }
}

EXPLICIT_RUNTIME_RESOLVER(min_f64_resolver, min_f64_fn_t)
{
    return min_f64_select(detect_simd_level_ts());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
double min_f64(const double *data, size_t n)
    __attribute__((ifunc("min_f64_resolver")));
