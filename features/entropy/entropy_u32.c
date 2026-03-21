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
#include <stdlib.h>
#include <string.h>
#include <dynemit/compiler.h>
#include <dynemit/entropy.h>

static int
cmp_u32(const void *a, const void *b)
{
    uint32_t va = *(const uint32_t *)a;
    uint32_t vb = *(const uint32_t *)b;
    return (va > vb) - (va < vb);
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
entropy_u32_scalar(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint32_t *sorted = malloc(n * sizeof(uint32_t));
    if (!sorted) return 0.0;
    memcpy(sorted, data, n * sizeof(uint32_t));
    qsort(sorted, n, sizeof(uint32_t), cmp_u32);

    size_t cap = 256;
    uint64_t *cnts = malloc(cap * sizeof(uint64_t));
    if (!cnts) { free(sorted); return 0.0; }
    size_t num_cnts = 0;
    uint64_t run = 1;
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 1; i < n; i++) {
        if (sorted[i] == sorted[i - 1]) {
            run++;
        } else {
            if (num_cnts >= cap) {
                cap *= 2;
                uint64_t *tmp = realloc(cnts, cap * sizeof(uint64_t));
                if (!tmp) { free(cnts); free(sorted); return 0.0; }
                cnts = tmp;
            }
            cnts[num_cnts++] = run;
            run = 1;
        }
    }
    if (num_cnts >= cap) {
        cap *= 2;
        uint64_t *tmp = realloc(cnts, cap * sizeof(uint64_t));
        if (!tmp) { free(cnts); free(sorted); return 0.0; }
        cnts = tmp;
    }
    cnts[num_cnts++] = run;

    double h = entropy_histogram(cnts, num_cnts);
    free(cnts);
    free(sorted);
    return h;
}


#if defined(__x86_64__) || defined(__i386__)

/*
 * Hash-table-based entropy for u32: O(n) expected vs O(n log n) sort.
 * Open-addressing with multiplicative hash, power-of-2 capacity, load factor < 50%.
 */

#define HT_MULTIPLIER 2654435769u

static size_t
ht_build(const uint32_t *data, size_t n,
         uint32_t *ht_keys, uint32_t *ht_counts,
         size_t cap)
{
    unsigned shift = 32u - (unsigned)__builtin_ctz((unsigned)cap);
    uint32_t mask = (uint32_t)(cap - 1);
    size_t ndistinct = 0;

    for (size_t i = 0; i < n; i++) {
        uint32_t val = data[i];
        uint32_t h = (val * HT_MULTIPLIER) >> shift;
        while (ht_counts[h] != 0 && ht_keys[h] != val)
            h = (h + 1) & mask;
        if (ht_counts[h] == 0) {
            ht_keys[h] = val;
            ndistinct++;
        }
        ht_counts[h]++;
    }
    return ndistinct;
}

static size_t
ht_compact_counts(uint32_t *ht_counts, size_t cap)
{
    size_t ndirty = 0;
    for (size_t i = 0; i < cap; i++)
        if (ht_counts[i] != 0)
            ht_counts[ndirty++] = ht_counts[i];
    return ndirty;
}

__attribute__((target("sse2")))
static double
entropy_u32_sse2(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;

    size_t cap = 16;
    while (cap < 2 * n) cap <<= 1;

    uint32_t *ht_keys   = calloc(cap, sizeof(uint32_t));
    uint32_t *ht_counts = calloc(cap, sizeof(uint32_t));
    if (!ht_keys || !ht_counts) { free(ht_keys); free(ht_counts); return 0.0; }

    ht_build(data, n, ht_keys, ht_counts, cap);
    free(ht_keys);

    size_t ndirty = ht_compact_counts(ht_counts, cap);

    double inv_n = 1.0 / (double)n;
    __m128d vinv = _mm_set1_pd(inv_n);
    __m128d vsum = _mm_setzero_pd();

    size_t i = 0;
    for (; i + 2 <= ndirty; i += 2) {
        __m128i ci = _mm_loadl_epi64((const __m128i *)(ht_counts + i));
        __m128d cd = _mm_cvtepi32_pd(ci);
        __m128d p  = _mm_mul_pd(cd, vinv);
        __m128d l2 = fast_log2_pd_sse2(p);
        vsum = _mm_sub_pd(vsum, _mm_mul_pd(p, l2));
    }

    __m128d sh = _mm_unpackhi_pd(vsum, vsum);
    vsum = _mm_add_pd(vsum, sh);
    double h = _mm_cvtsd_f64(vsum);

    for (; i < ndirty; i++) {
        double p = (double)ht_counts[i] * inv_n;
        h -= p * fast_log2_scalar(p);
    }

    free(ht_counts);
    return h;
}

__attribute__((target("sse4.2")))
static double
entropy_u32_sse42(const uint32_t *data, size_t n)
{
    return entropy_u32_sse2(data, n);
}

__attribute__((target("avx")))
static double
entropy_u32_avx(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;

    size_t cap = 16;
    while (cap < 2 * n) cap <<= 1;

    uint32_t *ht_keys   = calloc(cap, sizeof(uint32_t));
    uint32_t *ht_counts = calloc(cap, sizeof(uint32_t));
    if (!ht_keys || !ht_counts) { free(ht_keys); free(ht_counts); return 0.0; }

    ht_build(data, n, ht_keys, ht_counts, cap);
    free(ht_keys);

    size_t ndirty = ht_compact_counts(ht_counts, cap);

    double inv_n = 1.0 / (double)n;
    __m256d vinv = _mm256_set1_pd(inv_n);
    __m256d vsum = _mm256_setzero_pd();

    size_t i = 0;
    for (; i + 4 <= ndirty; i += 4) {
        __m128i ci = _mm_loadu_si128((const __m128i *)(ht_counts + i));
        __m256d cd = _mm256_cvtepi32_pd(ci);
        __m256d p  = _mm256_mul_pd(cd, vinv);
        __m256d l2 = fast_log2_pd_avx(p);
        vsum = _mm256_sub_pd(vsum, _mm256_mul_pd(p, l2));
    }

    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    lo = _mm_add_pd(lo, hi);
    lo = _mm_add_pd(lo, _mm_unpackhi_pd(lo, lo));
    double h = _mm_cvtsd_f64(lo);

    for (; i < ndirty; i++) {
        double p = (double)ht_counts[i] * inv_n;
        h -= p * fast_log2_scalar(p);
    }

    free(ht_counts);
    return h;
}

__attribute__((target("avx2,fma")))
static double
entropy_u32_avx2(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;

    size_t cap = 16;
    while (cap < 2 * n) cap <<= 1;

    uint32_t *ht_keys   = calloc(cap, sizeof(uint32_t));
    uint32_t *ht_counts = calloc(cap, sizeof(uint32_t));
    if (!ht_keys || !ht_counts) { free(ht_keys); free(ht_counts); return 0.0; }

    ht_build(data, n, ht_keys, ht_counts, cap);
    free(ht_keys);

    size_t ndirty = ht_compact_counts(ht_counts, cap);

    double inv_n = 1.0 / (double)n;
    __m256d vinv = _mm256_set1_pd(inv_n);
    __m256d vsum = _mm256_setzero_pd();

    size_t i = 0;
    for (; i + 4 <= ndirty; i += 4) {
        __m128i ci = _mm_loadu_si128((const __m128i *)(ht_counts + i));
        __m256d cd = _mm256_cvtepi32_pd(ci);
        __m256d p  = _mm256_mul_pd(cd, vinv);
        __m256d l2 = fast_log2_pd_avx2_fma(p);
        vsum = _mm256_fnmadd_pd(p, l2, vsum);
    }

    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    lo = _mm_add_pd(lo, hi);
    lo = _mm_add_pd(lo, _mm_unpackhi_pd(lo, lo));
    double h = _mm_cvtsd_f64(lo);

    for (; i < ndirty; i++) {
        double p = (double)ht_counts[i] * inv_n;
        h -= p * fast_log2_scalar(p);
    }

    free(ht_counts);
    return h;
}

__attribute__((target("avx512f")))
static double
entropy_u32_avx512f(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;

    size_t cap = 16;
    while (cap < 2 * n) cap <<= 1;

    uint32_t *ht_keys   = calloc(cap, sizeof(uint32_t));
    uint32_t *ht_counts = calloc(cap, sizeof(uint32_t));
    if (!ht_keys || !ht_counts) { free(ht_keys); free(ht_counts); return 0.0; }

    ht_build(data, n, ht_keys, ht_counts, cap);
    free(ht_keys);

    /* SIMD compaction via compress-store */
    size_t ndirty = 0;
    __m512i vzero = _mm512_setzero_si512();
    for (size_t j = 0; j < cap; j += 16) {
        __m512i v  = _mm512_loadu_si512(ht_counts + j);
        __mmask16 nz = _mm512_cmpneq_epi32_mask(v, vzero);
        _mm512_mask_compressstoreu_epi32(ht_counts + ndirty, nz, v);
        ndirty += (size_t)_mm_popcnt_u32((unsigned)nz);
    }

    double inv_n = 1.0 / (double)n;
    __m512d vinv = _mm512_set1_pd(inv_n);
    __m512d vsum = _mm512_setzero_pd();

    size_t i = 0;
    for (; i + 8 <= ndirty; i += 8) {
        __m256i ci = _mm256_loadu_si256((const __m256i *)(ht_counts + i));
        __m512d cd = _mm512_cvtepi32_pd(ci);
        __m512d p  = _mm512_mul_pd(cd, vinv);
        __m512d l2 = fast_log2_pd_avx512(p);
        vsum = _mm512_fnmadd_pd(p, l2, vsum);
    }

    __m256d lo4 = _mm512_castpd512_pd256(vsum);
    __m256d hi4 = _mm512_extractf64x4_pd(vsum, 1);
    __m256d s4  = _mm256_add_pd(lo4, hi4);
    __m128d lo2 = _mm256_castpd256_pd128(s4);
    __m128d hi2 = _mm256_extractf128_pd(s4, 1);
    __m128d s2  = _mm_add_pd(lo2, hi2);
    s2 = _mm_add_pd(s2, _mm_unpackhi_pd(s2, s2));
    double h = _mm_cvtsd_f64(s2);

    for (; i < ndirty; i++) {
        double p = (double)ht_counts[i] * inv_n;
        h -= p * fast_log2_scalar(p);
    }

    free(ht_counts);
    return h;
}

#endif /* x86 */

#if defined(__aarch64__)

static double
entropy_u32_neon(const uint32_t *data, size_t n)
{
    return entropy_u32_scalar(data, n);
}

__attribute__((target("+sve")))
static double
entropy_u32_sve(const uint32_t *data, size_t n)
{
    return entropy_u32_scalar(data, n);
}

__attribute__((target("+sve2")))
static double
entropy_u32_sve2(const uint32_t *data, size_t n)
{
    return entropy_u32_scalar(data, n);
}

#endif /* aarch64 */

entropy_u32_fn_t
entropy_u32_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512F: return entropy_u32_avx512f;
    case SIMD_AVX2:    return entropy_u32_avx2;
    case SIMD_AVX:     return entropy_u32_avx;
    case SIMD_SSE4_2:  return entropy_u32_sse42;
    case SIMD_SSE2:    return entropy_u32_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return entropy_u32_sve2;
    case SIMD_SVE:     return entropy_u32_sve;
    case SIMD_NEON:    return entropy_u32_neon;
#endif
    case SIMD_SCALAR:
    default:           return entropy_u32_scalar;
    }
}

static entropy_u32_fn_t
entropy_u32_resolver(void)
{
    return entropy_u32_select(detect_simd_level());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
double entropy_u32(const uint32_t *data, size_t n)
    __attribute__((ifunc("entropy_u32_resolver")));
