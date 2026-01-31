/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_ERR_H
#define DYNEMIT_ERR_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file err.h
 * @brief Error handling utilities for dynemit runtime dispatch
 * 
 * This header provides error handling utilities for IFUNC resolvers and
 * runtime dispatch mechanisms. It ensures safe behavior when libraries
 * are loaded via dlopen() (e.g., Python extensions, plugin systems).
 * 
 * The IFUNC Problem:
 * ------------------
 * IFUNC resolvers run during ELF relocation, which happens during dlopen().
 * In some contexts (especially Python's dynamic module loading), resolvers
 * may run before the library is fully initialized, leading to crashes if
 * the resolver returns NULL or encounters issues.
 * 
 * The Solution:
 * -------------
 * Use EXPLICIT_RUNTIME_RESOLVER to wrap resolver functions with safety
 * checks that ensure non-NULL returns and provide clear failure diagnostics.
 * Combine with detect_simd_level_ts() for thread-safe SIMD detection.
 * 
 * @see detect_simd_level_ts() in <dynemit/core.h>
 * @see docs/IFUNC_RESOLVERS.md for detailed documentation
 */

/**
 * @brief Explicit runtime resolver macro for IFUNC dispatch
 * 
 * This macro wraps IFUNC resolver functions to ensure they never return NULL,
 * which could cause segmentation faults. If the resolver logic somehow produces
 * a NULL pointer, the program will trap immediately with a clear error rather
 * than causing undefined behavior later.
 * 
 * The macro generates:
 * - A wrapper function `name` that validates the result
 * - An implementation function `name_impl` where you write the actual logic
 * 
 * Usage Example:
 * @code
 * #include <dynemit/core.h>
 * #include <dynemit/err.h>
 * 
 * typedef void (*my_func_t)(float*, float*, size_t);
 * 
 * EXPLICIT_RUNTIME_RESOLVER(my_function_resolver)
 * {
 *     simd_level_t level = detect_simd_level_ts();
 *     
 *     switch (level) {
 *     case SIMD_AVX512F:
 *         return (void*)my_function_avx512;
 *     case SIMD_AVX2:
 *         return (void*)my_function_avx2;
 *     case SIMD_SSE2:
 *         return (void*)my_function_sse2;
 *     default:
 *         return (void*)my_function_scalar;
 *     }
 * }
 * 
 * void my_function(float *a, float *b, size_t n)
 *     __attribute__((ifunc("my_function_resolver")));
 * @endcode
 * 
 * @note Function pointer to void* conversions will generate -Wpedantic warnings.
 *       These are expected and safe for IFUNC resolvers. You can suppress them
 *       with: #pragma GCC diagnostic ignored "-Wpedantic"
 * 
 * @note Always use detect_simd_level_ts() (not detect_simd_level()) inside
 *       resolvers to ensure thread-safe, cached SIMD detection.
 * 
 * @param name The name of the resolver function (used by ifunc attribute)
 */
#define EXPLICIT_RUNTIME_RESOLVER(name) \
    static void* name##_impl(void); \
    __attribute__((used)) \
    static void* name(void) { \
        void* result = name##_impl(); \
        if (!result) { \
            __builtin_trap(); \
        } \
        return result; \
    } \
    static void* name##_impl(void)

#ifdef __cplusplus
}
#endif

#endif /* DYNEMIT_ERR_H */
