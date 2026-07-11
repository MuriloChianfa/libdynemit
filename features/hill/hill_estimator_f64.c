/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elifdef __aarch64__
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <dynemit/compiler.h>
#include <dynemit/hill.h>
#include <math.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Hill estimator for heavy-tail detection.
 * Given sorted descending counts, computes:
 *   mean(log(sorted[0..k-1] / sorted[k])) where k = min(n/4, 100)
 * Returns 0.0 if k < 2 or sorted[k] == 0.
 */

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
hill_estimator_f64_scalar(const uint64_t *sorted_desc, size_t n)
{
    if (n < 4) {
        return 0.0;
    }
    size_t k = n / 4;
    if (k > 100) {
        k = 100;
    }
    if (k < 2) {
        return 0.0;
    }
    if (sorted_desc[k] == 0) {
        return 0.0;
    }

    double threshold = (double)sorted_desc[k];
    double log_sum = 0.0;
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < k; i++) {
        log_sum += log((double)sorted_desc[i] / threshold);
    }
    return log_sum / (double)k;
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static double
hill_estimator_f64_sse2(const uint64_t *sorted_desc, size_t n)
{
    return hill_estimator_f64_scalar(sorted_desc, n);
}

__attribute__((target("sse4.2")))
static double
hill_estimator_f64_sse42(const uint64_t *sorted_desc, size_t n)
{
    return hill_estimator_f64_scalar(sorted_desc, n);
}

__attribute__((target("avx")))
static double
hill_estimator_f64_avx(const uint64_t *sorted_desc, size_t n)
{
    return hill_estimator_f64_scalar(sorted_desc, n);
}

__attribute__((target("avx2")))
static double
hill_estimator_f64_avx2(const uint64_t *sorted_desc, size_t n)
{
    return hill_estimator_f64_scalar(sorted_desc, n);
}

__attribute__((target("avx512f")))
static double
hill_estimator_f64_avx512f(const uint64_t *sorted_desc, size_t n)
{
    return hill_estimator_f64_scalar(sorted_desc, n);
}
#endif

#ifdef __aarch64__

static double
hill_estimator_f64_neon(const uint64_t *sorted_desc, size_t n)
{
    return hill_estimator_f64_scalar(sorted_desc, n);
}

__attribute__((target("+sve")))
static double
hill_estimator_f64_sve(const uint64_t *sorted_desc, size_t n)
{
    return hill_estimator_f64_scalar(sorted_desc, n);
}

__attribute__((target("+sve2")))
static double
hill_estimator_f64_sve2(const uint64_t *sorted_desc, size_t n)
{
    return hill_estimator_f64_scalar(sorted_desc, n);
}

#endif /* aarch64 */

hill_estimator_f64_fn_t
hill_estimator_f64_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return hill_estimator_f64_avx512f;
    case SIMD_AVX2:    return hill_estimator_f64_avx2;
    case SIMD_AVX:     return hill_estimator_f64_avx;
    case SIMD_SSE4_2:  return hill_estimator_f64_sse42;
    case SIMD_SSE2:    return hill_estimator_f64_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return hill_estimator_f64_sve2;
    case SIMD_SVE:     return hill_estimator_f64_sve;
    case SIMD_NEON:    return hill_estimator_f64_neon;
#endif
    case SIMD_SCALAR:
    default:           return hill_estimator_f64_scalar;
}
}

EXPLICIT_RUNTIME_RESOLVER(hill_estimator_f64_resolver, hill_estimator_f64_fn_t)
{
    return hill_estimator_f64_select(detect_simd_level_ts());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
double hill_estimator_f64(const uint64_t *sorted_desc, size_t n)
    __attribute__((ifunc("hill_estimator_f64_resolver")));
