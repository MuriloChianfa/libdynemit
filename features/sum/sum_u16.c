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
sum_u16_scalar(const uint16_t *data, size_t n)
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
sum_u16_sse2(const uint16_t *data, size_t n)
{
    size_t i = 0;
    __m128d vsum = _mm_setzero_pd();
    for (; i + 8 <= n; i += 8) {
        __m128i v16 = _mm_loadu_si128((const __m128i *)(data + i));
        __m128i lo32 = _mm_unpacklo_epi16(v16, _mm_setzero_si128());
        __m128i hi32 = _mm_unpackhi_epi16(v16, _mm_setzero_si128());
        __m128d d0 = _mm_cvtepi32_pd(lo32);
        __m128d d1 = _mm_cvtepi32_pd(_mm_srli_si128(lo32, 8));
        __m128d d2 = _mm_cvtepi32_pd(hi32);
        __m128d d3 = _mm_cvtepi32_pd(_mm_srli_si128(hi32, 8));
        vsum = _mm_add_pd(vsum, d0);
        vsum = _mm_add_pd(vsum, d1);
        vsum = _mm_add_pd(vsum, d2);
        vsum = _mm_add_pd(vsum, d3);
    }
    __m128d h = _mm_unpackhi_pd(vsum, vsum);
    vsum = _mm_add_pd(vsum, h);
    double sum = _mm_cvtsd_f64(vsum);
    for (; i < n; i++) {
        sum += (double)data[i];
    }
    return sum;
}

__attribute__((target("sse4.2")))
static double
sum_u16_sse42(const uint16_t *data, size_t n)
{
    size_t i = 0;
    __m128d vsum = _mm_setzero_pd();
    for (; i + 8 <= n; i += 8) {
        __m128i v16 = _mm_loadu_si128((const __m128i *)(data + i));
        __m128i lo32 = _mm_cvtepu16_epi32(v16);
        __m128i hi32 = _mm_cvtepu16_epi32(_mm_srli_si128(v16, 8));
        __m128d d0 = _mm_cvtepi32_pd(lo32);
        __m128d d1 = _mm_cvtepi32_pd(_mm_srli_si128(lo32, 8));
        __m128d d2 = _mm_cvtepi32_pd(hi32);
        __m128d d3 = _mm_cvtepi32_pd(_mm_srli_si128(hi32, 8));
        vsum = _mm_add_pd(vsum, d0);
        vsum = _mm_add_pd(vsum, d1);
        vsum = _mm_add_pd(vsum, d2);
        vsum = _mm_add_pd(vsum, d3);
    }
    __m128d h = _mm_unpackhi_pd(vsum, vsum);
    vsum = _mm_add_pd(vsum, h);
    double sum = _mm_cvtsd_f64(vsum);
    for (; i < n; i++) {
        sum += (double)data[i];
    }
    return sum;
}

__attribute__((target("avx")))
static double
sum_u16_avx(const uint16_t *data, size_t n)
{
    return sum_u16_sse42(data, n);
}

__attribute__((target("avx2")))
static double
sum_u16_avx2(const uint16_t *data, size_t n)
{
    size_t i = 0;
    __m256d vsum = _mm256_setzero_pd();
    for (; i + 16 <= n; i += 16) {
        __m256i v16 = _mm256_loadu_si256((const __m256i *)(data + i));
        __m128i lo16 = _mm256_castsi256_si128(v16);
        __m128i hi16 = _mm256_extracti128_si256(v16, 1);
        __m256i lo32 = _mm256_cvtepu16_epi32(lo16);
        __m256i hi32 = _mm256_cvtepu16_epi32(hi16);
        __m128i a = _mm256_castsi256_si128(lo32);
        __m128i b = _mm256_extracti128_si256(lo32, 1);
        __m128i c = _mm256_castsi256_si128(hi32);
        __m128i d = _mm256_extracti128_si256(hi32, 1);
        vsum = _mm256_add_pd(vsum, _mm256_cvtepi32_pd(a));
        vsum = _mm256_add_pd(vsum, _mm256_cvtepi32_pd(b));
        vsum = _mm256_add_pd(vsum, _mm256_cvtepi32_pd(c));
        vsum = _mm256_add_pd(vsum, _mm256_cvtepi32_pd(d));
    }
    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    __m128d s  = _mm_add_pd(lo, hi);
    __m128d sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double sum = _mm_cvtsd_f64(s);
    for (; i < n; i++) {
        sum += (double)data[i];
    }
    return sum;
}

__attribute__((target("avx512f")))
static double
sum_u16_avx512f(const uint16_t *data, size_t n)
{
    return sum_u16_avx2(data, n);
}
#endif /* x86 */

#ifdef __aarch64__

static double
sum_u16_neon(const uint16_t *data, size_t n)
{
    size_t i = 0;
    uint64x2_t vsum = vdupq_n_u64(0);
    for (; i + 8 <= n; i += 8) {
        uint16x8_t vi = vld1q_u16(data + i);
        uint32x4_t wide32 = vpaddlq_u16(vi);
        uint64x2_t wide64 = vpaddlq_u32(wide32);
        vsum = vaddq_u64(vsum, wide64);
    }
    double sum = (double)(vgetq_lane_u64(vsum, 0) + vgetq_lane_u64(vsum, 1));
    for (; i < n; i++)
        sum += (double)data[i];
    return sum;
}

__attribute__((target("+sve")))
static double
sum_u16_sve(const uint16_t *data, size_t n)
{
    uint64_t i = 0;
    svfloat64_t vsum = svdup_f64(0.0);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    do {
        svuint64_t vi = svld1uh_u64(pg, data + i);
        vsum = svadd_f64_m(pg, vsum, svcvt_f64_u64_x(pg, vi));
        i += svcntd();
        pg = svwhilelt_b64(i, (uint64_t)n);
    } while (svptest_any(svptrue_b64(), pg));
    return svaddv_f64(svptrue_b64(), vsum);
}

__attribute__((target("+sve2")))
static double
sum_u16_sve2(const uint16_t *data, size_t n)
{
    uint64_t i = 0;
    uint64_t vl = svcntd();
    svbool_t ptrue = svptrue_b64();
    svfloat64_t vsum0 = svdup_f64(0.0);
    svfloat64_t vsum1 = svdup_f64(0.0);
    for (; i + 2 * vl <= n; i += 2 * vl) {
        svuint64_t vi0 = svld1uh_u64(ptrue, data + i);
        svuint64_t vi1 = svld1uh_u64(ptrue, data + i + vl);
        vsum0 = svadd_f64_x(ptrue, vsum0, svcvt_f64_u64_x(ptrue, vi0));
        vsum1 = svadd_f64_x(ptrue, vsum1, svcvt_f64_u64_x(ptrue, vi1));
    }
    svfloat64_t vsum = svaddp_f64_x(ptrue, vsum0, vsum1);
    svbool_t pg = svwhilelt_b64(i, (uint64_t)n);
    while (svptest_any(ptrue, pg)) {
        svuint64_t vi = svld1uh_u64(pg, data + i);
        vsum = svadd_f64_m(pg, vsum, svcvt_f64_u64_x(pg, vi));
        i += vl;
        pg = svwhilelt_b64(i, (uint64_t)n);
    }
    return svaddv_f64(ptrue, vsum);
}

#endif /* aarch64 */

sum_u16_fn_t
sum_u16_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return sum_u16_avx512f;
    case SIMD_AVX2:    return sum_u16_avx2;
    case SIMD_AVX:     return sum_u16_avx;
    case SIMD_SSE4_2:  return sum_u16_sse42;
    case SIMD_SSE2:    return sum_u16_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return sum_u16_sve2;
    case SIMD_SVE:     return sum_u16_sve;
    case SIMD_NEON:    return sum_u16_neon;
#endif
    case SIMD_SCALAR:
    default:           return sum_u16_scalar;
}
}

EXPLICIT_RUNTIME_RESOLVER(sum_u16_resolver, sum_u16_fn_t)
{
    return sum_u16_select(detect_simd_level_ts());
}
DYNEMIT_IFUNC_SETUP(sum_u16_fn_t, sum_u16, sum_u16_resolver)

#if defined(DYNEMIT_NO_IFUNC)
double sum_u16(const uint16_t *data, size_t n)
{
    return DYNEMIT_IFUNC_INVOKE(sum_u16, (data, n));
}
#else
#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
double sum_u16(const uint16_t *data, size_t n)
    DYNEMIT_IFUNC_ATTR("sum_u16_resolver");
#endif
