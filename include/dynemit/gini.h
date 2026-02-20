/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_GINI_H
#define DYNEMIT_GINI_H

#include <stddef.h>
#include <stdint.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double (*gini_f64_fn_t)(const double *, size_t);
typedef double (*gini_u64_fn_t)(const uint64_t *, size_t);

double gini_f64(const double *sorted_data, size_t n);
double gini_u64(const uint64_t *sorted_data, size_t n);

gini_f64_fn_t gini_f64_select(simd_level_t level);
gini_u64_fn_t gini_u64_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_GINI_H */
