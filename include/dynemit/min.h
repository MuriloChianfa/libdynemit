/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_MIN_H
#define DYNEMIT_MIN_H

#include <stddef.h>
#include <stdint.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double (*min_f64_fn_t)(const double *, size_t);
typedef double (*min_u64_fn_t)(const uint64_t *, size_t);
typedef double (*min_u32_fn_t)(const uint32_t *, size_t);
typedef double (*min_u16_fn_t)(const uint16_t *, size_t);

double min_f64(const double *data, size_t n);
double min_u64(const uint64_t *data, size_t n);
double min_u32(const uint32_t *data, size_t n);
double min_u16(const uint16_t *data, size_t n);

min_f64_fn_t min_f64_select(simd_level_t level);
min_u64_fn_t min_u64_select(simd_level_t level);
min_u32_fn_t min_u32_select(simd_level_t level);
min_u16_fn_t min_u16_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_MIN_H */
