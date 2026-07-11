/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elifdef __aarch64__
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include "mem.h"
#include <dynemit/compiler.h>
#include <dynemit/radixs.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    RADIXS_U16_BUCKETS = 65536U
};

/*
 * Allocate a 65536-entry uint32_t histogram on the heap (256 KB), aligned
 * to 64 bytes for cache-line / AVX-512 friendliness. Returns NULL on
 * allocation failure; caller falls back to qsort.
 */
static inline uint32_t *
radixs_u16_alloc_counts(void)
{
    void *p = aligned_alloc(64, RADIXS_U16_BUCKETS * sizeof(uint32_t));
    if (p) {
        if (memsets(p, RADIXS_U16_BUCKETS * sizeof(uint32_t), 0,
                             RADIXS_U16_BUCKETS * sizeof(uint32_t)) != 0) {
            free(p);
            return nullptr;
        }
    }
    return (uint32_t *)p;
}

static int
radixs_u16_cmp(const void *a, const void *b)
{
    uint16_t x = *(const uint16_t *)a;
    uint16_t y = *(const uint16_t *)b;
    return (x > y) - (x < y);
}

static void
radixs_u16_qsort_fallback(const uint16_t *in, uint16_t *out, size_t n)
{
    if (in != out) {
        if (memcpys(out, n * sizeof(uint16_t), in,
                             n * sizeof(uint16_t)) != 0) {
            return;
        }
    }
    qsort(out, n, sizeof(uint16_t), radixs_u16_cmp);
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static void
radixs_u16_scalar(const uint16_t *in, uint16_t *out, size_t n)
{
    if (n == 0) {
        return;
    }

    uint32_t *counts = radixs_u16_alloc_counts();
    if (!counts) {
        radixs_u16_qsort_fallback(in, out, n);
        return;
    }

DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        counts[in[i]]++;
    }

    size_t pos = 0;
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (uint32_t v = 0; v < RADIXS_U16_BUCKETS; v++) {
        uint32_t c = counts[v];
        for (uint32_t k = 0; k < c; k++) {
            out[pos++] = (uint16_t)v;
        }
    }

    free(counts);
}


#if defined(__x86_64__) || defined(__i386__)

/*
 * Helper: fill `c` copies of `v` into out. Used by the bucket-emit phase
 * across the SIMD variants. Inlined per-target so the broadcast width
 * matches the variant's target attribute.
 */
__attribute__((target("sse2")))
static inline void
radixs_u16_fill_sse2(uint16_t *out, uint16_t v, uint32_t c)
{
    __m128i vv = _mm_set1_epi16((short)v);
    uint32_t i = 0;
    for (; i + 8 <= c; i += 8) {
        _mm_storeu_si128((__m128i *)(out + i), vv);
    }
    for (; i < c; i++) {
        out[i] = v;
    }
}

__attribute__((target("sse2")))
static void
radixs_u16_sse2(const uint16_t *in, uint16_t *out, size_t n)
{
    if (n == 0) {
        return;
    }

    uint32_t *counts = radixs_u16_alloc_counts();
    if (!counts) {
        radixs_u16_qsort_fallback(in, out, n);
        return;
    }

    for (size_t i = 0; i < n; i++) {
        counts[in[i]]++;
    }

    size_t pos = 0;
    for (uint32_t v = 0; v < RADIXS_U16_BUCKETS; v++) {
        uint32_t c = counts[v];
        if (c == 0) {
            continue;
        }
        radixs_u16_fill_sse2(out + pos, (uint16_t)v, c);
        pos += c;
    }

    free(counts);
}

__attribute__((target("sse4.2")))
static void
radixs_u16_sse42(const uint16_t *in, uint16_t *out, size_t n)
{
    radixs_u16_sse2(in, out, n);
}

__attribute__((target("avx")))
static void
radixs_u16_avx(const uint16_t *in, uint16_t *out, size_t n)
{
    radixs_u16_sse2(in, out, n);
}

__attribute__((target("avx2")))
static inline void
radixs_u16_fill_avx2(uint16_t *out, uint16_t v, uint32_t c)
{
    __m256i vv = _mm256_set1_epi16((short)v);
    uint32_t i = 0;
    for (; i + 16 <= c; i += 16) {
        _mm256_storeu_si256((__m256i *)(out + i), vv);
    }
    if (i + 8 <= c) {
        __m128i vv128 = _mm_set1_epi16((short)v);
        _mm_storeu_si128((__m128i *)(out + i), vv128);
        i += 8;
    }
    for (; i < c; i++) {
        out[i] = v;
    }
}

__attribute__((target("avx2")))
static void
radixs_u16_avx2(const uint16_t *in, uint16_t *out, size_t n)
{
    if (n == 0) {
        return;
    }

    uint32_t *counts = radixs_u16_alloc_counts();
    if (!counts) {
        radixs_u16_qsort_fallback(in, out, n);
        return;
    }

    /*
     * Two partial histograms broken by index parity to reduce the
     * dependency chain on the same bucket. Both are summed into counts0
     * before the emit phase.
     */
    uint32_t *counts0 = counts;
    uint32_t *counts1 = (uint32_t *)aligned_alloc(64,
                            RADIXS_U16_BUCKETS * sizeof(uint32_t));
    if (counts1) {
        if (memsets(counts1, RADIXS_U16_BUCKETS * sizeof(uint32_t), 0,
                             RADIXS_U16_BUCKETS * sizeof(uint32_t)) != 0) {
            free(counts1);
            counts1 = nullptr;
        }
    }
    if (counts1) {
        size_t i = 0;
        for (; i + 2 <= n; i += 2) {
            counts0[in[i]]++;
            counts1[in[i + 1]]++;
        }
        for (; i < n; i++) {
            counts0[in[i]]++;
        }
        for (uint32_t v = 0; v < RADIXS_U16_BUCKETS; v++) {
            counts0[v] += counts1[v];
        }
        free(counts1);
    } else {
        for (size_t i = 0; i < n; i++) {
            counts0[in[i]]++;
        }
    }

    size_t pos = 0;
    for (uint32_t v = 0; v < RADIXS_U16_BUCKETS; v++) {
        uint32_t c = counts0[v];
        if (c == 0) {
            continue;
        }
        radixs_u16_fill_avx2(out + pos, (uint16_t)v, c);
        pos += c;
    }

    free(counts);
}

__attribute__((target("avx512f,avx512bw")))
static inline void
radixs_u16_fill_avx512(uint16_t *out, uint16_t v, uint32_t c)
{
    __m512i vv = _mm512_set1_epi16((short)v);
    uint32_t i = 0;
    for (; i + 32 <= c; i += 32) {
        _mm512_storeu_si512((__m512i *)(out + i), vv);
    }
    if (i < c) {
        uint32_t rem = c - i;
        __mmask32 m = (__mmask32)((1U << rem) - 1U);
        _mm512_mask_storeu_epi16(out + i, m, vv);
    }
}

__attribute__((target("avx512f,avx512bw")))
static void
radixs_u16_avx512f(const uint16_t *in, uint16_t *out, size_t n)
{
    if (n == 0) {
        return;
    }

    uint32_t *counts = radixs_u16_alloc_counts();
    if (!counts) {
        radixs_u16_qsort_fallback(in, out, n);
        return;
    }

    for (size_t i = 0; i < n; i++) {
        counts[in[i]]++;
    }

    size_t pos = 0;
    for (uint32_t v = 0; v < RADIXS_U16_BUCKETS; v++) {
        uint32_t c = counts[v];
        if (c == 0) {
            continue;
        }
        radixs_u16_fill_avx512(out + pos, (uint16_t)v, c);
        pos += c;
    }

    free(counts);
}

__attribute__((target("avx512vbmi2,avx512vbmi,avx512bw,avx512f")))
static void
radixs_u16_avx512_vbmi2(const uint16_t *in, uint16_t *out, size_t n)
{
    /*
     * The hot path for u16 counting sort is the bucket-emit. VBMI2 does
     * not add a faster variant of "broadcast v into c contiguous u16
     * slots", so we share the AVX-512F+BW broadcast-store path and let
     * the IFUNC dispatch label this variant for VBMI2-class CPUs.
     */
    radixs_u16_avx512f(in, out, n);
}

#endif /* x86 */

#ifdef __aarch64__

static inline void
radixs_u16_fill_neon(uint16_t *out, uint16_t v, uint32_t c)
{
    uint16x8_t vv = vdupq_n_u16(v);
    uint32_t i = 0;
    for (; i + 8 <= c; i += 8)
        vst1q_u16(out + i, vv);
    for (; i < c; i++)
        out[i] = v;
}

static void
radixs_u16_neon(const uint16_t *in, uint16_t *out, size_t n)
{
    if (n == 0) return;

    uint32_t *counts = radixs_u16_alloc_counts();
    if (!counts) {
        radixs_u16_qsort_fallback(in, out, n);
        return;
    }

    for (size_t i = 0; i < n; i++)
        counts[in[i]]++;

    size_t pos = 0;
    for (uint32_t v = 0; v < RADIXS_U16_BUCKETS; v++) {
        uint32_t c = counts[v];
        if (c == 0) continue;
        radixs_u16_fill_neon(out + pos, (uint16_t)v, c);
        pos += c;
    }

    free(counts);
}

__attribute__((target("+sve")))
static inline void
radixs_u16_fill_sve(uint16_t *out, uint16_t v, uint32_t c)
{
    svuint16_t vv = svdup_u16(v);
    uint64_t vl = svcnth();
    uint64_t i = 0;
    svbool_t pg = svwhilelt_b16(i, (uint64_t)c);
    while (svptest_any(svptrue_b16(), pg)) {
        svst1_u16(pg, out + i, vv);
        i += vl;
        pg = svwhilelt_b16(i, (uint64_t)c);
    }
}

__attribute__((target("+sve")))
static void
radixs_u16_sve(const uint16_t *in, uint16_t *out, size_t n)
{
    if (n == 0) return;

    uint32_t *counts = radixs_u16_alloc_counts();
    if (!counts) {
        radixs_u16_qsort_fallback(in, out, n);
        return;
    }

    for (size_t i = 0; i < n; i++)
        counts[in[i]]++;

    size_t pos = 0;
    for (uint32_t v = 0; v < RADIXS_U16_BUCKETS; v++) {
        uint32_t c = counts[v];
        if (c == 0) continue;
        radixs_u16_fill_sve(out + pos, (uint16_t)v, c);
        pos += c;
    }

    free(counts);
}

__attribute__((target("+sve2")))
static void
radixs_u16_sve2(const uint16_t *in, uint16_t *out, size_t n)
{
    radixs_u16_sve(in, out, n);
}

#endif /* aarch64 */

radixs_u16_fn_t
radixs_u16_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2: return radixs_u16_avx512_vbmi2;
    case SIMD_AVX512F: return radixs_u16_avx512f;
    case SIMD_AVX2:    return radixs_u16_avx2;
    case SIMD_AVX:     return radixs_u16_avx;
    case SIMD_SSE4_2:  return radixs_u16_sse42;
    case SIMD_SSE2:    return radixs_u16_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return radixs_u16_sve2;
    case SIMD_SVE:     return radixs_u16_sve;
    case SIMD_NEON:    return radixs_u16_neon;
#endif
    case SIMD_SCALAR:
    default:           return radixs_u16_scalar;
}
}

EXPLICIT_RUNTIME_RESOLVER(radixs_u16_resolver, radixs_u16_fn_t)
{
    return radixs_u16_select(detect_simd_level_ts());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512vbmi2,avx512vbmi,avx512bw,avx512f,avx2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
void radixs_u16(const uint16_t *in, uint16_t *out, size_t n)
    __attribute__((ifunc("radixs_u16_resolver")));
