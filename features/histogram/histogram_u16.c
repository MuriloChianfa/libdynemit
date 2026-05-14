/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dynemit/histogram.h>
#include <dynemit/compiler.h>

/*
 * Count elements falling into ranges defined by boundaries.
 * Given boundaries b0, b1, ..., b_{k-1} (ascending), counts elements in:
 *   [0, b0), [b0, b1), ..., [b_{k-1}, UINT16_MAX].
 * Output array has num_boundaries+1 elements.
 *
 * For u16 with small num_boundaries (typical: 2-3 for port ranges),
 * we iterate boundaries per element.
 */

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static void
histogram_u16_scalar(const uint16_t *data, size_t n,
                           const uint16_t *boundaries, size_t num_boundaries,
                           uint64_t *out)
{
    memset(out, 0, (num_boundaries + 1) * sizeof(uint64_t));
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        uint16_t val = data[i];
        size_t bucket = num_boundaries;
        for (size_t b = 0; b < num_boundaries; b++) {
            if (val < boundaries[b]) {
                bucket = b;
                break;
            }
        }
        out[bucket]++;
    }
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static void
histogram_u16_sse2(const uint16_t *data, size_t n,
                         const uint16_t *boundaries, size_t num_boundaries,
                         uint64_t *out)
{
    /* SSE2 lacks unsigned u16 comparison; use scalar */
    histogram_u16_scalar(data, n, boundaries, num_boundaries, out);
}

__attribute__((target("sse4.2")))
static void
histogram_u16_sse42(const uint16_t *data, size_t n,
                          const uint16_t *boundaries, size_t num_boundaries,
                          uint64_t *out)
{
    histogram_u16_scalar(data, n, boundaries, num_boundaries, out);
}

__attribute__((target("avx")))
static void
histogram_u16_avx(const uint16_t *data, size_t n,
                        const uint16_t *boundaries, size_t num_boundaries,
                        uint64_t *out)
{
    histogram_u16_scalar(data, n, boundaries, num_boundaries, out);
}

__attribute__((target("avx2")))
static void
histogram_u16_avx2(const uint16_t *data, size_t n,
                         const uint16_t *boundaries, size_t num_boundaries,
                         uint64_t *out)
{
    /*
     * AVX2 vectorized: load 16 u16 elements, for each boundary compare
     * using unsigned comparison (bias by 0x8000 for signed cmpgt).
     * Count how many elements are >= each boundary to determine bucket.
     */
    memset(out, 0, (num_boundaries + 1) * sizeof(uint64_t));
    if (num_boundaries == 0) {
        out[0] = n;
        return;
    }

    const __m256i bias = _mm256_set1_epi16((short)0x8000);
    size_t i = 0;

    /* Pre-bias boundaries (32-byte alignment required for AVX2 stores) */
    __m256i *vbounds = (__m256i *)aligned_alloc(32, num_boundaries * sizeof(__m256i));
    if (!vbounds) {
        histogram_u16_scalar(data, n, boundaries, num_boundaries, out);
        return;
    }
    for (size_t b = 0; b < num_boundaries; b++)
        vbounds[b] = _mm256_add_epi16(_mm256_set1_epi16((short)boundaries[b]), bias);

    /*
     * For each group of 16 elements, count how many fall below each boundary.
     * bucket[elem] = number of boundaries where elem >= boundary.
     */
    for (; i + 16 <= n; i += 16) {
        __m256i vdata = _mm256_loadu_si256((const __m256i *)(data + i));
        __m256i vbiased = _mm256_add_epi16(vdata, bias);

        /* Start with all in the last bucket */
        __m256i vbucket = _mm256_set1_epi16((short)num_boundaries);
        for (size_t b = num_boundaries; b > 0; b--) {
            /* elem < boundary[b-1] => biased_elem < biased_boundary[b-1] */
            __m256i cmp = _mm256_cmpgt_epi16(vbounds[b - 1], vbiased);
            /* cmp is 0xFFFF where boundary > elem, i.e., elem < boundary */
            /* For those elements, bucket = b-1 */
            vbucket = _mm256_blendv_epi8(vbucket, _mm256_set1_epi16((short)(b - 1)), cmp);
        }

        /* Extract buckets and increment counts */
        alignas(32) uint16_t buckets[16];
        _mm256_storeu_si256((__m256i *)buckets, vbucket);
        for (size_t j = 0; j < 16; j++)
            out[buckets[j]]++;
    }

    /* Scalar tail */
    for (; i < n; i++) {
        uint16_t val = data[i];
        size_t bucket = num_boundaries;
        for (size_t b = 0; b < num_boundaries; b++) {
            if (val < boundaries[b]) {
                bucket = b;
                break;
            }
        }
        out[bucket]++;
    }

    free(vbounds);
}

__attribute__((target("avx512f")))
static void
histogram_u16_avx512f(const uint16_t *data, size_t n,
                            const uint16_t *boundaries, size_t num_boundaries,
                            uint64_t *out)
{
    /* AVX512F does not have 16-bit comparison; delegate to AVX2 path */
    histogram_u16_avx2(data, n, boundaries, num_boundaries, out);
}
#endif

#if defined(__aarch64__)

static void
histogram_u16_neon(const uint16_t *data, size_t n,
                         const uint16_t *boundaries, size_t num_boundaries,
                         uint64_t *out)
{
    histogram_u16_scalar(data, n, boundaries, num_boundaries, out);
}

__attribute__((target("+sve")))
static void
histogram_u16_sve(const uint16_t *data, size_t n,
                        const uint16_t *boundaries, size_t num_boundaries,
                        uint64_t *out)
{
    histogram_u16_scalar(data, n, boundaries, num_boundaries, out);
}

__attribute__((target("+sve2")))
static void
histogram_u16_sve2(const uint16_t *data, size_t n,
                         const uint16_t *boundaries, size_t num_boundaries,
                         uint64_t *out)
{
    histogram_u16_scalar(data, n, boundaries, num_boundaries, out);
}

#endif /* aarch64 */

histogram_u16_fn_t
histogram_u16_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return histogram_u16_avx512f;
    case SIMD_AVX2:    return histogram_u16_avx2;
    case SIMD_AVX:     return histogram_u16_avx;
    case SIMD_SSE4_2:  return histogram_u16_sse42;
    case SIMD_SSE2:    return histogram_u16_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return histogram_u16_sve2;
    case SIMD_SVE:     return histogram_u16_sve;
    case SIMD_NEON:    return histogram_u16_neon;
#endif
    case SIMD_SCALAR:
    default:           return histogram_u16_scalar;
    }
}

static histogram_u16_fn_t
histogram_u16_resolver(void)
{
    return histogram_u16_select(detect_simd_level());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
void histogram_u16(const uint16_t *data, size_t n,
                         const uint16_t *boundaries, size_t num_boundaries,
                         uint64_t *out)
    __attribute__((ifunc("histogram_u16_resolver")));
