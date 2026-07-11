/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_FAST_LOG2_H
#define DYNEMIT_FAST_LOG2_H

/*
 * Fast SIMD log2(x) for positive normal doubles.
 *
 * Decompose x = 2^e * m with m in [1,2), reduce to [sqrt(2)/2, sqrt(2)]
 * via conditional halving, then compute log2(m) = s * P(s^2) where
 * s = (m-1)/(m+1) and P is a degree-8 polynomial with coefficients
 * d_i = 2/((2i+1)*ln(2))  (Taylor of 2*atanh / ln(2)).
 *
 * Accuracy of around ~50 bits relative for normal positive inputs.
 * NOT valid for zero, negative, NaN, inf, or subnormals!!!
 */

#include <stdint.h>
#include "mem.h"

#define SIMD_LOG2_LN2   0.69314718055994530941723212145818
#define SIMD_LOG2_D0    (2.0 / ( 1.0 * SIMD_LOG2_LN2))
#define SIMD_LOG2_D1    (2.0 / ( 3.0 * SIMD_LOG2_LN2))
#define SIMD_LOG2_D2    (2.0 / ( 5.0 * SIMD_LOG2_LN2))
#define SIMD_LOG2_D3    (2.0 / ( 7.0 * SIMD_LOG2_LN2))
#define SIMD_LOG2_D4    (2.0 / ( 9.0 * SIMD_LOG2_LN2))
#define SIMD_LOG2_D5    (2.0 / (11.0 * SIMD_LOG2_LN2))
#define SIMD_LOG2_D6    (2.0 / (13.0 * SIMD_LOG2_LN2))
#define SIMD_LOG2_D7    (2.0 / (15.0 * SIMD_LOG2_LN2))
#define SIMD_LOG2_D8    (2.0 / (17.0 * SIMD_LOG2_LN2))
#define SIMD_LOG2_SQRT2 1.4142135623730950488

static inline double
fast_log2_scalar(double x)
{
    uint64_t xi;
    memcpys(&xi, sizeof(xi), &x, sizeof(xi));

    int64_t ei = (int64_t)((xi >> 52) & 0x7FF) - 1023;
    double e = (double)ei;

    uint64_t mi = (xi & 0x000FFFFFFFFFFFFFULL) | 0x3FF0000000000000ULL;
    double m;
    memcpys(&m, sizeof(m), &mi, sizeof(m));

    if (m > SIMD_LOG2_SQRT2) {
        m *= 0.5;
        e += 1.0;
    }

    double f = m - 1.0;
    double s = f / (2.0 + f);
    double u = s * s;

    double p = SIMD_LOG2_D8;
    p = p * u + SIMD_LOG2_D7;
    p = p * u + SIMD_LOG2_D6;
    p = p * u + SIMD_LOG2_D5;
    p = p * u + SIMD_LOG2_D4;
    p = p * u + SIMD_LOG2_D3;
    p = p * u + SIMD_LOG2_D2;
    p = p * u + SIMD_LOG2_D1;
    p = p * u + SIMD_LOG2_D0;

    return e + s * p;
}

#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>

__attribute__((target("sse2"), always_inline))
static inline __m128d
fast_log2_pd_sse2(__m128d x)
{
    const __m128i MANT_MASK = _mm_set1_epi64x(0x000FFFFFFFFFFFFFLL);
    const __m128i EXP_1023  = _mm_set1_epi64x(0x3FF0000000000000LL);
    const __m128i MAGIC_I   = _mm_set1_epi64x(0x4330000000000000LL);
    const __m128d MAGIC_D   = _mm_set1_pd(4503599627370496.0);
    const __m128d BIAS      = _mm_set1_pd(1023.0);
    const __m128d vSQRT2    = _mm_set1_pd(SIMD_LOG2_SQRT2);
    const __m128d HALF      = _mm_set1_pd(0.5);
    const __m128d ONE       = _mm_set1_pd(1.0);
    const __m128d TWO       = _mm_set1_pd(2.0);

    __m128i xi = _mm_castpd_si128(x);

    __m128i ei = _mm_and_si128(_mm_srli_epi64(xi, 52),
                               _mm_set1_epi64x(0x7FF));
    __m128d e  = _mm_sub_pd(
        _mm_sub_pd(_mm_castsi128_pd(_mm_or_si128(ei, MAGIC_I)), MAGIC_D),
        BIAS);

    __m128d m = _mm_castsi128_pd(
        _mm_or_si128(_mm_and_si128(xi, MANT_MASK), EXP_1023));

    __m128d gt = _mm_cmpgt_pd(m, vSQRT2);
    __m128d m_half = _mm_mul_pd(m, HALF);
    __m128d e_inc  = _mm_add_pd(e, ONE);
    m = _mm_or_pd(_mm_andnot_pd(gt, m), _mm_and_pd(gt, m_half));
    e = _mm_or_pd(_mm_andnot_pd(gt, e), _mm_and_pd(gt, e_inc));

    __m128d f = _mm_sub_pd(m, ONE);
    __m128d s = _mm_div_pd(f, _mm_add_pd(TWO, f));
    __m128d u = _mm_mul_pd(s, s);

    __m128d p = _mm_set1_pd(SIMD_LOG2_D8);
    p = _mm_add_pd(_mm_mul_pd(p, u), _mm_set1_pd(SIMD_LOG2_D7));
    p = _mm_add_pd(_mm_mul_pd(p, u), _mm_set1_pd(SIMD_LOG2_D6));
    p = _mm_add_pd(_mm_mul_pd(p, u), _mm_set1_pd(SIMD_LOG2_D5));
    p = _mm_add_pd(_mm_mul_pd(p, u), _mm_set1_pd(SIMD_LOG2_D4));
    p = _mm_add_pd(_mm_mul_pd(p, u), _mm_set1_pd(SIMD_LOG2_D3));
    p = _mm_add_pd(_mm_mul_pd(p, u), _mm_set1_pd(SIMD_LOG2_D2));
    p = _mm_add_pd(_mm_mul_pd(p, u), _mm_set1_pd(SIMD_LOG2_D1));
    p = _mm_add_pd(_mm_mul_pd(p, u), _mm_set1_pd(SIMD_LOG2_D0));

    return _mm_add_pd(e, _mm_mul_pd(s, p));
}

__attribute__((target("avx"), always_inline))
static inline __m256d
fast_log2_pd_avx(__m256d x)
{
    const __m128i MANT_MASK = _mm_set1_epi64x(0x000FFFFFFFFFFFFFLL);
    const __m128i EXP_1023  = _mm_set1_epi64x(0x3FF0000000000000LL);
    const __m128i MAGIC_I   = _mm_set1_epi64x(0x4330000000000000LL);
    const __m128d MAGIC_D   = _mm_set1_pd(4503599627370496.0);
    const __m128d BIAS128   = _mm_set1_pd(1023.0);
    const __m256d vSQRT2    = _mm256_set1_pd(SIMD_LOG2_SQRT2);
    const __m256d NEG_HALF  = _mm256_set1_pd(-0.5);
    const __m256d ONE       = _mm256_set1_pd(1.0);
    const __m256d TWO       = _mm256_set1_pd(2.0);

    __m128i xi_lo = _mm_castpd_si128(_mm256_castpd256_pd128(x));
    __m128i xi_hi = _mm_castpd_si128(_mm256_extractf128_pd(x, 1));

    __m128i ei_lo = _mm_and_si128(_mm_srli_epi64(xi_lo, 52),
                                  _mm_set1_epi64x(0x7FF));
    __m128i ei_hi = _mm_and_si128(_mm_srli_epi64(xi_hi, 52),
                                  _mm_set1_epi64x(0x7FF));
    __m128d e_lo = _mm_sub_pd(
        _mm_sub_pd(_mm_castsi128_pd(_mm_or_si128(ei_lo, MAGIC_I)), MAGIC_D),
        BIAS128);
    __m128d e_hi = _mm_sub_pd(
        _mm_sub_pd(_mm_castsi128_pd(_mm_or_si128(ei_hi, MAGIC_I)), MAGIC_D),
        BIAS128);
    __m256d e = _mm256_set_m128d(e_hi, e_lo);

    __m128d m_lo = _mm_castsi128_pd(
        _mm_or_si128(_mm_and_si128(xi_lo, MANT_MASK), EXP_1023));
    __m128d m_hi = _mm_castsi128_pd(
        _mm_or_si128(_mm_and_si128(xi_hi, MANT_MASK), EXP_1023));
    __m256d m = _mm256_set_m128d(m_hi, m_lo);

    /*
     * Arithmetic range reduction, avoids VBLENDVPD which is a throughput
     * Both AND results and the subsequent add/mul are exact in IEEE 754
     * The asm barrier prevents GCC from pattern-matching
     * the arithmetic back into VBLENDVPD
     */
    __m256d gt = _mm256_cmp_pd(m, vSQRT2, _CMP_GT_OQ);
    __asm__ volatile("" : "+x"(gt));
    m = _mm256_mul_pd(m, _mm256_add_pd(ONE, _mm256_and_pd(gt, NEG_HALF)));
    e = _mm256_add_pd(e, _mm256_and_pd(gt, ONE));

    __m256d f = _mm256_sub_pd(m, ONE);
    __m256d s = _mm256_div_pd(f, _mm256_add_pd(TWO, f));
    __m256d u = _mm256_mul_pd(s, s);

    __m256d p = _mm256_set1_pd(SIMD_LOG2_D8);
    p = _mm256_add_pd(_mm256_mul_pd(p, u), _mm256_set1_pd(SIMD_LOG2_D7));
    p = _mm256_add_pd(_mm256_mul_pd(p, u), _mm256_set1_pd(SIMD_LOG2_D6));
    p = _mm256_add_pd(_mm256_mul_pd(p, u), _mm256_set1_pd(SIMD_LOG2_D5));
    p = _mm256_add_pd(_mm256_mul_pd(p, u), _mm256_set1_pd(SIMD_LOG2_D4));
    p = _mm256_add_pd(_mm256_mul_pd(p, u), _mm256_set1_pd(SIMD_LOG2_D3));
    p = _mm256_add_pd(_mm256_mul_pd(p, u), _mm256_set1_pd(SIMD_LOG2_D2));
    p = _mm256_add_pd(_mm256_mul_pd(p, u), _mm256_set1_pd(SIMD_LOG2_D1));
    p = _mm256_add_pd(_mm256_mul_pd(p, u), _mm256_set1_pd(SIMD_LOG2_D0));

    return _mm256_add_pd(e, _mm256_mul_pd(s, p));
}

__attribute__((target("avx2,fma"), always_inline))
static inline __m256d
fast_log2_pd_avx2_fma(__m256d x)
{
    const __m256i MANT_MASK = _mm256_set1_epi64x(0x000FFFFFFFFFFFFFLL);
    const __m256i EXP_1023  = _mm256_set1_epi64x(0x3FF0000000000000LL);
    const __m256i MAGIC_I   = _mm256_set1_epi64x(0x4330000000000000LL);
    const __m256d MAGIC_D   = _mm256_set1_pd(4503599627370496.0);
    const __m256d BIAS      = _mm256_set1_pd(1023.0);
    const __m256d vSQRT2    = _mm256_set1_pd(SIMD_LOG2_SQRT2);
    const __m256d HALF      = _mm256_set1_pd(0.5);
    const __m256d ONE       = _mm256_set1_pd(1.0);
    const __m256d TWO       = _mm256_set1_pd(2.0);

    __m256i xi = _mm256_castpd_si256(x);

    __m256i ei = _mm256_and_si256(_mm256_srli_epi64(xi, 52),
                                  _mm256_set1_epi64x(0x7FF));
    __m256d e  = _mm256_sub_pd(
        _mm256_sub_pd(_mm256_castsi256_pd(_mm256_or_si256(ei, MAGIC_I)),
                      MAGIC_D),
        BIAS);

    __m256d m = _mm256_castsi256_pd(
        _mm256_or_si256(_mm256_and_si256(xi, MANT_MASK), EXP_1023));

    __m256d gt = _mm256_cmp_pd(m, vSQRT2, _CMP_GT_OQ);
    m = _mm256_blendv_pd(m, _mm256_mul_pd(m, HALF), gt);
    e = _mm256_blendv_pd(e, _mm256_add_pd(e, ONE), gt);

    __m256d f = _mm256_sub_pd(m, ONE);
    __m256d s = _mm256_div_pd(f, _mm256_add_pd(TWO, f));
    __m256d u = _mm256_mul_pd(s, s);

    __m256d p = _mm256_set1_pd(SIMD_LOG2_D8);
    p = _mm256_fmadd_pd(p, u, _mm256_set1_pd(SIMD_LOG2_D7));
    p = _mm256_fmadd_pd(p, u, _mm256_set1_pd(SIMD_LOG2_D6));
    p = _mm256_fmadd_pd(p, u, _mm256_set1_pd(SIMD_LOG2_D5));
    p = _mm256_fmadd_pd(p, u, _mm256_set1_pd(SIMD_LOG2_D4));
    p = _mm256_fmadd_pd(p, u, _mm256_set1_pd(SIMD_LOG2_D3));
    p = _mm256_fmadd_pd(p, u, _mm256_set1_pd(SIMD_LOG2_D2));
    p = _mm256_fmadd_pd(p, u, _mm256_set1_pd(SIMD_LOG2_D1));
    p = _mm256_fmadd_pd(p, u, _mm256_set1_pd(SIMD_LOG2_D0));

    return _mm256_fmadd_pd(s, p, e);
}

__attribute__((target("avx512f"), always_inline))
static inline __m512d
fast_log2_pd_avx512(__m512d x)
{
    const __m512i MANT_MASK = _mm512_set1_epi64(0x000FFFFFFFFFFFFFLL);
    const __m512i EXP_1023  = _mm512_set1_epi64(0x3FF0000000000000LL);
    const __m512i MAGIC_I   = _mm512_set1_epi64(0x4330000000000000LL);
    const __m512d MAGIC_D   = _mm512_set1_pd(4503599627370496.0);
    const __m512d BIAS      = _mm512_set1_pd(1023.0);
    const __m512d vSQRT2    = _mm512_set1_pd(SIMD_LOG2_SQRT2);
    const __m512d HALF      = _mm512_set1_pd(0.5);
    const __m512d ONE       = _mm512_set1_pd(1.0);
    const __m512d TWO       = _mm512_set1_pd(2.0);

    __m512i xi = _mm512_castpd_si512(x);

    __m512i ei = _mm512_and_si512(_mm512_srli_epi64(xi, 52),
                                  _mm512_set1_epi64(0x7FF));
    __m512d e  = _mm512_sub_pd(
        _mm512_sub_pd(_mm512_castsi512_pd(_mm512_or_si512(ei, MAGIC_I)),
                      MAGIC_D),
        BIAS);

    __m512d m = _mm512_castsi512_pd(
        _mm512_or_si512(_mm512_and_si512(xi, MANT_MASK), EXP_1023));

    __mmask8 gt = _mm512_cmp_pd_mask(m, vSQRT2, _CMP_GT_OQ);
    m = _mm512_mask_mul_pd(m, gt, m, HALF);
    e = _mm512_mask_add_pd(e, gt, e, ONE);

    __m512d f = _mm512_sub_pd(m, ONE);
    __m512d s = _mm512_div_pd(f, _mm512_add_pd(TWO, f));
    __m512d u = _mm512_mul_pd(s, s);

    __m512d p = _mm512_set1_pd(SIMD_LOG2_D8);
    p = _mm512_fmadd_pd(p, u, _mm512_set1_pd(SIMD_LOG2_D7));
    p = _mm512_fmadd_pd(p, u, _mm512_set1_pd(SIMD_LOG2_D6));
    p = _mm512_fmadd_pd(p, u, _mm512_set1_pd(SIMD_LOG2_D5));
    p = _mm512_fmadd_pd(p, u, _mm512_set1_pd(SIMD_LOG2_D4));
    p = _mm512_fmadd_pd(p, u, _mm512_set1_pd(SIMD_LOG2_D3));
    p = _mm512_fmadd_pd(p, u, _mm512_set1_pd(SIMD_LOG2_D2));
    p = _mm512_fmadd_pd(p, u, _mm512_set1_pd(SIMD_LOG2_D1));
    p = _mm512_fmadd_pd(p, u, _mm512_set1_pd(SIMD_LOG2_D0));

    return _mm512_fmadd_pd(s, p, e);
}

#endif /* x86 */
#endif /* DYNEMIT_FAST_LOG2_H */
