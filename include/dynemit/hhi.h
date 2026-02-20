/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_HHI_H
#define DYNEMIT_HHI_H

#include <stddef.h>
#include <stdint.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef double (*hhi_u16_fn_t)(const uint16_t *, size_t);
typedef double (*hhi_u32_fn_t)(const uint32_t *, size_t);
typedef double (*hhi_histogram_fn_t)(const uint64_t *, size_t);

double hhi_u16(const uint16_t *data, size_t n);
double hhi_u32(const uint32_t *data, size_t n);
double hhi_histogram(const uint64_t *counts, size_t n);

hhi_u16_fn_t hhi_u16_select(simd_level_t level);
hhi_u32_fn_t hhi_u32_select(simd_level_t level);
hhi_histogram_fn_t hhi_histogram_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_HHI_H */
