/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_VECTOR_SUB_H
#define DYNEMIT_VECTOR_SUB_H

#include <stddef.h>

/**
 * Element-wise subtraction of two float vectors: out[i] = a[i] - b[i]
 * Automatically dispatches to the best SIMD implementation available.
 */
void vector_sub_f32(const float *a, const float *b, float *out, size_t n);

#endif // DYNEMIT_VECTOR_SUB_H
