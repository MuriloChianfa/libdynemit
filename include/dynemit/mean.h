/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_MEAN_H
#define DYNEMIT_MEAN_H

#include <stddef.h>
#include <stdint.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double (*mean_f64_fn_t)(const double *, size_t);
typedef double (*mean_u64_fn_t)(const uint64_t *, size_t);
typedef double (*mean_u32_fn_t)(const uint32_t *, size_t);
typedef double (*mean_u16_fn_t)(const uint16_t *, size_t);

double mean_f64(const double *data, size_t n);
double mean_u64(const uint64_t *data, size_t n);
double mean_u32(const uint32_t *data, size_t n);
double mean_u16(const uint16_t *data, size_t n);

mean_f64_fn_t mean_f64_select(simd_level_t level);
mean_u64_fn_t mean_u64_select(simd_level_t level);
mean_u32_fn_t mean_u32_select(simd_level_t level);
mean_u16_fn_t mean_u16_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_MEAN_H */
