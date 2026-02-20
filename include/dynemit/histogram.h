/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_HISTOGRAM_H
#define DYNEMIT_HISTOGRAM_H

#include <stddef.h>
#include <stdint.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*histogram_u16_fn_t)(const uint16_t *, size_t,
                                   const uint16_t *, size_t, uint64_t *);
typedef void (*histogram_u64_fn_t)(const uint64_t *, size_t,
                                   const uint64_t *, size_t, uint64_t *);

void histogram_u16(const uint16_t *data, size_t n,
                   const uint16_t *boundaries, size_t num_boundaries,
                   uint64_t *out);

void histogram_u64(const uint64_t *data, size_t n,
                   const uint64_t *boundaries, size_t num_boundaries,
                   uint64_t *out);

histogram_u16_fn_t histogram_u16_select(simd_level_t level);
histogram_u64_fn_t histogram_u64_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_HISTOGRAM_H */
