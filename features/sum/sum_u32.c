/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <dynemit/sum.h>
#include <dynemit/compiler.h>

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
sum_u32_scalar(const uint32_t *data, size_t n)
{
    double sum = 0.0;
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++)
        sum += (double)data[i];
    return sum;
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static double
sum_u32_sse2(const uint32_t *data, size_t n)
{
    size_t i = 0;
    __m128d vsum = _mm_setzero_pd();
    for (; i + 4 <= n; i += 4) {
        __m128i vi = _mm_loadu_si128((const __m128i *)(data + i));
        __m128d lo = _mm_cvtepi32_pd(vi);
        __m128d hi = _mm_cvtepi32_pd(_mm_srli_si128(vi, 8));
        vsum = _mm_add_pd(vsum, lo);
        vsum = _mm_add_pd(vsum, hi);
    }
    __m128d h = _mm_unpackhi_pd(vsum, vsum);
    vsum = _mm_add_pd(vsum, h);
    double sum = _mm_cvtsd_f64(vsum);
    for (; i < n; i++)
        sum += (double)data[i];
    return sum;
}

__attribute__((target("sse4.2")))
static double
sum_u32_sse42(const uint32_t *data, size_t n)
{
    return sum_u32_sse2(data, n);
}

__attribute__((target("avx")))
static double
sum_u32_avx(const uint32_t *data, size_t n)
{
    size_t i = 0;
    __m256d vsum = _mm256_setzero_pd();
    for (; i + 4 <= n; i += 4) {
        __m128i vi = _mm_loadu_si128((const __m128i *)(data + i));
        __m256d vd = _mm256_cvtepi32_pd(vi);
        vsum = _mm256_add_pd(vsum, vd);
    }
    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    __m128d s  = _mm_add_pd(lo, hi);
    __m128d sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double sum = _mm_cvtsd_f64(s);
    for (; i < n; i++)
        sum += (double)data[i];
    return sum;
}

__attribute__((target("avx2")))
static double
sum_u32_avx2(const uint32_t *data, size_t n)
{
    size_t i = 0;
    __m256d vsum = _mm256_setzero_pd();
    for (; i + 8 <= n; i += 8) {
        __m256i vi = _mm256_loadu_si256((const __m256i *)(data + i));
        __m128i lo_i = _mm256_castsi256_si128(vi);
        __m128i hi_i = _mm256_extracti128_si256(vi, 1);
        __m256d lo_d = _mm256_cvtepi32_pd(lo_i);
        __m256d hi_d = _mm256_cvtepi32_pd(hi_i);
        vsum = _mm256_add_pd(vsum, lo_d);
        vsum = _mm256_add_pd(vsum, hi_d);
    }
    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    __m128d s  = _mm_add_pd(lo, hi);
    __m128d sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double sum = _mm_cvtsd_f64(s);
    for (; i < n; i++)
        sum += (double)data[i];
    return sum;
}

__attribute__((target("avx512f")))
static double
sum_u32_avx512f(const uint32_t *data, size_t n)
{
    size_t i = 0;
    __m512d vsum = _mm512_setzero_pd();
    for (; i + 16 <= n; i += 16) {
        __m512i vi  = _mm512_loadu_si512(data + i);
        __m256i lo  = _mm512_castsi512_si256(vi);
        __m256i hi  = _mm512_extracti64x4_epi64(vi, 1);
        __m512d ld  = _mm512_cvtepu32_pd(lo);
        __m512d hd  = _mm512_cvtepu32_pd(hi);
        vsum = _mm512_add_pd(vsum, ld);
        vsum = _mm512_add_pd(vsum, hd);
    }
    double sum = _mm512_reduce_add_pd(vsum);
    for (; i < n; i++)
        sum += (double)data[i];
    return sum;
}
#endif /* x86 */

#if defined(__aarch64__)

static double
sum_u32_neon(const uint32_t *data, size_t n)
{
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
    return sum;
}

__attribute__((target("+sve")))
static double
sum_u32_sve(const uint32_t *data, size_t n)
{
    uint64_t i = 0;
    svfloat64_t vsum = svdup_f64(0.0);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    do {
        svuint64_t vi = svld1uw_u64(pg, data + i);
        vsum = svadd_f64_m(pg, vsum, svcvt_f64_u64_x(pg, vi));
        i += svcntd();
        pg = svwhilelt_b64(i, (uint64_t)n);
    } while (svptest_any(svptrue_b64(), pg));
    return svaddv_f64(svptrue_b64(), vsum);
}

__attribute__((target("+sve2")))
static double
sum_u32_sve2(const uint32_t *data, size_t n)
{
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
    return svaddv_f64(ptrue, vsum);
}

#endif /* aarch64 */

sum_u32_fn_t
sum_u32_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return sum_u32_avx512f;
    case SIMD_AVX2:    return sum_u32_avx2;
    case SIMD_AVX:     return sum_u32_avx;
    case SIMD_SSE4_2:  return sum_u32_sse42;
    case SIMD_SSE2:    return sum_u32_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return sum_u32_sve2;
    case SIMD_SVE:     return sum_u32_sve;
    case SIMD_NEON:    return sum_u32_neon;
#endif
    case SIMD_SCALAR:
    default:           return sum_u32_scalar;
    }
}

EXPLICIT_RUNTIME_RESOLVER(sum_u32_resolver, sum_u32_fn_t)
{
    return sum_u32_select(detect_simd_level_ts());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
double sum_u32(const uint32_t *data, size_t n)
    __attribute__((ifunc("sum_u32_resolver")));
