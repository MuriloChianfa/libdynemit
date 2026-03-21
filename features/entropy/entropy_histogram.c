/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#include "fast_log2.h"
#elif defined(__aarch64__)
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <dynemit/entropy.h>
#include <dynemit/compiler.h>

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
entropy_histogram_scalar(const uint64_t *counts, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t total = 0;
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++)
        total += counts[i];
    if (total == 0) return 0.0;
    double inv_total = 1.0 / (double)total;
    double h = 0.0;
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        if (counts[i] == 0) continue;
        double p = (double)counts[i] * inv_total;
        h -= p * fast_log2_scalar(p);
    }
    return h;
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static double
entropy_histogram_sse2(const uint64_t *counts, size_t n)
{
    if (n == 0) return 0.0;

    size_t i = 0;
    __m128i vsum_i = _mm_setzero_si128();
    for (; i + 2 <= n; i += 2)
        vsum_i = _mm_add_epi64(vsum_i,
                    _mm_loadu_si128((const __m128i *)(counts + i)));
    alignas(16) uint64_t tmp[2];
    _mm_store_si128((__m128i *)tmp, vsum_i);
    uint64_t total = tmp[0] + tmp[1];
    for (; i < n; i++)
        total += counts[i];
    if (total == 0) return 0.0;

    double inv_total = 1.0 / (double)total;
    double h = 0.0;
    for (i = 0; i < n; i++) {
        if (counts[i] == 0) continue;
        double p = (double)counts[i] * inv_total;
        h -= p * fast_log2_scalar(p);
    }
    return h;
}

__attribute__((target("sse4.2")))
static double
entropy_histogram_sse42(const uint64_t *counts, size_t n)
{
    return entropy_histogram_sse2(counts, n);
}

__attribute__((target("avx")))
static double
entropy_histogram_avx(const uint64_t *counts, size_t n)
{
    if (n == 0) return 0.0;

    size_t i = 0;
    __m128i vsum_i = _mm_setzero_si128();
    for (; i + 2 <= n; i += 2)
        vsum_i = _mm_add_epi64(vsum_i,
                    _mm_loadu_si128((const __m128i *)(counts + i)));
    alignas(16) uint64_t tmp[2];
    _mm_store_si128((__m128i *)tmp, vsum_i);
    uint64_t total = tmp[0] + tmp[1];
    for (; i < n; i++)
        total += counts[i];
    if (total == 0) return 0.0;

    double inv_total = 1.0 / (double)total;
    __m256d vinv = _mm256_set1_pd(inv_total);
    __m256d vsum = _mm256_setzero_pd();

    i = 0;
    for (; i + 4 <= n; i += 4) {
        __m256d cd = _mm256_set_pd(
            (double)counts[i + 3], (double)counts[i + 2],
            (double)counts[i + 1], (double)counts[i]);
        __m256d nonzero = _mm256_cmp_pd(cd, _mm256_setzero_pd(), _CMP_NEQ_OQ);
        __m256d p = _mm256_mul_pd(cd, vinv);
        __m256d safe_p = _mm256_blendv_pd(_mm256_set1_pd(1.0), p, nonzero);
        __m256d l2 = fast_log2_pd_avx(safe_p);
        __m256d contrib = _mm256_and_pd(_mm256_mul_pd(p, l2), nonzero);
        vsum = _mm256_sub_pd(vsum, contrib);
    }

    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    lo = _mm_add_pd(lo, hi);
    lo = _mm_add_pd(lo, _mm_unpackhi_pd(lo, lo));
    double h = _mm_cvtsd_f64(lo);

    for (; i < n; i++) {
        if (counts[i] == 0) continue;
        double p = (double)counts[i] * inv_total;
        h -= p * fast_log2_scalar(p);
    }
    return h;
}

__attribute__((target("avx2,fma")))
static double
entropy_histogram_avx2(const uint64_t *counts, size_t n)
{
    if (n == 0) return 0.0;

    size_t i = 0;
    __m256i vsum_i = _mm256_setzero_si256();
    for (; i + 4 <= n; i += 4)
        vsum_i = _mm256_add_epi64(vsum_i,
                    _mm256_loadu_si256((const __m256i *)(counts + i)));
    alignas(32) uint64_t tmp[4];
    _mm256_storeu_si256((__m256i *)tmp, vsum_i);
    uint64_t total = tmp[0] + tmp[1] + tmp[2] + tmp[3];
    for (; i < n; i++)
        total += counts[i];
    if (total == 0) return 0.0;

    double inv_total = 1.0 / (double)total;
    __m256d vinv = _mm256_set1_pd(inv_total);
    __m256d vsum = _mm256_setzero_pd();

    i = 0;
    for (; i + 4 <= n; i += 4) {
        __m256d cd = _mm256_set_pd(
            (double)counts[i + 3], (double)counts[i + 2],
            (double)counts[i + 1], (double)counts[i]);
        __m256d nonzero = _mm256_cmp_pd(cd, _mm256_setzero_pd(), _CMP_NEQ_OQ);
        __m256d p = _mm256_mul_pd(cd, vinv);
        __m256d safe_p = _mm256_blendv_pd(_mm256_set1_pd(1.0), p, nonzero);
        __m256d l2 = fast_log2_pd_avx2_fma(safe_p);
        __m256d contrib = _mm256_and_pd(_mm256_mul_pd(p, l2), nonzero);
        vsum = _mm256_sub_pd(vsum, contrib);
    }

    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    lo = _mm_add_pd(lo, hi);
    lo = _mm_add_pd(lo, _mm_unpackhi_pd(lo, lo));
    double h = _mm_cvtsd_f64(lo);

    for (; i < n; i++) {
        if (counts[i] == 0) continue;
        double p = (double)counts[i] * inv_total;
        h -= p * fast_log2_scalar(p);
    }
    return h;
}

__attribute__((target("avx512f")))
static double
entropy_histogram_avx512f(const uint64_t *counts, size_t n)
{
    if (n == 0) return 0.0;

    size_t i = 0;
    __m512i vsum_i = _mm512_setzero_si512();
    for (; i + 8 <= n; i += 8)
        vsum_i = _mm512_add_epi64(vsum_i, _mm512_loadu_si512(counts + i));
    alignas(64) uint64_t tmp[8];
    _mm512_storeu_si512(tmp, vsum_i);
    uint64_t total = 0;
    for (size_t j = 0; j < 8; j++)
        total += tmp[j];
    for (; i < n; i++)
        total += counts[i];
    if (total == 0) return 0.0;

    double inv_total = 1.0 / (double)total;
    __m512d vinv = _mm512_set1_pd(inv_total);
    __m512d vsum = _mm512_setzero_pd();

    i = 0;
    for (; i + 8 <= n; i += 8) {
        /* uint64 -> double: scalar pack (no direct SIMD cvt in AVX-512F) */
        __m512d cd = _mm512_set_pd(
            (double)counts[i + 7], (double)counts[i + 6],
            (double)counts[i + 5], (double)counts[i + 4],
            (double)counts[i + 3], (double)counts[i + 2],
            (double)counts[i + 1], (double)counts[i]);
        __mmask8 nz = _mm512_cmp_pd_mask(cd, _mm512_setzero_pd(), _CMP_NEQ_OQ);
        __m512d p = _mm512_mul_pd(cd, vinv);
        __m512d safe_p = _mm512_mask_blend_pd(nz, _mm512_set1_pd(1.0), p);
        __m512d l2 = fast_log2_pd_avx512(safe_p);
        __m512d contrib = _mm512_maskz_mul_pd(nz, p, l2);
        vsum = _mm512_sub_pd(vsum, contrib);
    }

    __m256d lo4 = _mm512_castpd512_pd256(vsum);
    __m256d hi4 = _mm512_extractf64x4_pd(vsum, 1);
    __m256d s4  = _mm256_add_pd(lo4, hi4);
    __m128d lo2 = _mm256_castpd256_pd128(s4);
    __m128d hi2 = _mm256_extractf128_pd(s4, 1);
    __m128d s2  = _mm_add_pd(lo2, hi2);
    s2 = _mm_add_pd(s2, _mm_unpackhi_pd(s2, s2));
    double h = _mm_cvtsd_f64(s2);

    for (; i < n; i++) {
        if (counts[i] == 0) continue;
        double p = (double)counts[i] * inv_total;
        h -= p * fast_log2_scalar(p);
    }
    return h;
}
#endif

#if defined(__aarch64__)

static double
entropy_histogram_neon(const uint64_t *counts, size_t n)
{
    return entropy_histogram_scalar(counts, n);
}

__attribute__((target("+sve")))
static double
entropy_histogram_sve(const uint64_t *counts, size_t n)
{
    return entropy_histogram_scalar(counts, n);
}

__attribute__((target("+sve2")))
static double
entropy_histogram_sve2(const uint64_t *counts, size_t n)
{
    return entropy_histogram_scalar(counts, n);
}

#endif /* aarch64 */

entropy_histogram_fn_t
entropy_histogram_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512F: return entropy_histogram_avx512f;
    case SIMD_AVX2:    return entropy_histogram_avx2;
    case SIMD_AVX:     return entropy_histogram_avx;
    case SIMD_SSE4_2:  return entropy_histogram_sse42;
    case SIMD_SSE2:    return entropy_histogram_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return entropy_histogram_sve2;
    case SIMD_SVE:     return entropy_histogram_sve;
    case SIMD_NEON:    return entropy_histogram_neon;
#endif
    case SIMD_SCALAR:
    default:           return entropy_histogram_scalar;
    }
}

static entropy_histogram_fn_t
entropy_histogram_resolver(void)
{
    return entropy_histogram_select(detect_simd_level());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
double entropy_histogram(const uint64_t *counts, size_t n)
    __attribute__((ifunc("entropy_histogram_resolver")));
