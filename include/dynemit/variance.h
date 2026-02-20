/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_VARIANCE_H
#define DYNEMIT_VARIANCE_H

#include <stddef.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double (*variance_f64_fn_t)(const double *, size_t);

double variance_f64(const double *data, size_t n);

variance_f64_fn_t variance_f64_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_VARIANCE_H */
