/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <dynemit/skewness.h>
#include <dynemit/compiler.h>
#include <dynemit/mean.h>

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
skewness_f64_scalar(const double *data, size_t n)
{
    if (n < 3) return 0.0;
    double m = mean_f64(data, n);
    double sum2 = 0.0, sum3 = 0.0;
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        double d  = data[i] - m;
        double d2 = d * d;
        sum2 += d2;
        sum3 += d2 * d;
    }
    double var = sum2 / (double)n;
    if (var < 1e-30) return 0.0;
    double std = sqrt(var);
    return (sum3 / (double)n) / (std * std * std);
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static double
skewness_f64_sse2(const double *data, size_t n)
{
    return skewness_f64_scalar(data, n);
}

__attribute__((target("sse4.2")))
static double
skewness_f64_sse42(const double *data, size_t n)
{
    return skewness_f64_scalar(data, n);
}

__attribute__((target("avx")))
static double
skewness_f64_avx(const double *data, size_t n)
{
    if (n < 3) return 0.0;
    double m = mean_f64(data, n);
    size_t i = 0;
    __m256d vmean = _mm256_set1_pd(m);
    __m256d vsum2 = _mm256_setzero_pd();
    __m256d vsum3 = _mm256_setzero_pd();
    for (; i + 4 <= n; i += 4) {
        __m256d v  = _mm256_loadu_pd(data + i);
        __m256d d  = _mm256_sub_pd(v, vmean);
        __m256d d2 = _mm256_mul_pd(d, d);
        vsum2 = _mm256_add_pd(vsum2, d2);
        vsum3 = _mm256_add_pd(vsum3, _mm256_mul_pd(d2, d));
    }
    __m128d lo2 = _mm256_castpd256_pd128(vsum2);
    __m128d hi2 = _mm256_extractf128_pd(vsum2, 1);
    __m128d s2  = _mm_add_pd(lo2, hi2);
    __m128d sh2 = _mm_unpackhi_pd(s2, s2);
    s2 = _mm_add_pd(s2, sh2);
    double sum2 = _mm_cvtsd_f64(s2);

    __m128d lo3 = _mm256_castpd256_pd128(vsum3);
    __m128d hi3 = _mm256_extractf128_pd(vsum3, 1);
    __m128d s3  = _mm_add_pd(lo3, hi3);
    __m128d sh3 = _mm_unpackhi_pd(s3, s3);
    s3 = _mm_add_pd(s3, sh3);
    double sum3 = _mm_cvtsd_f64(s3);

    for (; i < n; i++) {
        double d  = data[i] - m;
        double d2 = d * d;
        sum2 += d2;
        sum3 += d2 * d;
    }
    double var = sum2 / (double)n;
    if (var < 1e-30) return 0.0;
    double std_val = sqrt(var);
    return (sum3 / (double)n) / (std_val * std_val * std_val);
}

__attribute__((target("avx2")))
static double
skewness_f64_avx2(const double *data, size_t n)
{
    return skewness_f64_avx(data, n);
}

__attribute__((target("avx512f")))
static double
skewness_f64_avx512f(const double *data, size_t n)
{
    if (n < 3) return 0.0;
    double m = mean_f64(data, n);
    size_t i = 0;
    __m512d vmean = _mm512_set1_pd(m);
    __m512d vsum2 = _mm512_setzero_pd();
    __m512d vsum3 = _mm512_setzero_pd();
    for (; i + 8 <= n; i += 8) {
        __m512d v  = _mm512_loadu_pd(data + i);
        __m512d d  = _mm512_sub_pd(v, vmean);
        __m512d d2 = _mm512_mul_pd(d, d);
        vsum2 = _mm512_add_pd(vsum2, d2);
        vsum3 = _mm512_fmadd_pd(d2, d, vsum3);
    }
    double sum2 = _mm512_reduce_add_pd(vsum2);
    double sum3 = _mm512_reduce_add_pd(vsum3);
    for (; i < n; i++) {
        double d  = data[i] - m;
        double d2 = d * d;
        sum2 += d2;
        sum3 += d2 * d;
    }
    double var = sum2 / (double)n;
    if (var < 1e-30) return 0.0;
    double std_val = sqrt(var);
    return (sum3 / (double)n) / (std_val * std_val * std_val);
}
#endif

#if defined(__aarch64__)

static double
skewness_f64_neon(const double *data, size_t n)
{
    if (n < 3) return 0.0;
    double m = mean_f64(data, n);
    size_t i = 0;
    float64x2_t vmean = vdupq_n_f64(m);
    float64x2_t vsum2 = vdupq_n_f64(0.0);
    float64x2_t vsum3 = vdupq_n_f64(0.0);
    for (; i + 2 <= n; i += 2) {
        float64x2_t v = vld1q_f64(data + i);
        float64x2_t d = vsubq_f64(v, vmean);
        float64x2_t d2 = vmulq_f64(d, d);
        vsum2 = vaddq_f64(vsum2, d2);
        vsum3 = vfmaq_f64(vsum3, d2, d);
    }
    double sum2 = vgetq_lane_f64(vsum2, 0) + vgetq_lane_f64(vsum2, 1);
    double sum3 = vgetq_lane_f64(vsum3, 0) + vgetq_lane_f64(vsum3, 1);
    for (; i < n; i++) {
        double d = data[i] - m;
        double d2 = d * d;
        sum2 += d2;
        sum3 += d2 * d;
    }
    double var = sum2 / (double)n;
    if (var < 1e-30) return 0.0;
    double std_val = sqrt(var);
    return (sum3 / (double)n) / (std_val * std_val * std_val);
}

__attribute__((target("+sve")))
static double
skewness_f64_sve(const double *data, size_t n)
{
    if (n < 3) return 0.0;
    double m = mean_f64(data, n);
    uint64_t i = 0;
    svfloat64_t vmean = svdup_f64(m);
    svfloat64_t vsum2 = svdup_f64(0.0);
    svfloat64_t vsum3 = svdup_f64(0.0);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    do {
        svfloat64_t v = svld1_f64(pg, data + i);
        svfloat64_t d = svsub_f64_x(pg, v, vmean);
        svfloat64_t d2 = svmul_f64_x(pg, d, d);
        vsum2 = svadd_f64_m(pg, vsum2, d2);
        vsum3 = svmla_f64_m(pg, vsum3, d2, d);
        i += svcntd();
        pg = svwhilelt_b64(i, (uint64_t)n);
    } while (svptest_any(svptrue_b64(), pg));
    double sum2 = svaddv_f64(svptrue_b64(), vsum2);
    double sum3 = svaddv_f64(svptrue_b64(), vsum3);
    double var = sum2 / (double)n;
    if (var < 1e-30) return 0.0;
    double std_val = sqrt(var);
    return (sum3 / (double)n) / (std_val * std_val * std_val);
}

__attribute__((target("+sve2")))
static double
skewness_f64_sve2(const double *data, size_t n)
{
    if (n < 3) return 0.0;
    double m = mean_f64(data, n);
    uint64_t i = 0;
    uint64_t vl = svcntd();
    svbool_t ptrue = svptrue_b64();
    svfloat64_t vmean = svdup_f64(m);
    svfloat64_t vsum2_0 = svdup_f64(0.0);
    svfloat64_t vsum2_1 = svdup_f64(0.0);
    svfloat64_t vsum3_0 = svdup_f64(0.0);
    svfloat64_t vsum3_1 = svdup_f64(0.0);
    for (; i + 2 * vl <= n; i += 2 * vl) {
        svfloat64_t v0 = svld1_f64(ptrue, data + i);
        svfloat64_t d0 = svsub_f64_x(ptrue, v0, vmean);
        svfloat64_t d2_0 = svmul_f64_x(ptrue, d0, d0);
        vsum2_0 = svadd_f64_x(ptrue, vsum2_0, d2_0);
        vsum3_0 = svmla_f64_x(ptrue, vsum3_0, d2_0, d0);
        svfloat64_t v1 = svld1_f64(ptrue, data + i + vl);
        svfloat64_t d1 = svsub_f64_x(ptrue, v1, vmean);
        svfloat64_t d2_1 = svmul_f64_x(ptrue, d1, d1);
        vsum2_1 = svadd_f64_x(ptrue, vsum2_1, d2_1);
        vsum3_1 = svmla_f64_x(ptrue, vsum3_1, d2_1, d1);
    }
    svfloat64_t vsum2 = svaddp_f64_x(ptrue, vsum2_0, vsum2_1);
    svfloat64_t vsum3 = svaddp_f64_x(ptrue, vsum3_0, vsum3_1);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    while (svptest_any(ptrue, pg)) {
        svfloat64_t v = svld1_f64(pg, data + i);
        svfloat64_t d = svsub_f64_x(pg, v, vmean);
        svfloat64_t d2 = svmul_f64_x(pg, d, d);
        vsum2 = svadd_f64_m(pg, vsum2, d2);
        vsum3 = svmla_f64_m(pg, vsum3, d2, d);
        i += vl;
        pg = svwhilelt_b64(i, (uint64_t)n);
    }
    double sum2 = svaddv_f64(ptrue, vsum2);
    double sum3 = svaddv_f64(ptrue, vsum3);
    double var = sum2 / (double)n;
    if (var < 1e-30) return 0.0;
    double std_val = sqrt(var);
    return (sum3 / (double)n) / (std_val * std_val * std_val);
}

#endif /* aarch64 */

skewness_f64_fn_t
skewness_f64_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return skewness_f64_avx512f;
    case SIMD_AVX2:    return skewness_f64_avx2;
    case SIMD_AVX:     return skewness_f64_avx;
    case SIMD_SSE4_2:  return skewness_f64_sse42;
    case SIMD_SSE2:    return skewness_f64_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return skewness_f64_sve2;
    case SIMD_SVE:     return skewness_f64_sve;
    case SIMD_NEON:    return skewness_f64_neon;
#endif
    case SIMD_SCALAR:
    default:           return skewness_f64_scalar;
    }
}

static skewness_f64_fn_t
skewness_f64_resolver(void)
{
    return skewness_f64_select(detect_simd_level());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
double skewness_f64(const double *data, size_t n)
    __attribute__((ifunc("skewness_f64_resolver")));
