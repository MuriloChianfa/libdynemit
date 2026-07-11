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
#include <stdlib.h>
#include <dynemit/histogram.h>
#include <dynemit/compiler.h>
#include "mem.h"

/*
 * Count elements falling into ranges defined by boundaries.
 * Given boundaries b0, b1, ..., b_{k-1} (ascending), counts elements in:
 *   [0, b0), [b0, b1), ..., [b_{k-1}, UINT64_MAX].
 * Output array has num_boundaries+1 elements.
 *
 * For u64 with small num_boundaries, SIMD benefit is limited
 * (only 2 per SSE2, 4 per AVX2). Use scalar for most levels,
 * SIMD from AVX512F for the wide registers.
 */

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static void
histogram_u64_scalar(const uint64_t *data, size_t n,
                           const uint64_t *boundaries, size_t num_boundaries,
                           uint64_t *out)
{
    if (memsets(out, (num_boundaries + 1) * sizeof(uint64_t), 0,
                         (num_boundaries + 1) * sizeof(uint64_t)) != 0)
        return;
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        uint64_t val = data[i];
        size_t bucket = num_boundaries;
        for (size_t b = 0; b < num_boundaries; b++) {
            if (val < boundaries[b]) {
                bucket = b;
                break;
            }
        }
        out[bucket]++;
    }
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static void
histogram_u64_sse2(const uint64_t *data, size_t n,
                         const uint64_t *boundaries, size_t num_boundaries,
                         uint64_t *out)
{
    histogram_u64_scalar(data, n, boundaries, num_boundaries, out);
}

__attribute__((target("sse4.2")))
static void
histogram_u64_sse42(const uint64_t *data, size_t n,
                          const uint64_t *boundaries, size_t num_boundaries,
                          uint64_t *out)
{
    histogram_u64_scalar(data, n, boundaries, num_boundaries, out);
}

__attribute__((target("avx")))
static void
histogram_u64_avx(const uint64_t *data, size_t n,
                        const uint64_t *boundaries, size_t num_boundaries,
                        uint64_t *out)
{
    histogram_u64_scalar(data, n, boundaries, num_boundaries, out);
}

__attribute__((target("avx2")))
static void
histogram_u64_avx2(const uint64_t *data, size_t n,
                         const uint64_t *boundaries, size_t num_boundaries,
                         uint64_t *out)
{
    histogram_u64_scalar(data, n, boundaries, num_boundaries, out);
}

__attribute__((target("avx512f")))
static void
histogram_u64_avx512f(const uint64_t *data, size_t n,
                            const uint64_t *boundaries, size_t num_boundaries,
                            uint64_t *out)
{
    /*
     * AVX512F has native unsigned 64-bit comparison via mask registers.
     * Process 8 elements at a time. For each boundary, count elements
     * that are less than the boundary using _mm512_cmplt_epu64_mask.
     */
    if (memsets(out, (num_boundaries + 1) * sizeof(uint64_t), 0,
                         (num_boundaries + 1) * sizeof(uint64_t)) != 0)
        return;
    if (num_boundaries == 0) {
        out[0] = n;
        return;
    }

    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m512i vdata = _mm512_loadu_si512(data + i);

        /* Determine bucket for each element.
         * bucket = num_boundaries - (number of boundaries where elem < boundary) */
        uint8_t ge_counts[8] = {0};
        for (size_t b = 0; b < num_boundaries; b++) {
            __m512i vbound = _mm512_set1_epi64((long long)boundaries[b]);
            /* mask: bit set where data[j] < boundary[b] */
            __mmask8 lt_mask = _mm512_cmplt_epu64_mask(vdata, vbound);
            /* For elements where data < boundary, they are in bucket <= b.
             * We want the first boundary where elem < boundary. */
            for (size_t j = 0; j < 8; j++) {
                if ((lt_mask & (1u << j)) && ge_counts[j] == 0)
                    ge_counts[j] = (uint8_t)(b + 1);
            }
        }
        for (size_t j = 0; j < 8; j++) {
            size_t bucket = ge_counts[j] > 0 ? (size_t)(ge_counts[j] - 1) : num_boundaries;
            out[bucket]++;
        }
    }

    /* Scalar tail */
    for (; i < n; i++) {
        uint64_t val = data[i];
        size_t bucket = num_boundaries;
        for (size_t b = 0; b < num_boundaries; b++) {
            if (val < boundaries[b]) {
                bucket = b;
                break;
            }
        }
        out[bucket]++;
    }
}
#endif

#if defined(__aarch64__)

static void
histogram_u64_neon(const uint64_t *data, size_t n,
                         const uint64_t *boundaries, size_t num_boundaries,
                         uint64_t *out)
{
    histogram_u64_scalar(data, n, boundaries, num_boundaries, out);
}

__attribute__((target("+sve")))
static void
histogram_u64_sve(const uint64_t *data, size_t n,
                        const uint64_t *boundaries, size_t num_boundaries,
                        uint64_t *out)
{
    histogram_u64_scalar(data, n, boundaries, num_boundaries, out);
}

__attribute__((target("+sve2")))
static void
histogram_u64_sve2(const uint64_t *data, size_t n,
                         const uint64_t *boundaries, size_t num_boundaries,
                         uint64_t *out)
{
    histogram_u64_scalar(data, n, boundaries, num_boundaries, out);
}

#endif /* aarch64 */

histogram_u64_fn_t
histogram_u64_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return histogram_u64_avx512f;
    case SIMD_AVX2:    return histogram_u64_avx2;
    case SIMD_AVX:     return histogram_u64_avx;
    case SIMD_SSE4_2:  return histogram_u64_sse42;
    case SIMD_SSE2:    return histogram_u64_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return histogram_u64_sve2;
    case SIMD_SVE:     return histogram_u64_sve;
    case SIMD_NEON:    return histogram_u64_neon;
#endif
    case SIMD_SCALAR:
    default:           return histogram_u64_scalar;
    }
}

EXPLICIT_RUNTIME_RESOLVER(histogram_u64_resolver, histogram_u64_fn_t)
{
    return histogram_u64_select(detect_simd_level_ts());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
void histogram_u64(const uint64_t *data, size_t n,
                         const uint64_t *boundaries, size_t num_boundaries,
                         uint64_t *out)
    __attribute__((ifunc("histogram_u64_resolver")));
