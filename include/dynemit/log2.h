/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_LOG2_H
#define DYNEMIT_LOG2_H

#include <stddef.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*log2_f64_fn_t)(const double *, double *, size_t);

void log2_f64(const double *in, double *out, size_t n);

log2_f64_fn_t log2_f64_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_LOG2_H */
