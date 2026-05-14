/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_CORE_H
#define DYNEMIT_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// CPU feature detection helpers
void cpuid_x86(uint32_t leaf, uint32_t subleaf,
               uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);

uint64_t xgetbv_x86(uint32_t xcr);

typedef enum {
    // x86
    SIMD_SCALAR = 0,
    SIMD_SSE2 = 1,
    SIMD_SSE4_2 = 2,
    SIMD_AVX = 3,
    SIMD_AVX2 = 4,
    SIMD_AVX512F = 5,
    SIMD_AVX512_VBMI2 = 6,
    // AArch64
    SIMD_NEON = 10,
    SIMD_SVE = 11,
    SIMD_SVE2 = 12
} simd_level_t;

/**
 * Detect the highest SIMD level supported by the CPU.
 * 
 * This function performs runtime CPU feature detection using CPUID to determine
 * the most advanced SIMD instruction set available. It checks both CPU support
 * and OS support (via XGETBV) for each SIMD level.
 * 
 * Note: This function performs CPUID calls each time it's invoked, which can
 * add overhead if called frequently. For performance-critical paths or
 * multi-threaded contexts (especially IFUNC resolvers), use detect_simd_level_ts()
 * instead, which caches the result.
 * 
 * @return The highest supported SIMD level, from SIMD_SCALAR (baseline) to
 *         SIMD_AVX512F on x86 or SIMD_SVE2 on AArch64.
 * @see detect_simd_level_ts() for cached, thread-safe version
 */
simd_level_t detect_simd_level(void);

/**
 * Thread-safe cached SIMD level detection.
 * 
 * This function caches the result of detect_simd_level() using atomic operations
 * to avoid repeated CPUID calls in multi-threaded contexts. Safe to call from
 * IFUNC resolvers and concurrent threads.
 * 
 * The result is computed once on first call and cached for subsequent calls.
 * All threads will observe the same cached value after initialization.
 * 
 * Use this instead of detect_simd_level() when:
 * - Called from IFUNC resolvers (required for dlopen() compatibility)
 * - Called frequently from multiple threads
 * - Performance-critical paths where CPUID overhead matters
 * 
 * @return Detected SIMD level (cached after first call)
 * @see detect_simd_level() for non-cached version
 * @see EXPLICIT_RUNTIME_RESOLVER in <dynemit/err.h> for IFUNC resolver wrapper
 */
simd_level_t detect_simd_level_ts(void);

const char *simd_level_name(simd_level_t level);

/**
 * Architecture-specific table of all SIMD levels supported by this build.
 * Useful for iterating over levels in tests and benchmarks.
 */
#if defined(__x86_64__) || defined(__i386__)
static const simd_level_t DYNEMIT_SIMD_LEVELS[] = {
    SIMD_SCALAR, SIMD_SSE2, SIMD_SSE4_2, SIMD_AVX,
    SIMD_AVX2, SIMD_AVX512F, SIMD_AVX512_VBMI2
};
#elif defined(__aarch64__)
static const simd_level_t DYNEMIT_SIMD_LEVELS[] = {
    SIMD_SCALAR, SIMD_NEON, SIMD_SVE, SIMD_SVE2
};
#else
static const simd_level_t DYNEMIT_SIMD_LEVELS[] = {
    SIMD_SCALAR
};
#endif
#define DYNEMIT_N_LEVELS \
    ((int)(sizeof(DYNEMIT_SIMD_LEVELS) / sizeof(DYNEMIT_SIMD_LEVELS[0])))

/**
 * Get list of available features in this build.
 * Returns nullptr-terminated array of feature names.
 * Only available when using the all-in-one library (DYNEMIT_ALL_FEATURES defined).
 */
const char **dynemit_features(void);

#ifdef __cplusplus
}
#endif

#endif // DYNEMIT_CORE_H

