/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_COMPILER_H
#define DYNEMIT_COMPILER_H

#include <dynemit/err.h>

/**
 * @file compiler.h
 * @brief Compiler portability macros for dynemit
 *
 * Abstracts compiler-specific constructs so the library builds cleanly
 * with both GCC (13+) and Clang (16+).
 */

/* Detect compiler */
#if defined(__clang__)
#  define DYNEMIT_COMPILER_CLANG 1
#elif defined(__GNUC__)
#  define DYNEMIT_COMPILER_GCC 1
#endif

/*
 * DYNEMIT_NO_AUTOVECTORIZE: disable auto-vectorization on a function.
 * - GCC: __attribute__((optimize("no-tree-vectorize")))
 * - Clang: __attribute__((optnone)) is too aggressive; instead we use
 *   a pragma inside the function body. So we provide a loop-level pragma macro.
 *
 * Usage:
 *   __attribute__((target("default")))
 *   DYNEMIT_NO_AUTOVECTORIZE
 *   static void my_func_scalar(float *out, const float *in, size_t n)
 *   {
 *       DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
 *       for (size_t i = 0; i < n; i++)
 *           out[i] = in[i] * 2.0f;
 *   }
 *
 * On GCC the function-level attribute handles everything and the pragma
 * macros expand to nothing.  On Clang the function-level macro is empty
 * and the pragma disables vectorization on the immediately following loop.
 */
#ifdef DYNEMIT_COMPILER_GCC
#  define DYNEMIT_NO_AUTOVECTORIZE __attribute__((optimize("no-tree-vectorize")))
#  define DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
#  define DYNEMIT_PRAGMA_NO_VECTORIZE_END
#elif defined(DYNEMIT_COMPILER_CLANG)
#  define DYNEMIT_NO_AUTOVECTORIZE
#  define DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN \
       _Pragma("clang loop vectorize(disable) interleave(disable)")
#  define DYNEMIT_PRAGMA_NO_VECTORIZE_END
#else
#  define DYNEMIT_NO_AUTOVECTORIZE
#  define DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
#  define DYNEMIT_PRAGMA_NO_VECTORIZE_END
#endif

/*
 * IFUNC dispatch is incompatible with ASan/TSan/UBSan: resolvers run during
 * ELF relocation before sanitizer runtimes initialize.  When DYNEMIT_NO_IFUNC
 * is set (e.g. via DYNEMIT_SANITIZE builds), public APIs use constructor-time
 * resolver calls instead of GNU ifunc.
 */
#if defined(DYNEMIT_NO_IFUNC)
#  define DYNEMIT_IFUNC_SETUP(fn_t, api, resolver)                        \
    static fn_t api##_dynemit_fn;                                           \
    __attribute__((constructor))                                            \
    static void api##_dynemit_init(void) { api##_dynemit_fn = resolver(); }
#  define DYNEMIT_IFUNC_ATTR(resolver)
#  define DYNEMIT_IFUNC_INVOKE(api, args) api##_dynemit_fn args
#  define DYNEMIT_TARGET_DEFAULT
#else
#  define DYNEMIT_IFUNC_SETUP(fn_t, api, resolver)
#  define DYNEMIT_IFUNC_ATTR(resolver) __attribute__((ifunc(resolver)))
#  define DYNEMIT_IFUNC_INVOKE(api, args)
#  define DYNEMIT_TARGET_DEFAULT __attribute__((target("default")))
#endif

#endif /* DYNEMIT_COMPILER_H */
