/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_ENTROPY_H
#define DYNEMIT_ENTROPY_H

#include <stddef.h>
#include <stdint.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double (*entropy_u16_fn_t)(const uint16_t *, size_t);
typedef double (*entropy_u32_fn_t)(const uint32_t *, size_t);
typedef double (*entropy_histogram_fn_t)(const uint64_t *, size_t);

double entropy_u16(const uint16_t *data, size_t n);
double entropy_u32(const uint32_t *data, size_t n);
double entropy_histogram(const uint64_t *counts, size_t n);

entropy_u16_fn_t entropy_u16_select(simd_level_t level);
entropy_u32_fn_t entropy_u32_select(simd_level_t level);
entropy_histogram_fn_t entropy_histogram_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_ENTROPY_H */
