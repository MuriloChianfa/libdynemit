/* SPDX-License-Identifier: BSL-1.0 */
#include <stddef.h>
#include <stdint.h>
#include <dynemit/compiler.h>
#include <dynemit/concentration.h>
#include <dynemit/hhi.h>

/*
 * Composite concentration analysis.
 * Calls topk_ratios_f64, hill_estimator_f64, and hhi_histogram
 * to populate a concentration_result_t.
 */

__attribute__((target("default")))
DYNEMIT_NO_AUTOVECTORIZE
static void
concentration_f64_scalar(const uint64_t *sorted_counts_desc, size_t n,
                        uint64_t total,
                        const size_t *k_values, size_t num_k,
                        concentration_result_t *out)
{
    topk_ratios_f64(sorted_counts_desc, n, total, k_values, num_k, out->topk_ratios);
    out->heavy_tail_index = hill_estimator_f64(sorted_counts_desc, n);
    out->concentration    = hhi_histogram(sorted_counts_desc, n);
}

__attribute__((target("sse2")))
static void
concentration_f64_sse2(const uint64_t *sorted_counts_desc, size_t n,
                      uint64_t total,
                      const size_t *k_values, size_t num_k,
                      concentration_result_t *out)
{
    concentration_f64_scalar(sorted_counts_desc, n, total, k_values, num_k, out);
}

__attribute__((target("sse4.2")))
static void
concentration_f64_sse42(const uint64_t *sorted_counts_desc, size_t n,
                       uint64_t total,
                       const size_t *k_values, size_t num_k,
                       concentration_result_t *out)
{
    concentration_f64_scalar(sorted_counts_desc, n, total, k_values, num_k, out);
}

__attribute__((target("avx")))
static void
concentration_f64_avx(const uint64_t *sorted_counts_desc, size_t n,
                     uint64_t total,
                     const size_t *k_values, size_t num_k,
                     concentration_result_t *out)
{
    concentration_f64_scalar(sorted_counts_desc, n, total, k_values, num_k, out);
}

__attribute__((target("avx2")))
static void
concentration_f64_avx2(const uint64_t *sorted_counts_desc, size_t n,
                      uint64_t total,
                      const size_t *k_values, size_t num_k,
                      concentration_result_t *out)
{
    concentration_f64_scalar(sorted_counts_desc, n, total, k_values, num_k, out);
}

__attribute__((target("avx512f")))
static void
concentration_f64_avx512f(const uint64_t *sorted_counts_desc, size_t n,
                         uint64_t total,
                         const size_t *k_values, size_t num_k,
                         concentration_result_t *out)
{
    concentration_f64_scalar(sorted_counts_desc, n, total, k_values, num_k, out);
}

concentration_f64_fn_t
concentration_f64_select(simd_level_t level)
{
    switch (level) {
    case SIMD_AVX512F: return concentration_f64_avx512f;
    case SIMD_AVX2:    return concentration_f64_avx2;
    case SIMD_AVX:     return concentration_f64_avx;
    case SIMD_SSE4_2:  return concentration_f64_sse42;
    case SIMD_SSE2:    return concentration_f64_sse2;
    case SIMD_SCALAR:
    default:           return concentration_f64_scalar;
    }
}

static concentration_f64_fn_t
concentration_f64_resolver(void)
{
    return concentration_f64_select(detect_simd_level());
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
void concentration_f64(const uint64_t *sorted_counts_desc, size_t n,
                      uint64_t total,
                      const size_t *k_values, size_t num_k,
                      concentration_result_t *out)
    __attribute__((ifunc("concentration_f64_resolver")));
