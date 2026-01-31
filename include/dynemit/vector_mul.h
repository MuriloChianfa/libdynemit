/* SPDX-License-Identifier: BSL-1.0 */
#ifndef VECTOR_MUL_H
#define VECTOR_MUL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Vector multiplication function - dynamically dispatched at runtime
// based on available CPU SIMD capabilities
void vector_mul_f32(const float *a, const float *b, float *out, size_t n);

#ifdef __cplusplus
}
#endif

#endif // VECTOR_MUL_H
