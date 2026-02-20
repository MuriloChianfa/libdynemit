/* SPDX-License-Identifier: BSL-1.0 */
#include <immintrin.h>
#include <stddef.h>
#include <stdint.h>
#include <dynemit/sum.h>
#include <dynemit/compiler.h>

__attribute__((target("default")))
DYNEMIT_NO_AUTOVECTORIZE
static double
sum_u64_scalar(const uint64_t *data, size_t n)
{
    double sum = 0.0;
    DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++)
        sum += (double)data[i];
    return sum;
}

__attribute__((target("sse2")))
static double
sum_u64_sse2(const uint64_t *data, size_t n)
{
    return sum_u64_scalar(data, n);
}

__attribute__((target("sse4.2")))
static double
sum_u64_sse42(const uint64_t *data, size_t n)
{
    return sum_u64_scalar(data, n);
}

__attribute__((target("avx")))
static double
sum_u64_avx(const uint64_t *data, size_t n)
{
    return sum_u64_scalar(data, n);
}

__attribute__((target("avx2")))
static double
sum_u64_avx2(const uint64_t *data, size_t n)
{
    return sum_u64_scalar(data, n);
}

__attribute__((target("avx512f")))
static double
sum_u64_avx512f(const uint64_t *data, size_t n)
{
    size_t i = 0;
    __m512i vsum = _mm512_setzero_si512();
    for (; i + 8 <= n; i += 8)
        vsum = _mm512_add_epi64(vsum, _mm512_loadu_si512(data + i));
    alignas(64) uint64_t tmp[8];
    _mm512_storeu_si512(tmp, vsum);
    double sum = 0.0;
    for (int j = 0; j < 8; j++)
        sum += (double)tmp[j];
    for (; i < n; i++)
        sum += (double)data[i];
    return sum;
}

sum_u64_fn_t
sum_u64_select(simd_level_t level)
{
    switch (level) {
    case SIMD_AVX512F: return sum_u64_avx512f;
    case SIMD_AVX2:    return sum_u64_avx2;
    case SIMD_AVX:     return sum_u64_avx;
    case SIMD_SSE4_2:  return sum_u64_sse42;
    case SIMD_SSE2:    return sum_u64_sse2;
    case SIMD_SCALAR:
    default:           return sum_u64_scalar;
    }
}

static sum_u64_fn_t
sum_u64_resolver(void)
{
    return sum_u64_select(detect_simd_level());
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
double sum_u64(const uint64_t *data, size_t n)
    __attribute__((ifunc("sum_u64_resolver")));
