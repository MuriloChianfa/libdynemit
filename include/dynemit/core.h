#ifndef DYNEMIT_CORE_H
#define DYNEMIT_CORE_H

#include <stdint.h>

// CPU feature detection helpers
void cpuid_x86(uint32_t leaf, uint32_t subleaf,
               uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

uint64_t xgetbv_x86(uint32_t xcr);

// SIMD levels
typedef enum {
    SIMD_SCALAR = 0,
    SIMD_SSE2 = 1,
    SIMD_SSE4_2 = 2,
    SIMD_AVX = 3,
    SIMD_AVX2 = 4,
    SIMD_AVX512F = 5
} simd_level_t;

simd_level_t detect_simd_level(void);

const char *simd_level_name(simd_level_t level);

/**
 * Get list of available features in this build.
 * Returns nullptr-terminated array of feature names.
 * Only available when using the all-in-one library (DYNEMIT_ALL_FEATURES defined).
 */
const char **dynemit_features(void);

#endif // DYNEMIT_CORE_H

