/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_ADD_H
#define DYNEMIT_ADD_H

#include <stddef.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*add_f32_fn_t)(const float *, const float *, float *, size_t);

/**
 * Element-wise addition of two float vectors: out[i] = a[i] + b[i]
 * Automatically dispatches to the best SIMD implementation available.
 */
void add_f32(const float *a, const float *b, float *out, size_t n);

/**
 * Return function pointer to the add_f32 implementation for a specific SIMD level.
 */
add_f32_fn_t add_f32_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_ADD_H */
