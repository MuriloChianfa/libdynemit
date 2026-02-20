/* SPDX-License-Identifier: BSL-1.0 */
#include <math.h>
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <dynemit/entropy.h>
#include <dynemit/compiler.h>

__attribute__((target("default")))
DYNEMIT_NO_AUTOVECTORIZE
static double
entropy_histogram_scalar(const uint64_t *counts, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t total = 0;
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++)
        total += counts[i];
    if (total == 0) return 0.0;
    double inv_total = 1.0 / (double)total;
    double h = 0.0;
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        if (counts[i] == 0) continue;
        double p = (double)counts[i] * inv_total;
        h -= p * log2(p);
    }
    return h;
}

__attribute__((target("sse2")))
static double
entropy_histogram_sse2(const uint64_t *counts, size_t n)
{
    return entropy_histogram_scalar(counts, n);
}

__attribute__((target("sse4.2")))
static double
entropy_histogram_sse42(const uint64_t *counts, size_t n)
{
    return entropy_histogram_scalar(counts, n);
}

__attribute__((target("avx")))
static double
entropy_histogram_avx(const uint64_t *counts, size_t n)
{
    return entropy_histogram_scalar(counts, n);
}

__attribute__((target("avx2")))
static double
entropy_histogram_avx2(const uint64_t *counts, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m256i vsum = _mm256_setzero_si256();
    for (; i + 4 <= n; i += 4)
        vsum = _mm256_add_epi64(vsum, _mm256_loadu_si256((const __m256i *)(counts + i)));
    alignas(32) uint64_t tmp[4];
    _mm256_storeu_si256((__m256i *)tmp, vsum);
    uint64_t total = tmp[0] + tmp[1] + tmp[2] + tmp[3];
    for (; i < n; i++)
        total += counts[i];
    if (total == 0) return 0.0;
    double inv_total = 1.0 / (double)total;
    double h = 0.0;
    for (size_t j = 0; j < n; j++) {
        if (counts[j] == 0) continue;
        double p = (double)counts[j] * inv_total;
        h -= p * log2(p);
    }
    return h;
}

__attribute__((target("avx512f")))
static double
entropy_histogram_avx512f(const uint64_t *counts, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m512i vsum = _mm512_setzero_si512();
    for (; i + 8 <= n; i += 8)
        vsum = _mm512_add_epi64(vsum, _mm512_loadu_si512(counts + i));
    alignas(64) uint64_t tmp[8];
    _mm512_storeu_si512(tmp, vsum);
    uint64_t total = 0;
    for (size_t j = 0; j < 8; j++)
        total += tmp[j];
    for (; i < n; i++)
        total += counts[i];
    if (total == 0) return 0.0;
    double inv_total = 1.0 / (double)total;
    double h = 0.0;
    for (size_t j = 0; j < n; j++) {
        if (counts[j] == 0) continue;
        double p = (double)counts[j] * inv_total;
        h -= p * log2(p);
    }
    return h;
}

entropy_histogram_fn_t
entropy_histogram_select(simd_level_t level)
{
    switch (level) {
    case SIMD_AVX512F: return entropy_histogram_avx512f;
    case SIMD_AVX2:    return entropy_histogram_avx2;
    case SIMD_AVX:     return entropy_histogram_avx;
    case SIMD_SSE4_2:  return entropy_histogram_sse42;
    case SIMD_SSE2:    return entropy_histogram_sse2;
    case SIMD_SCALAR:
    default:           return entropy_histogram_scalar;
    }
}

static entropy_histogram_fn_t
entropy_histogram_resolver(void)
{
    return entropy_histogram_select(detect_simd_level());
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
double entropy_histogram(const uint64_t *counts, size_t n)
    __attribute__((ifunc("entropy_histogram_resolver")));
