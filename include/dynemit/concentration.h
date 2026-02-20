/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_CONCENTRATION_H
#define DYNEMIT_CONCENTRATION_H

#include <stddef.h>
#include <stdint.h>
#include <dynemit/core.h>
#include <dynemit/topk.h>
#include <dynemit/hill.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    double *topk_ratios;
    double  heavy_tail_index;
    double  concentration;
} concentration_result_t;

typedef void (*concentration_f64_fn_t)(const uint64_t *, size_t, uint64_t,
                                       const size_t *, size_t,
                                       concentration_result_t *);

void concentration_f64(const uint64_t *sorted_counts_desc, size_t n,
                       uint64_t total,
                       const size_t *k_values, size_t num_k,
                       concentration_result_t *out);

concentration_f64_fn_t concentration_f64_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_CONCENTRATION_H */
