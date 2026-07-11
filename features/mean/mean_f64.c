/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <dynemit/mean.h>
#include <dynemit/compiler.h>

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
mean_f64_scalar(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    double sum = 0.0;
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++)
        sum += data[i];
    return sum / (double)n;
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static double
mean_f64_sse2(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m128d vsum = _mm_setzero_pd();
    for (; i + 2 <= n; i += 2)
        vsum = _mm_add_pd(vsum, _mm_loadu_pd(data + i));
    __m128d hi = _mm_unpackhi_pd(vsum, vsum);
    vsum = _mm_add_pd(vsum, hi);
    double sum = _mm_cvtsd_f64(vsum);
    for (; i < n; i++)
        sum += data[i];
    return sum / (double)n;
}

__attribute__((target("sse4.2")))
static double
mean_f64_sse42(const double *data, size_t n)
{
    return mean_f64_sse2(data, n);
}

__attribute__((target("avx")))
static double
mean_f64_avx(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m256d vsum = _mm256_setzero_pd();
    for (; i + 4 <= n; i += 4)
        vsum = _mm256_add_pd(vsum, _mm256_loadu_pd(data + i));
    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    __m128d s  = _mm_add_pd(lo, hi);
    __m128d sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double sum = _mm_cvtsd_f64(s);
    for (; i < n; i++)
        sum += data[i];
    return sum / (double)n;
}

__attribute__((target("avx2")))
static double
mean_f64_avx2(const double *data, size_t n)
{
    return mean_f64_avx(data, n);
}

__attribute__((target("avx512f")))
static double
mean_f64_avx512f(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m512d vsum = _mm512_setzero_pd();
    for (; i + 8 <= n; i += 8)
        vsum = _mm512_add_pd(vsum, _mm512_loadu_pd(data + i));
    double sum = _mm512_reduce_add_pd(vsum);
    for (; i < n; i++)
        sum += data[i];
    return sum / (double)n;
}
#endif

#if defined(__aarch64__)

static double
mean_f64_neon(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    float64x2_t vsum = vdupq_n_f64(0.0);
    for (; i + 2 <= n; i += 2)
        vsum = vaddq_f64(vsum, vld1q_f64(data + i));
    double sum = vgetq_lane_f64(vsum, 0) + vgetq_lane_f64(vsum, 1);
    for (; i < n; i++)
        sum += data[i];
    return sum / (double)n;
}

__attribute__((target("+sve")))
static double
mean_f64_sve(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    svfloat64_t vsum = svdup_f64(0.0);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    do {
        vsum = svadd_f64_m(pg, vsum, svld1_f64(pg, data + i));
        i += svcntd();
        pg = svwhilelt_b64(i, (uint64_t)n);
    } while (svptest_any(svptrue_b64(), pg));
    return svaddv_f64(svptrue_b64(), vsum) / (double)n;
}

__attribute__((target("+sve2")))
static double
mean_f64_sve2(const double *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t i = 0;
    uint64_t vl = svcntd();
    svbool_t ptrue = svptrue_b64();
    svfloat64_t vsum0 = svdup_f64(0.0);
    svfloat64_t vsum1 = svdup_f64(0.0);
    for (; i + 2 * vl <= n; i += 2 * vl) {
        vsum0 = svadd_f64_x(ptrue, vsum0, svld1_f64(ptrue, data + i));
        vsum1 = svadd_f64_x(ptrue, vsum1, svld1_f64(ptrue, data + i + vl));
    }
    svfloat64_t vsum = svaddp_f64_x(ptrue, vsum0, vsum1);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    while (svptest_any(ptrue, pg)) {
        vsum = svadd_f64_m(pg, vsum, svld1_f64(pg, data + i));
        i += vl;
        pg = svwhilelt_b64(i, (uint64_t)n);
    }
    return svaddv_f64(ptrue, vsum) / (double)n;
}

#endif /* aarch64 */

mean_f64_fn_t
mean_f64_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return mean_f64_avx512f;
    case SIMD_AVX2:    return mean_f64_avx2;
    case SIMD_AVX:     return mean_f64_avx;
    case SIMD_SSE4_2:  return mean_f64_sse42;
    case SIMD_SSE2:    return mean_f64_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return mean_f64_sve2;
    case SIMD_SVE:     return mean_f64_sve;
    case SIMD_NEON:    return mean_f64_neon;
#endif
    case SIMD_SCALAR:
    default:           return mean_f64_scalar;
    }
}

EXPLICIT_RUNTIME_RESOLVER(mean_f64_resolver, mean_f64_fn_t)
{
    return mean_f64_select(detect_simd_level_ts());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
double mean_f64(const double *data, size_t n)
    __attribute__((ifunc("mean_f64_resolver")));
