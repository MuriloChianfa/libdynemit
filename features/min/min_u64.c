/* SPDX-License-Identifier: BSL-1.0 */
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <dynemit/min.h>
#include <dynemit/compiler.h>

__attribute__((target("default")))
DYNEMIT_NO_AUTOVECTORIZE
static double
min_u64_scalar(const uint64_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint64_t result = data[0];
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 1; i < n; i++)
        if (data[i] < result) result = data[i];
    return (double)result;
}

__attribute__((target("sse2")))
static double
min_u64_sse2(const uint64_t *data, size_t n)
{
    return min_u64_scalar(data, n);
}

__attribute__((target("sse4.2")))
static double
min_u64_sse42(const uint64_t *data, size_t n)
{
    return min_u64_scalar(data, n);
}

__attribute__((target("avx")))
static double
min_u64_avx(const uint64_t *data, size_t n)
{
    return min_u64_scalar(data, n);
}

__attribute__((target("avx2")))
static double
min_u64_avx2(const uint64_t *data, size_t n)
{
    return min_u64_scalar(data, n);
}

__attribute__((target("avx512f")))
static double
min_u64_avx512f(const uint64_t *data, size_t n)
{
    if (n == 0) return 0.0;
    size_t i = 0;
    __m512i vmin = _mm512_set1_epi64((long long)UINT64_MAX);
    for (; i + 8 <= n; i += 8)
        vmin = _mm512_min_epu64(vmin, _mm512_loadu_si512(data + i));
    uint64_t result = _mm512_reduce_min_epu64(vmin);
    for (; i < n; i++)
        if (data[i] < result) result = data[i];
    return (double)result;
}

min_u64_fn_t
min_u64_select(simd_level_t level)
{
    switch (level) {
    case SIMD_AVX512F: return min_u64_avx512f;
    case SIMD_AVX2:    return min_u64_avx2;
    case SIMD_AVX:     return min_u64_avx;
    case SIMD_SSE4_2:  return min_u64_sse42;
    case SIMD_SSE2:    return min_u64_sse2;
    case SIMD_SCALAR:
    default:           return min_u64_scalar;
    }
}

static min_u64_fn_t
min_u64_resolver(void)
{
    return min_u64_select(detect_simd_level());
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
double min_u64(const uint64_t *data, size_t n)
    __attribute__((ifunc("min_u64_resolver")));
