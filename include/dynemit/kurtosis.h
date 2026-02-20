/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_KURTOSIS_H
#define DYNEMIT_KURTOSIS_H

#include <stddef.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double (*kurtosis_f64_fn_t)(const double *, size_t);

double kurtosis_f64(const double *data, size_t n);

kurtosis_f64_fn_t kurtosis_f64_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_KURTOSIS_H */
