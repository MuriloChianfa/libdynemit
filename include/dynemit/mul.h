/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_MUL_H
#define DYNEMIT_MUL_H

#include <stddef.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*mul_f32_fn_t)(const float *, const float *, float *, size_t);

void mul_f32(const float *a, const float *b, float *out, size_t n);

mul_f32_fn_t mul_f32_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_MUL_H */
