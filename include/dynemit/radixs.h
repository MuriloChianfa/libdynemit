/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_RADIXS_H
#define DYNEMIT_RADIXS_H

#include <stddef.h>
#include <stdint.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Out-of-place ascending radix sort.
 *
 * Three integer-key variants:
 *   - radixs_u16: 16-bit counting sort with a 65536-entry histogram.
 *     No external scratch is required.
 *   - radixs_u32: least-significant-digit (LSD) 8-bit radix sort,
 *     4 passes. Internally allocates a temporary buffer of n*sizeof(uint32_t)
 *     bytes via aligned_alloc.
 *   - radixs_u64: LSD 8-bit radix sort, 8 passes. Internally allocates
 *     a temporary buffer of n*sizeof(uint64_t) bytes.
 *
 * Contract:
 *   - The result lands in @p out in ascending order.
 *   - @p in and @p out MUST NOT alias.
 *   - n == 0 is a no-op (in may be NULL).
 *   - On internal allocation failure (u32/u64 only), the function falls back
 *     to copying @p in into @p out and finishing the sort in-place using
 *     qsort, so callers always observe a sorted output.
 *
 * Why SIMD: LSD radix sort is branch-free and cache-friendly, and the
 * histogram and scatter phases map well onto AVX-512F gather/scatter and
 * AVX-512 VBMI2 byte permutes. Used by network-traffic feature extractors
 * over per-window arrays of IPs, ports, ASNs, and packet counts.
 */
typedef void (*radixs_u16_fn_t)(const uint16_t *, uint16_t *, size_t);
typedef void (*radixs_u32_fn_t)(const uint32_t *, uint32_t *, size_t);
typedef void (*radixs_u64_fn_t)(const uint64_t *, uint64_t *, size_t);

void radixs_u16(const uint16_t *in, uint16_t *out, size_t n);
void radixs_u32(const uint32_t *in, uint32_t *out, size_t n);
void radixs_u64(const uint64_t *in, uint64_t *out, size_t n);

radixs_u16_fn_t radixs_u16_select(simd_level_t level);
radixs_u32_fn_t radixs_u32_select(simd_level_t level);
radixs_u64_fn_t radixs_u64_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_RADIXS_H */
