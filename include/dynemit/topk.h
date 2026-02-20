/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_TOPK_H
#define DYNEMIT_TOPK_H

#include <stddef.h>
#include <stdint.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*topk_ratios_f64_fn_t)(const uint64_t *, size_t, uint64_t,
                                     const size_t *, size_t, double *);

void topk_ratios_f64(const uint64_t *sorted_desc, size_t n,
                     uint64_t total,
                     const size_t *k_values, size_t num_k,
                     double *out_ratios);

topk_ratios_f64_fn_t topk_ratios_f64_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_TOPK_H */
