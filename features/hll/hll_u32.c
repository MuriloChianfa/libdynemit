/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#  include <immintrin.h>
#elif defined(__aarch64__)
#  include <arm_neon.h>
#  include <arm_sve.h>
#endif

#include <stddef.h>
#include <stdint.h>

#include <dynemit/compiler.h>
#include <dynemit/hll.h>

#include "hll.h"

#if !DYNEMIT_TS
uint8_t *hll_regs_tls = NULL;
#endif

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
hll_u32_scalar(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint8_t *regs = hll_get_regs();
    if (!regs) return 0.0;

DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        uint64_t h   = hll_mix64((uint64_t)data[i]);
        uint32_t idx = hll_idx(h);
        uint8_t  r   = (uint8_t)hll_rank(h);
        regs[idx] = hll_max_u8(regs[idx], r);
    }

    double est = hll_finalize_scalar(regs);
    hll_reset_regs(regs);
    return est;
}

#if defined(__x86_64__) || defined(__i386__)

__attribute__((target("sse2")))
static double
hll_u32_sse2(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint8_t *regs = hll_get_regs();
    if (!regs) return 0.0;

    for (size_t i = 0; i < n; i++) {
        uint64_t h   = hll_mix64((uint64_t)data[i]);
        uint32_t idx = hll_idx(h);
        uint8_t  r   = (uint8_t)hll_rank(h);
        regs[idx] = hll_max_u8(regs[idx], r);
    }

    double est = hll_finalize_sse2(regs);
    hll_reset_regs(regs);
    return est;
}

__attribute__((target("sse4.2")))
static double
hll_u32_sse42(const uint32_t *data, size_t n)
{
    return hll_u32_sse2(data, n);
}

__attribute__((target("avx")))
static double
hll_u32_avx(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint8_t *regs = hll_get_regs();
    if (!regs) return 0.0;

    for (size_t i = 0; i < n; i++) {
        uint64_t h   = hll_mix64((uint64_t)data[i]);
        uint32_t idx = hll_idx(h);
        uint8_t  r   = (uint8_t)hll_rank(h);
        regs[idx] = hll_max_u8(regs[idx], r);
    }

    double est = hll_finalize_avx(regs);
    hll_reset_regs(regs);
    return est;
}

__attribute__((target("avx2,fma")))
static double
hll_u32_avx2(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint8_t *regs = hll_get_regs();
    if (!regs) return 0.0;

    for (size_t i = 0; i < n; i++) {
        uint64_t h   = hll_mix64((uint64_t)data[i]);
        uint32_t idx = hll_idx(h);
        uint8_t  r   = (uint8_t)hll_rank(h);
        regs[idx] = hll_max_u8(regs[idx], r);
    }

    double est = hll_finalize_avx2(regs);
    hll_reset_regs(regs);
    return est;
}

__attribute__((target("avx512f")))
static double
hll_u32_avx512f(const uint32_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint8_t *regs = hll_get_regs();
    if (!regs) return 0.0;

    for (size_t i = 0; i < n; i++) {
        uint64_t h   = hll_mix64((uint64_t)data[i]);
        uint32_t idx = hll_idx(h);
        uint8_t  r   = (uint8_t)hll_rank(h);
        regs[idx] = hll_max_u8(regs[idx], r);
    }

    double est = hll_finalize_avx512(regs);
    hll_reset_regs(regs);
    return est;
}

#endif /* x86 */

#if defined(__aarch64__)

static double
hll_u32_neon(const uint32_t *data, size_t n)
{
    return hll_u32_scalar(data, n);
}

__attribute__((target("+sve")))
static double
hll_u32_sve(const uint32_t *data, size_t n)
{
    return hll_u32_scalar(data, n);
}

__attribute__((target("+sve2")))
static double
hll_u32_sve2(const uint32_t *data, size_t n)
{
    return hll_u32_scalar(data, n);
}

#endif /* aarch64 */

hll_u32_fn_t
hll_u32_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return hll_u32_avx512f;
    case SIMD_AVX2:    return hll_u32_avx2;
    case SIMD_AVX:     return hll_u32_avx;
    case SIMD_SSE4_2:  return hll_u32_sse42;
    case SIMD_SSE2:    return hll_u32_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return hll_u32_sve2;
    case SIMD_SVE:     return hll_u32_sve;
    case SIMD_NEON:    return hll_u32_neon;
#endif
    case SIMD_SCALAR:
    default:           return hll_u32_scalar;
    }
}

static hll_u32_fn_t
hll_u32_resolver(void)
{
    return hll_u32_select(detect_simd_level());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
double hll_u32(const uint32_t *data, size_t n)
    __attribute__((ifunc("hll_u32_resolver")));
