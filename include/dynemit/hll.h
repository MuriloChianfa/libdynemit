/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_HLL_H
#define DYNEMIT_HLL_H

#include <stddef.h>
#include <stdint.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * HyperLogLog++ approximate distinct-count cardinality estimator.
 *
 * One-shot API: given an input array, returns an estimate of the number
 * of distinct values. Internally uses a compile-time-sized sketch of
 * m = 2^DYNEMIT_HLL_P 8-bit registers (default p=10, m=1024, ~3.25% std error).
 *
 * The sketch lives in a thread-local scratch buffer reused across calls
 * (lazy-allocated; zeroed after each estimate).
 *
 * Algorithm:
 *   - 64-bit branchless SplitMix64 hash of each element
 *   - top p bits pick a register, remaining bits contribute rank = clz+1
 *   - register holds max observed rank (monotonic, order-independent)
 *   - final estimate uses alpha_m * m^2 / sum 2^(-r[i]); when the number
 *     of zero registers V is non-zero and the raw estimate is small,
 *     LinearCounting (m * ln(m/V)) is used instead (HLL++ small-range fix)
 *
 * Returns 0.0 for n == 0. Result is deterministic: every SIMD variant
 * produces bit-identical output on the same input (same hash, same
 * register updates, same reduction order in the scalar tail).
 */
typedef double (*hll_u32_fn_t)(const uint32_t *data, size_t n);
typedef double (*hll_u64_fn_t)(const uint64_t *data, size_t n);

double hll_u32(const uint32_t *data, size_t n);
double hll_u64(const uint64_t *data, size_t n);

hll_u32_fn_t hll_u32_select(simd_level_t level);
hll_u64_fn_t hll_u64_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_HLL_H */
