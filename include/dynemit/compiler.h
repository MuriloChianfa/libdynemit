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

#endif /* DYNEMIT_COMPILER_H */
