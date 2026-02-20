/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_SIMPSON_H
#define DYNEMIT_SIMPSON_H

#include <stddef.h>
#include <stdint.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double (*simpson_u16_fn_t)(const uint16_t *, size_t);
typedef double (*simpson_u32_fn_t)(const uint32_t *, size_t);
typedef double (*simpson_histogram_fn_t)(const uint64_t *, size_t);

double simpson_u16(const uint16_t *data, size_t n);
double simpson_u32(const uint32_t *data, size_t n);
double simpson_histogram(const uint64_t *counts, size_t n);

simpson_u16_fn_t simpson_u16_select(simd_level_t level);
simpson_u32_fn_t simpson_u32_select(simd_level_t level);
simpson_histogram_fn_t simpson_histogram_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_SIMPSON_H */
