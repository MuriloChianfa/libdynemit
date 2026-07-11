/* SPDX-License-Identifier: BSL-1.0 */
#include <dynemit/compiler.h>
#include <dynemit/mean.h>
#include <dynemit/sum.h>
#include <stddef.h>
#include <stdint.h>

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
mean_u64_impl(const uint64_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    return sum_u64(data, n) / (double)n;
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static double mean_u64_sse2(const uint64_t *d, size_t n) { return mean_u64_impl(d, n); }
__attribute__((target("sse4.2")))
static double mean_u64_sse42(const uint64_t *d, size_t n) { return mean_u64_impl(d, n); }
__attribute__((target("avx")))
static double mean_u64_avx(const uint64_t *d, size_t n) { return mean_u64_impl(d, n); }
__attribute__((target("avx2")))
static double mean_u64_avx2(const uint64_t *d, size_t n) { return mean_u64_impl(d, n); }
__attribute__((target("avx512f")))
static double mean_u64_avx512f(const uint64_t *d, size_t n) { return mean_u64_impl(d, n); }
#endif

#ifdef __aarch64__
static double mean_u64_neon(const uint64_t *d, size_t n) { return mean_u64_impl(d, n); }
__attribute__((target("+sve")))
static double mean_u64_sve(const uint64_t *d, size_t n) { return mean_u64_impl(d, n); }
__attribute__((target("+sve2")))
static double mean_u64_sve2(const uint64_t *d, size_t n) { return mean_u64_impl(d, n); }
#endif

mean_u64_fn_t
mean_u64_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return mean_u64_avx512f;
    case SIMD_AVX2:    return mean_u64_avx2;
    case SIMD_AVX:     return mean_u64_avx;
    case SIMD_SSE4_2:  return mean_u64_sse42;
    case SIMD_SSE2:    return mean_u64_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return mean_u64_sve2;
    case SIMD_SVE:     return mean_u64_sve;
    case SIMD_NEON:    return mean_u64_neon;
#endif
    case SIMD_SCALAR:
    default:           return mean_u64_impl;
}
}

EXPLICIT_RUNTIME_RESOLVER(mean_u64_resolver, mean_u64_fn_t)
{
    return mean_u64_select(detect_simd_level_ts());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
double mean_u64(const uint64_t *data, size_t n)
    __attribute__((ifunc("mean_u64_resolver")));
