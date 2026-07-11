/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elifdef __aarch64__
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include "mem.h"
#include <dynemit/compiler.h>
#include <dynemit/hhi.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

static int
cmp_u32(const void *a, const void *b)
{
    uint32_t va = *(const uint32_t *)a;
    uint32_t vb = *(const uint32_t *)b;
    return (va > vb) - (va < vb);
}

#if defined(__x86_64__) || defined(__i386__)
DYNEMIT_TARGET_DEFAULT
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
hhi_u32_scalar(const uint32_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    uint32_t *sorted = (uint32_t *)malloc(n * sizeof(uint32_t));
    if (!sorted) {
        return 0.0;
    }
    if (memcpys(sorted, n * sizeof(uint32_t), data,
                         n * sizeof(uint32_t)) != 0) {
        free(sorted);
        return 0.0;
    }
    qsort(sorted, n, sizeof(uint32_t), cmp_u32);
    double total = (double)n;
    double sum_sq = 0.0;
    size_t i = 0;
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    while (i < n) {
        uint32_t val = sorted[i];
        size_t count = 0;
        while (i < n && sorted[i] == val) {
            count++;
            i++;
        }
        double p = (double)count / total;
        sum_sq += p * p;
    }
    free(sorted);
    return sum_sq;
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static double
hhi_u32_sse2(const uint32_t *data, size_t n)
{
    return hhi_u32_scalar(data, n);
}

__attribute__((target("sse4.2")))
static double
hhi_u32_sse42(const uint32_t *data, size_t n)
{
    return hhi_u32_scalar(data, n);
}

__attribute__((target("avx")))
static double
hhi_u32_avx(const uint32_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    uint32_t *sorted = (uint32_t *)malloc(n * sizeof(uint32_t));
    if (!sorted) {
        return 0.0;
    }
    if (memcpys(sorted, n * sizeof(uint32_t), data,
                         n * sizeof(uint32_t)) != 0) {
        free(sorted);
        return 0.0;
    }
    qsort(sorted, n, sizeof(uint32_t), cmp_u32);

    size_t ndistinct = 0;
    for (size_t j = 0; j < n; ) {
        ndistinct++;
        uint32_t val = sorted[j];
        while (j < n && sorted[j] == val) {
            j++;
        }
    }

    uint64_t *counts = (uint64_t *)malloc(ndistinct * sizeof(uint64_t));
    if (!counts) { free(sorted); return 0.0; }
    size_t ci = 0;
    for (size_t j = 0; j < n; ) {
        uint32_t val = sorted[j];
        uint64_t cnt = 0;
        while (j < n && sorted[j] == val) { cnt++; j++; }
        counts[ci++] = cnt;
    }
    free(sorted);

    double total = (double)n;
    __m256d vtotal = _mm256_set1_pd(total);
    __m256d vsum = _mm256_setzero_pd();
    size_t i = 0;
    for (; i + 4 <= ndistinct; i += 4) {
        __m256d v = _mm256_setr_pd(
            (double)counts[i],     (double)counts[i + 1],
            (double)counts[i + 2], (double)counts[i + 3]);
        __m256d p = _mm256_div_pd(v, vtotal);
        vsum = _mm256_add_pd(vsum, _mm256_mul_pd(p, p));
    }
    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    __m128d s  = _mm_add_pd(lo, hi);
    __m128d sh = _mm_unpackhi_pd(s, s);
    s = _mm_add_pd(s, sh);
    double sum_sq = _mm_cvtsd_f64(s);
    for (; i < ndistinct; i++) {
        double p = (double)counts[i] / total;
        sum_sq += p * p;
    }
    free(counts);
    return sum_sq;
}

__attribute__((target("avx2")))
static double
hhi_u32_avx2(const uint32_t *data, size_t n)
{
    return hhi_u32_avx(data, n);
}

__attribute__((target("avx512f")))
static double
hhi_u32_avx512f(const uint32_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    uint32_t *sorted = (uint32_t *)malloc(n * sizeof(uint32_t));
    if (!sorted) {
        return 0.0;
    }
    if (memcpys(sorted, n * sizeof(uint32_t), data,
                         n * sizeof(uint32_t)) != 0) {
        free(sorted);
        return 0.0;
    }
    qsort(sorted, n, sizeof(uint32_t), cmp_u32);

    size_t ndistinct = 0;
    for (size_t j = 0; j < n; ) {
        ndistinct++;
        uint32_t val = sorted[j];
        while (j < n && sorted[j] == val) {
            j++;
        }
    }

    uint64_t *counts = (uint64_t *)malloc(ndistinct * sizeof(uint64_t));
    if (!counts) { free(sorted); return 0.0; }
    size_t ci = 0;
    for (size_t j = 0; j < n; ) {
        uint32_t val = sorted[j];
        uint64_t cnt = 0;
        while (j < n && sorted[j] == val) { cnt++; j++; }
        counts[ci++] = cnt;
    }
    free(sorted);

    double total = (double)n;
    __m512d vtotal = _mm512_set1_pd(total);
    __m512d vsum = _mm512_setzero_pd();
    size_t i = 0;
    for (; i + 8 <= ndistinct; i += 8) {
        __m512d v = _mm512_setr_pd(
            (double)counts[i],     (double)counts[i + 1],
            (double)counts[i + 2], (double)counts[i + 3],
            (double)counts[i + 4], (double)counts[i + 5],
            (double)counts[i + 6], (double)counts[i + 7]);
        __m512d p = _mm512_div_pd(v, vtotal);
        vsum = _mm512_fmadd_pd(p, p, vsum);
    }
    double sum_sq = _mm512_reduce_add_pd(vsum);
    for (; i < ndistinct; i++) {
        double p = (double)counts[i] / total;
        sum_sq += p * p;
    }
    free(counts);
    return sum_sq;
}
#endif

#ifdef __aarch64__

static double
hhi_u32_neon(const uint32_t *data, size_t n)
{
    return hhi_u32_scalar(data, n);
}

__attribute__((target("+sve")))
static double
hhi_u32_sve(const uint32_t *data, size_t n)
{
    return hhi_u32_scalar(data, n);
}

__attribute__((target("+sve2")))
static double
hhi_u32_sve2(const uint32_t *data, size_t n)
{
    return hhi_u32_scalar(data, n);
}

#endif /* aarch64 */

hhi_u32_fn_t
hhi_u32_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return hhi_u32_avx512f;
    case SIMD_AVX2:    return hhi_u32_avx2;
    case SIMD_AVX:     return hhi_u32_avx;
    case SIMD_SSE4_2:  return hhi_u32_sse42;
    case SIMD_SSE2:    return hhi_u32_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return hhi_u32_sve2;
    case SIMD_SVE:     return hhi_u32_sve;
    case SIMD_NEON:    return hhi_u32_neon;
#endif
    case SIMD_SCALAR:
    default:           return hhi_u32_scalar;
}
}

EXPLICIT_RUNTIME_RESOLVER(hhi_u32_resolver, hhi_u32_fn_t)
{
    return hhi_u32_select(detect_simd_level_ts());
}
DYNEMIT_IFUNC_SETUP(hhi_u32_fn_t, hhi_u32, hhi_u32_resolver)

#if defined(DYNEMIT_NO_IFUNC)
double hhi_u32(const uint32_t *data, size_t n)
{
    return DYNEMIT_IFUNC_INVOKE(hhi_u32, (data, n));
}
#else
#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
double hhi_u32(const uint32_t *data, size_t n)
    DYNEMIT_IFUNC_ATTR("hhi_u32_resolver");
#endif
