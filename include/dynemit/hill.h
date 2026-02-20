/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_HILL_H
#define DYNEMIT_HILL_H

#include <stddef.h>
#include <stdint.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double (*hill_estimator_f64_fn_t)(const uint64_t *, size_t);

double hill_estimator_f64(const uint64_t *sorted_desc, size_t n);

hill_estimator_f64_fn_t hill_estimator_f64_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_HILL_H */
