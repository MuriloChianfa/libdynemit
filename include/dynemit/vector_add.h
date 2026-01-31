/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_VECTOR_ADD_H
#define DYNEMIT_VECTOR_ADD_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Element-wise addition of two float vectors: out[i] = a[i] + b[i]
 * Automatically dispatches to the best SIMD implementation available.
 */
void vector_add_f32(const float *a, const float *b, float *out, size_t n);

#ifdef __cplusplus
}
#endif

#endif // DYNEMIT_VECTOR_ADD_H
