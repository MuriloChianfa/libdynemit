/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_SKEWNESS_H
#define DYNEMIT_SKEWNESS_H

#include <stddef.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double (*skewness_f64_fn_t)(const double *, size_t);

double skewness_f64(const double *data, size_t n);

skewness_f64_fn_t skewness_f64_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_SKEWNESS_H */
