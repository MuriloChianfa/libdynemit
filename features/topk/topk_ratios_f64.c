/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elifdef __aarch64__
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <dynemit/compiler.h>
#include <dynemit/topk.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Compute top-K concentration ratios from pre-sorted descending frequency counts.
 * out_ratios[j] = sum(sorted_desc[0..k_values[j]-1]) / total
 */

#if defined(__x86_64__) || defined(__i386__)
DYNEMIT_TARGET_DEFAULT
#endif
DYNEMIT_NO_AUTOVECTORIZE
static void
topk_ratios_f64_scalar(const uint64_t *sorted_desc, size_t n,
                       uint64_t total,
                       const size_t *k_values, size_t num_k,
                       double *out_ratios)
{
    double dtotal = (double)total;
    for (size_t j = 0; j < num_k; j++) {
        size_t k = k_values[j];
        if (k > n) {
            k = n;
        }
        double partial = 0.0;
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
        for (size_t i = 0; i < k; i++) {
            partial += (double)sorted_desc[i];
        }
        out_ratios[j] = (dtotal > 0.0) ? partial / dtotal : 0.0;
    }
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static void
topk_ratios_f64_sse2(const uint64_t *sorted_desc, size_t n,
                     uint64_t total,
                     const size_t *k_values, size_t num_k,
                     double *out_ratios)
{
    topk_ratios_f64_scalar(sorted_desc, n, total, k_values, num_k, out_ratios);
}

__attribute__((target("sse4.2")))
static void
topk_ratios_f64_sse42(const uint64_t *sorted_desc, size_t n,
                      uint64_t total,
                      const size_t *k_values, size_t num_k,
                      double *out_ratios)
{
    topk_ratios_f64_scalar(sorted_desc, n, total, k_values, num_k, out_ratios);
}

__attribute__((target("avx")))
static void
topk_ratios_f64_avx(const uint64_t *sorted_desc, size_t n,
                    uint64_t total,
                    const size_t *k_values, size_t num_k,
                    double *out_ratios)
{
    topk_ratios_f64_scalar(sorted_desc, n, total, k_values, num_k, out_ratios);
}

__attribute__((target("avx2")))
static void
topk_ratios_f64_avx2(const uint64_t *sorted_desc, size_t n,
                     uint64_t total,
                     const size_t *k_values, size_t num_k,
                     double *out_ratios)
{
    topk_ratios_f64_scalar(sorted_desc, n, total, k_values, num_k, out_ratios);
}

__attribute__((target("avx512f")))
static void
topk_ratios_f64_avx512f(const uint64_t *sorted_desc, size_t n,
                        uint64_t total,
                        const size_t *k_values, size_t num_k,
                        double *out_ratios)
{
    double dtotal = (double)total;
    for (size_t j = 0; j < num_k; j++) {
        size_t k = k_values[j];
        if (k > n) {
            k = n;
        }
        __m512d vsum = _mm512_setzero_pd();
        size_t i = 0;
        for (; i + 8 <= k; i += 8) {
            __m512d vdata = _mm512_set_pd(
                (double)sorted_desc[i + 7], (double)sorted_desc[i + 6],
                (double)sorted_desc[i + 5], (double)sorted_desc[i + 4],
                (double)sorted_desc[i + 3], (double)sorted_desc[i + 2],
                (double)sorted_desc[i + 1], (double)sorted_desc[i]);
            vsum = _mm512_add_pd(vsum, vdata);
        }
        double partial = _mm512_reduce_add_pd(vsum);
        for (; i < k; i++) {
            partial += (double)sorted_desc[i];
        }
        out_ratios[j] = (dtotal > 0.0) ? partial / dtotal : 0.0;
    }
}
#endif

#ifdef __aarch64__

static void
topk_ratios_f64_neon(const uint64_t *sorted_desc, size_t n,
                     uint64_t total,
                     const size_t *k_values, size_t num_k,
                     double *out_ratios)
{
    topk_ratios_f64_scalar(sorted_desc, n, total, k_values, num_k, out_ratios);
}

__attribute__((target("+sve")))
static void
topk_ratios_f64_sve(const uint64_t *sorted_desc, size_t n,
                    uint64_t total,
                    const size_t *k_values, size_t num_k,
                    double *out_ratios)
{
    topk_ratios_f64_scalar(sorted_desc, n, total, k_values, num_k, out_ratios);
}

__attribute__((target("+sve2")))
static void
topk_ratios_f64_sve2(const uint64_t *sorted_desc, size_t n,
                     uint64_t total,
                     const size_t *k_values, size_t num_k,
                     double *out_ratios)
{
    topk_ratios_f64_scalar(sorted_desc, n, total, k_values, num_k, out_ratios);
}

#endif /* aarch64 */

topk_ratios_f64_fn_t
topk_ratios_f64_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return topk_ratios_f64_avx512f;
    case SIMD_AVX2:    return topk_ratios_f64_avx2;
    case SIMD_AVX:     return topk_ratios_f64_avx;
    case SIMD_SSE4_2:  return topk_ratios_f64_sse42;
    case SIMD_SSE2:    return topk_ratios_f64_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return topk_ratios_f64_sve2;
    case SIMD_SVE:     return topk_ratios_f64_sve;
    case SIMD_NEON:    return topk_ratios_f64_neon;
#endif
    case SIMD_SCALAR:
    default:           return topk_ratios_f64_scalar;
}
}

EXPLICIT_RUNTIME_RESOLVER(topk_ratios_f64_resolver, topk_ratios_f64_fn_t)
{
    return topk_ratios_f64_select(detect_simd_level_ts());
}
DYNEMIT_IFUNC_SETUP(topk_ratios_f64_fn_t, topk_ratios_f64, topk_ratios_f64_resolver)

#if defined(DYNEMIT_NO_IFUNC)
void topk_ratios_f64(const uint64_t *sorted_desc, size_t n,
                     uint64_t total,
                     const size_t *k_values, size_t num_k,
                     double *out_ratios)
{
    DYNEMIT_IFUNC_INVOKE(topk_ratios_f64, (sorted_desc, n, total, k_values, num_k, out_ratios));
}
#else
#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
void topk_ratios_f64(const uint64_t *sorted_desc, size_t n,
                     uint64_t total,
                     const size_t *k_values, size_t num_k,
                     double *out_ratios)
    DYNEMIT_IFUNC_ATTR("topk_ratios_f64_resolver");
#endif
