/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_HLL_INTERNAL_H
#define DYNEMIT_HLL_INTERNAL_H

/*
 * HLL internal building blocks (shared by hll_u32.c and hll_u64.c).
 *
 * This is the FEATURE-LOCAL hll.h (include via "hll.h" with quotes).  The
 * PUBLIC API lives in <dynemit/hll.h>.  The two never collide because the
 * .c files use the quoted form (resolves to this file in the feature
 * directory) for internals and the angle form for the public prototypes.
 *
 * All helpers are branchless on the hot update path:
 *
 *   hash  -> index  -> rank  -> register max-update
 *
 * The SIMD finalizers below all compute
 *
 *   sum  = Sigma_i 2^(-r[i])
 *   V    = |{i : r[i] == 0}|
 *
 * and defer to hll_apply_correction() for the HLL++ alpha/LC decision,
 * so every variant produces bit-identical output on the same register
 * array (register updates are deterministic: same hash, same max-chain).
 */

#if defined(__x86_64__) || defined(__i386__)
#  include <immintrin.h>
#elif defined(__aarch64__)
#  include <arm_neon.h>
#  include <arm_sve.h>
#endif

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bias.h"

#include <dynemit/compiler.h>

#ifndef DYNEMIT_HLL_P
#  define DYNEMIT_HLL_P 10
#endif

#if DYNEMIT_HLL_P < 4 || DYNEMIT_HLL_P > 18
#  error "DYNEMIT_HLL_P must be in [4, 18]"
#endif

#define DYNEMIT_HLL_M         (1u << DYNEMIT_HLL_P)
#define DYNEMIT_HLL_Q         (64u - DYNEMIT_HLL_P)
#define DYNEMIT_HLL_MAX_RANK  (DYNEMIT_HLL_Q + 1u)

/*
 * SplitMix64 (Stafford's variant 13) - avalanches a 64-bit integer into
 * a uniformly distributed 64-bit output with 3 xorshift/multiply rounds.
 * Pure arithmetic, zero branches, zero memory access.
 */
static inline uint64_t
hll_mix64(uint64_t x)
{
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

/* Top p bits of the hash pick the register index. */
static inline uint32_t
hll_idx(uint64_t h)
{
    return (uint32_t)(h >> DYNEMIT_HLL_Q);
}

/*
 * Rank = 1 + position of the first 1-bit in the remaining q = 64-p bits.
 * Shift the hash left by p so the remaining bits are top-aligned, then
 * OR in a sentinel at bit (p-1) so when all q bits are zero (probability
 * 2^-q) clz saturates at q, giving rank = q + 1 = DYNEMIT_HLL_MAX_RANK.
 * Entirely branchless.
 */
static inline uint32_t
hll_rank(uint64_t h)
{
    uint64_t w = (h << DYNEMIT_HLL_P) | (1ULL << (DYNEMIT_HLL_P - 1));
    return (uint32_t)(__builtin_clzll(w) + 1);
}

/* Branchless unsigned 8-bit max via xor-diff mask. */
static inline uint8_t
hll_max_u8(uint8_t a, uint8_t b)
{
    return (uint8_t)(a ^ ((a ^ b) & (uint8_t)(-(int8_t)(a < b))));
}

extern _Thread_local uint8_t *hll_regs_tls;

static inline uint8_t *
hll_get_regs(void)
{
    if (__builtin_expect(!hll_regs_tls, 0)) {
        hll_regs_tls = aligned_alloc(64, DYNEMIT_HLL_M);
        if (!hll_regs_tls) return NULL;
        memset(hll_regs_tls, 0, DYNEMIT_HLL_M);
    }
    return hll_regs_tls;
}

static inline void
hll_reset_regs(uint8_t *regs)
{
    memset(regs, 0, DYNEMIT_HLL_M);
}

/*
 * Every input produces exactly one histogram increment.
 */
static inline void
hll_build_histogram(const uint8_t *regs, unsigned *histo)
{
    for (unsigned i = 0; i <= DYNEMIT_HLL_MAX_RANK; i++) histo[i] = 0;
    for (unsigned i = 0; i < DYNEMIT_HLL_M; i++) histo[regs[i]]++;
}

#if defined(__x86_64__) || defined(__i386__)

__attribute__((target("sse2"), always_inline))
static inline void
hll_build_histogram_sse2(const uint8_t *regs, unsigned *histo)
{
    hll_build_histogram(regs, histo);
}

__attribute__((target("avx"), always_inline))
static inline void
hll_build_histogram_avx(const uint8_t *regs, unsigned *histo)
{
    hll_build_histogram(regs, histo);
}

__attribute__((target("avx2"), always_inline))
static inline void
hll_build_histogram_avx2(const uint8_t *regs, unsigned *histo)
{
    hll_build_histogram(regs, histo);
}

__attribute__((target("avx512f"), always_inline))
static inline void
hll_build_histogram_avx512(const uint8_t *regs, unsigned *histo)
{
    hll_build_histogram(regs, histo);
}

#endif /* x86 */

/*
 * Ertl's improved estimator (Otmar Ertl 2017, used by Redis)
 *
 * Uses HLL_ALPHA_INF = 1 / (2 ln 2) from bias.h (the "alpha" constant
 * that Ertl's formulation applies uniformly for all m - the tau/sigma
 * terms already capture the small- and large-range biases that the
 * Flajolet alpha_m correction used to handle).
 *
 * Converges quickly, the loop terminates as soon as the partial sum
 * stops moving in double precision.
 */
static inline double
hll_sigma(double x)
{
    if (x == 1.0) return INFINITY;
    double z_prev, y = 1.0, z = x;
    do {
        x = x * x;
        z_prev = z;
        z += x * y;
        y += y;
    } while (z_prev != z);
    return z;
}

/*
 * Same fixed-point convergence as sigma().
 */
static inline double
hll_tau(double x)
{
    if (x == 0.0 || x == 1.0) return 0.0;
    double z_prev, y = 1.0, z = 1.0 - x;
    do {
        x = sqrt(x);
        z_prev = z;
        y *= 0.5;
        double d = 1.0 - x;
        z -= d * d * y;
    } while (z_prev != z);
    return z / 3.0;
}

/*
 * Scalar estimator operating on the register histogram.
 */
static inline double
hll_estimate_from_histogram(const unsigned *histo)
{
    double m_d = (double)DYNEMIT_HLL_M;
    double z = m_d * hll_tau((m_d - (double)histo[DYNEMIT_HLL_MAX_RANK]) / m_d);
    for (unsigned k = DYNEMIT_HLL_Q; k >= 1; k--) {
        z += (double)histo[k];
        z *= 0.5;
    }
    z += m_d * hll_sigma((double)histo[0] / m_d);
    return HLL_ALPHA_INF * m_d * m_d / z;
}

/*
 * The scalar finalizer goes through hll_build_histogram + the scalar
 * estimator.  DYNEMIT_NO_AUTOVECTORIZE on the caller (scalar variant
 * wrapper) plus the pragma below keep Clang and GCC from auto-
 * vectorizing the histogram scan, so the scalar variant is truly
 * scalar.
 */
static inline double
hll_finalize_scalar(const uint8_t *regs)
{
    unsigned histo[DYNEMIT_HLL_MAX_RANK + 1];
    for (unsigned i = 0; i <= DYNEMIT_HLL_MAX_RANK; i++) histo[i] = 0;
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (unsigned i = 0; i < DYNEMIT_HLL_M; i++) histo[regs[i]]++;
    return hll_estimate_from_histogram(histo);
}

#if defined(__x86_64__) || defined(__i386__)

__attribute__((target("sse2"), always_inline))
static inline double
hll_finalize_sse2(const uint8_t *regs)
{
    unsigned histo[DYNEMIT_HLL_MAX_RANK + 1];
    hll_build_histogram_sse2(regs, histo);
    return hll_estimate_from_histogram(histo);
}

__attribute__((target("avx"), always_inline))
static inline double
hll_finalize_avx(const uint8_t *regs)
{
    unsigned histo[DYNEMIT_HLL_MAX_RANK + 1];
    hll_build_histogram_avx(regs, histo);
    return hll_estimate_from_histogram(histo);
}

__attribute__((target("avx2,fma"), always_inline))
static inline double
hll_finalize_avx2(const uint8_t *regs)
{
    unsigned histo[DYNEMIT_HLL_MAX_RANK + 1];
    hll_build_histogram_avx2(regs, histo);
    return hll_estimate_from_histogram(histo);
}

__attribute__((target("avx512f"), always_inline))
static inline double
hll_finalize_avx512(const uint8_t *regs)
{
    unsigned histo[DYNEMIT_HLL_MAX_RANK + 1];
    hll_build_histogram_avx512(regs, histo);
    return hll_estimate_from_histogram(histo);
}

#endif /* x86 */

#endif /* DYNEMIT_HLL_INTERNAL_H */
