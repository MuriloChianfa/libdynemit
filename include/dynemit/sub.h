/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_SUB_H
#define DYNEMIT_SUB_H

#include <stddef.h>
#include <dynemit/core.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*sub_f32_fn_t)(const float *, const float *, float *, size_t);

void sub_f32(const float *a, const float *b, float *out, size_t n);

sub_f32_fn_t sub_f32_select(simd_level_t level);

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_SUB_H */
