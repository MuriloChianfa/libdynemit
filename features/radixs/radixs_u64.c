/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elifdef __aarch64__
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include "mem.h"
#include <dynemit/compiler.h>
#include <dynemit/radixs.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

enum {
    RADIXS_U64_PASSES = 8,
    RADIXS_U64_BUCKETS = 256
};

static int
radixs_u64_cmp(const void *a, const void *b)
{
    uint64_t x = *(const uint64_t *)a;
    uint64_t y = *(const uint64_t *)b;
    return (x > y) - (x < y);
}

static void
radixs_u64_qsort_fallback(const uint64_t *in, uint64_t *out, size_t n)
{
    if (in != out) {
        if (memcpys(out, n * sizeof(uint64_t), in,
                             n * sizeof(uint64_t)) != 0) {
            return;
        }
    }
    qsort(out, n, sizeof(uint64_t), radixs_u64_cmp);
}

/*
 * One-pass build of all 8 byte-position histograms over the input,
 * returning a per-pass skip mask that flags digits where every element
 * shares the same byte (so the corresponding scatter pass is a memcpy).
 */
static unsigned
radixs_u64_build_histograms(const uint64_t *in, size_t n,
                           uint32_t hist[RADIXS_U64_PASSES][RADIXS_U64_BUCKETS])
{
    if (memsets(hist,
                         sizeof(uint32_t) * RADIXS_U64_PASSES * RADIXS_U64_BUCKETS,
                         0,
                         sizeof(uint32_t) * RADIXS_U64_PASSES * RADIXS_U64_BUCKETS) != 0) {
        return 0;
    }

    for (size_t i = 0; i < n; i++) {
        uint64_t v = in[i];
        hist[0][(v      ) & 0xff]++;
        hist[1][(v >>  8) & 0xff]++;
        hist[2][(v >> 16) & 0xff]++;
        hist[3][(v >> 24) & 0xff]++;
        hist[4][(v >> 32) & 0xff]++;
        hist[5][(v >> 40) & 0xff]++;
        hist[6][(v >> 48) & 0xff]++;
        hist[7][(v >> 56) & 0xff]++;
    }

    unsigned skip = 0;
    for (int p = 0; p < RADIXS_U64_PASSES; p++) {
        for (int b = 0; b < RADIXS_U64_BUCKETS; b++) {
            if (hist[p][b] == n) {
                skip |= (1U << p);
                break;
            }
        }
    }
    return skip;
}

static void
radixs_u64_prefix_sum(uint32_t hist[RADIXS_U64_PASSES][RADIXS_U64_BUCKETS])
{
    for (int p = 0; p < RADIXS_U64_PASSES; p++) {
        uint32_t sum = 0;
        for (int b = 0; b < RADIXS_U64_BUCKETS; b++) {
            uint32_t c = hist[p][b];
            hist[p][b] = sum;
            sum += c;
        }
    }
}

static inline void
radixs_u64_pass_scalar(const uint64_t *src, uint64_t *dst, size_t n,
                      uint32_t *hist, int byte_idx)
{
    int shift = byte_idx * 8;
    for (size_t i = 0; i < n; i++) {
        uint64_t v = src[i];
        uint32_t b = (uint32_t)((v >> shift) & 0xff);
        dst[hist[b]++] = v;
    }
}

static void
radixs_u64_run(const uint64_t *in, uint64_t *out, size_t n)
{
    if (n == 0) {
        return;
    }
    if (n == 1) { out[0] = in[0]; return; }

    uint32_t (*hist)[RADIXS_U64_BUCKETS] =
        aligned_alloc(64, sizeof(uint32_t) * RADIXS_U64_PASSES * RADIXS_U64_BUCKETS);
    if (!hist) {
        radixs_u64_qsort_fallback(in, out, n);
        return;
    }
    uint64_t *tmp = aligned_alloc(64, mem_aligned_count(n, uint64_t));
    if (!tmp) {
        free(hist);
        radixs_u64_qsort_fallback(in, out, n);
        return;
    }

    unsigned skip = radixs_u64_build_histograms(in, n, hist);
    radixs_u64_prefix_sum(hist);

    int active_passes = 0;
    int pass_list[RADIXS_U64_PASSES];
    for (int p = 0; p < RADIXS_U64_PASSES; p++) {
        if (!(skip & (1U << p))) {
            pass_list[active_passes++] = p;
        }
    }

    if (active_passes == 0) {
        if (memcpys(out, n * sizeof(uint64_t), in,
                             n * sizeof(uint64_t)) != 0) {
            free(tmp);
            free(hist);
            return;
        }
        free(tmp);
        free(hist);
        return;
    }

    const uint64_t *src = in;
    uint64_t *dst = (active_passes & 1) ? out : tmp;

    for (int idx = 0; idx < active_passes; idx++) {
        radixs_u64_pass_scalar(src, dst, n, hist[pass_list[idx]], pass_list[idx]);
        src = dst;
        dst = (dst == tmp) ? out : tmp;
    }

    free(tmp);
    free(hist);
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static void
radixs_u64_scalar(const uint64_t *in, uint64_t *out, size_t n)
{
    radixs_u64_run(in, out, n);
}


#if defined(__x86_64__) || defined(__i386__)

__attribute__((target("sse2")))
static void
radixs_u64_sse2(const uint64_t *in, uint64_t *out, size_t n)
{
    radixs_u64_run(in, out, n);
}

__attribute__((target("sse4.2")))
static void
radixs_u64_sse42(const uint64_t *in, uint64_t *out, size_t n)
{
    radixs_u64_run(in, out, n);
}

__attribute__((target("avx")))
static void
radixs_u64_avx(const uint64_t *in, uint64_t *out, size_t n)
{
    radixs_u64_run(in, out, n);
}

/*
 * AVX2: extract all 8 byte digits per element into 8 byte-buffers via
 * shifts and masks; scalar increments follow because AVX2 has no scatter.
 */
__attribute__((target("avx2,bmi2")))
static unsigned
radixs_u64_build_histograms_avx2(const uint64_t *in, size_t n,
                                uint32_t hist[RADIXS_U64_PASSES][RADIXS_U64_BUCKETS])
{
    if (memsets(hist,
                         sizeof(uint32_t) * RADIXS_U64_PASSES * RADIXS_U64_BUCKETS,
                         0,
                         sizeof(uint32_t) * RADIXS_U64_PASSES * RADIXS_U64_BUCKETS) != 0) {
        return 0;
    }

    const __m256i mask = _mm256_set1_epi64x(0xff);
    size_t i = 0;
    alignas(32) uint64_t b[8][4];

    for (; i + 4 <= n; i += 4) {
        __m256i v = _mm256_loadu_si256((const __m256i *)(in + i));
        for (int p = 0; p < 8; p++) {
            __m256i bp = _mm256_and_si256(_mm256_srli_epi64(v, p * 8), mask);
            _mm256_store_si256((__m256i *)b[p], bp);
        }
        for (int p = 0; p < 8; p++) {
            for (int j = 0; j < 4; j++) {
                hist[p][b[p][j]]++;
            }
        }
    }
    for (; i < n; i++) {
        uint64_t v = in[i];
        hist[0][(v      ) & 0xff]++;
        hist[1][(v >>  8) & 0xff]++;
        hist[2][(v >> 16) & 0xff]++;
        hist[3][(v >> 24) & 0xff]++;
        hist[4][(v >> 32) & 0xff]++;
        hist[5][(v >> 40) & 0xff]++;
        hist[6][(v >> 48) & 0xff]++;
        hist[7][(v >> 56) & 0xff]++;
    }

    unsigned skip = 0;
    for (int p = 0; p < RADIXS_U64_PASSES; p++) {
        for (int bb = 0; bb < RADIXS_U64_BUCKETS; bb++) {
            if (hist[p][bb] == n) { skip |= (1U << p); break; }
        }
    }
    return skip;
}

__attribute__((target("avx2,bmi2")))
static void
radixs_u64_avx2(const uint64_t *in, uint64_t *out, size_t n)
{
    if (n == 0) {
        return;
    }
    if (n == 1) { out[0] = in[0]; return; }

    uint32_t (*hist)[RADIXS_U64_BUCKETS] =
        aligned_alloc(64, sizeof(uint32_t) * RADIXS_U64_PASSES * RADIXS_U64_BUCKETS);
    if (!hist) { radixs_u64_qsort_fallback(in, out, n); return; }
    uint64_t *tmp = aligned_alloc(64, mem_aligned_count(n, uint64_t));
    if (!tmp) { free(hist); radixs_u64_qsort_fallback(in, out, n); return; }

    unsigned skip = radixs_u64_build_histograms_avx2(in, n, hist);
    radixs_u64_prefix_sum(hist);

    int active = 0;
    int pass_list[RADIXS_U64_PASSES];
    for (int p = 0; p < RADIXS_U64_PASSES; p++) {
        if (!(skip & (1U << p))) {
            pass_list[active++] = p;
        }
    }

    if (active == 0) {
        if (memcpys(out, n * sizeof(uint64_t), in,
                             n * sizeof(uint64_t)) != 0) {
            free(tmp);
            free(hist);
            return;
        }
        free(tmp);
        free(hist);
        return;
    }

    const uint64_t *src = in;
    uint64_t *dst = (active & 1) ? out : tmp;
    for (int idx = 0; idx < active; idx++) {
        radixs_u64_pass_scalar(src, dst, n, hist[pass_list[idx]], pass_list[idx]);
        src = dst;
        dst = (dst == tmp) ? out : tmp;
    }

    free(tmp);
    free(hist);
}

/*
 * AVX-512F scatter: 8 u64 per vector, vpconflictq to detect within-vector
 * collisions, vpscatterqq for the conflict-free fast path.
 */
__attribute__((target("avx512f,avx512cd")))
static void
radixs_u64_pass_avx512f(const uint64_t *src, uint64_t *dst, size_t n,
                       uint32_t *hist, int byte_idx)
{
    const int shift = byte_idx * 8;
    const __m512i mask = _mm512_set1_epi64(0xff);
    size_t i = 0;
    alignas(64) uint64_t buf_v[8];
    alignas(64) uint64_t buf_b[8];

    for (; i + 8 <= n; i += 8) {
        __m512i v = _mm512_loadu_si512((const void *)(src + i));
        __m512i b = (shift == 0)
                    ? _mm512_and_si512(v, mask)
                    : _mm512_and_si512(_mm512_srli_epi64(v, shift), mask);
        __m512i conflict = _mm512_conflict_epi64(b);
        __mmask8 unique = _mm512_cmpeq_epi64_mask(conflict, _mm512_setzero_si512());

        if (unique == 0xff) {
            _mm512_store_si512((void *)buf_b, b);
            __m512i offsets = _mm512_set_epi64(
                (long long)hist[buf_b[7]], (long long)hist[buf_b[6]],
                (long long)hist[buf_b[5]], (long long)hist[buf_b[4]],
                (long long)hist[buf_b[3]], (long long)hist[buf_b[2]],
                (long long)hist[buf_b[1]], (long long)hist[buf_b[0]]);
            _mm512_i64scatter_epi64(dst, offsets, v, 8);
            for (int j = 0; j < 8; j++) {
                hist[buf_b[j]]++;
            }
        } else {
            _mm512_store_si512((void *)buf_v, v);
            _mm512_store_si512((void *)buf_b, b);
            for (int j = 0; j < 8; j++) {
                dst[hist[buf_b[j]]++] = buf_v[j];
            }
        }
    }
    for (; i < n; i++) {
        uint64_t vi = src[i];
        uint32_t bi = (uint32_t)((vi >> shift) & 0xff);
        dst[hist[bi]++] = vi;
    }
}

__attribute__((target("avx512f,avx512cd,avx2,bmi2")))
static void
radixs_u64_avx512f(const uint64_t *in, uint64_t *out, size_t n)
{
    if (n == 0) {
        return;
    }
    if (n == 1) { out[0] = in[0]; return; }

    uint32_t (*hist)[RADIXS_U64_BUCKETS] =
        aligned_alloc(64, sizeof(uint32_t) * RADIXS_U64_PASSES * RADIXS_U64_BUCKETS);
    if (!hist) { radixs_u64_qsort_fallback(in, out, n); return; }
    uint64_t *tmp = aligned_alloc(64, mem_aligned_count(n, uint64_t));
    if (!tmp) { free(hist); radixs_u64_qsort_fallback(in, out, n); return; }

    unsigned skip = radixs_u64_build_histograms_avx2(in, n, hist);
    radixs_u64_prefix_sum(hist);

    int active = 0;
    int pass_list[RADIXS_U64_PASSES];
    for (int p = 0; p < RADIXS_U64_PASSES; p++) {
        if (!(skip & (1U << p))) {
            pass_list[active++] = p;
        }
    }

    if (active == 0) {
        if (memcpys(out, n * sizeof(uint64_t), in,
                             n * sizeof(uint64_t)) != 0) {
            free(tmp);
            free(hist);
            return;
        }
        free(tmp);
        free(hist);
        return;
    }

    const uint64_t *src = in;
    uint64_t *dst = (active & 1) ? out : tmp;
    for (int idx = 0; idx < active; idx++) {
        radixs_u64_pass_avx512f(src, dst, n, hist[pass_list[idx]], pass_list[idx]);
        src = dst;
        dst = (dst == tmp) ? out : tmp;
    }

    free(tmp);
    free(hist);
}

/*
 * VBMI2: vpermb extracts the requested byte from each lane in a single
 * permute, replacing the shift+mask used by the AVX-512F scatter pass.
 * Reuses the AVX-512F scatter logic for the actual store.
 */
__attribute__((target("avx512vbmi2,avx512vbmi,avx512bw,avx512f,avx512cd")))
static void
radixs_u64_pass_vbmi2(const uint64_t *src, uint64_t *dst, size_t n,
                     uint32_t *hist, int byte_idx)
{
    /*
     * For 8 lanes of u64 in a 64-byte vector, lane k starts at byte k*8.
     * To extract byte_idx of each lane into the low byte of each u64 lane
     * in the result, the permute index for lane k's low byte is
     * (k*8 + byte_idx); the other 7 bytes of that lane are zeroed by
     * pointing them at lane 0 of the source and then masking.
     */
    alignas(64) uint8_t idx_bytes[64] = {0};
    for (int k = 0; k < 8; k++) {
        idx_bytes[(size_t)k * 8U] = (uint8_t)((k * 8) + byte_idx);
    }
    __m512i perm = _mm512_load_si512((const void *)idx_bytes);
    const __m512i mask = _mm512_set1_epi64(0xff);

    size_t i = 0;
    alignas(64) uint64_t buf_v[8];
    alignas(64) uint64_t buf_b[8];

    for (; i + 8 <= n; i += 8) {
        __m512i v = _mm512_loadu_si512((const void *)(src + i));
        __m512i b = _mm512_and_si512(_mm512_permutexvar_epi8(perm, v), mask);
        __m512i conflict = _mm512_conflict_epi64(b);
        __mmask8 unique = _mm512_cmpeq_epi64_mask(conflict, _mm512_setzero_si512());

        if (unique == 0xff) {
            _mm512_store_si512((void *)buf_b, b);
            __m512i offsets = _mm512_set_epi64(
                (long long)hist[buf_b[7]], (long long)hist[buf_b[6]],
                (long long)hist[buf_b[5]], (long long)hist[buf_b[4]],
                (long long)hist[buf_b[3]], (long long)hist[buf_b[2]],
                (long long)hist[buf_b[1]], (long long)hist[buf_b[0]]);
            _mm512_i64scatter_epi64(dst, offsets, v, 8);
            for (int j = 0; j < 8; j++) {
                hist[buf_b[j]]++;
            }
        } else {
            _mm512_store_si512((void *)buf_v, v);
            _mm512_store_si512((void *)buf_b, b);
            for (int j = 0; j < 8; j++) {
                dst[hist[buf_b[j]]++] = buf_v[j];
            }
        }
    }
    int shift = byte_idx * 8;
    for (; i < n; i++) {
        uint64_t vi = src[i];
        uint32_t bi = (uint32_t)((vi >> shift) & 0xff);
        dst[hist[bi]++] = vi;
    }
}

__attribute__((target("avx512vbmi2,avx512vbmi,avx512bw,avx512f,avx512cd,avx2,bmi2")))
static void
radixs_u64_avx512_vbmi2(const uint64_t *in, uint64_t *out, size_t n)
{
    if (n == 0) {
        return;
    }
    if (n == 1) { out[0] = in[0]; return; }

    uint32_t (*hist)[RADIXS_U64_BUCKETS] =
        aligned_alloc(64, sizeof(uint32_t) * RADIXS_U64_PASSES * RADIXS_U64_BUCKETS);
    if (!hist) { radixs_u64_qsort_fallback(in, out, n); return; }
    uint64_t *tmp = aligned_alloc(64, mem_aligned_count(n, uint64_t));
    if (!tmp) { free(hist); radixs_u64_qsort_fallback(in, out, n); return; }

    unsigned skip = radixs_u64_build_histograms_avx2(in, n, hist);
    radixs_u64_prefix_sum(hist);

    int active = 0;
    int pass_list[RADIXS_U64_PASSES];
    for (int p = 0; p < RADIXS_U64_PASSES; p++) {
        if (!(skip & (1U << p))) {
            pass_list[active++] = p;
        }
    }

    if (active == 0) {
        if (memcpys(out, n * sizeof(uint64_t), in,
                             n * sizeof(uint64_t)) != 0) {
            free(tmp);
            free(hist);
            return;
        }
        free(tmp);
        free(hist);
        return;
    }

    const uint64_t *src = in;
    uint64_t *dst = (active & 1) ? out : tmp;
    for (int idx = 0; idx < active; idx++) {
        radixs_u64_pass_vbmi2(src, dst, n, hist[pass_list[idx]], pass_list[idx]);
        src = dst;
        dst = (dst == tmp) ? out : tmp;
    }

    free(tmp);
    free(hist);
}
#endif /* x86 */

#ifdef __aarch64__

static void
radixs_u64_neon(const uint64_t *in, uint64_t *out, size_t n)
{
    radixs_u64_run(in, out, n);
}

__attribute__((target("+sve")))
static void
radixs_u64_sve(const uint64_t *in, uint64_t *out, size_t n)
{
    radixs_u64_run(in, out, n);
}

__attribute__((target("+sve2")))
static void
radixs_u64_sve2(const uint64_t *in, uint64_t *out, size_t n)
{
    radixs_u64_run(in, out, n);
}

#endif /* aarch64 */

radixs_u64_fn_t
radixs_u64_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2: return radixs_u64_avx512_vbmi2;
    case SIMD_AVX512F: return radixs_u64_avx512f;
    case SIMD_AVX2:    return radixs_u64_avx2;
    case SIMD_AVX:     return radixs_u64_avx;
    case SIMD_SSE4_2:  return radixs_u64_sse42;
    case SIMD_SSE2:    return radixs_u64_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return radixs_u64_sve2;
    case SIMD_SVE:     return radixs_u64_sve;
    case SIMD_NEON:    return radixs_u64_neon;
#endif
    case SIMD_SCALAR:
    default:           return radixs_u64_scalar;
}
}

EXPLICIT_RUNTIME_RESOLVER(radixs_u64_resolver, radixs_u64_fn_t)
{
    return radixs_u64_select(detect_simd_level_ts());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512vbmi2,avx512vbmi,avx512bw,avx512f,avx512cd,avx2,bmi2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
void radixs_u64(const uint64_t *in, uint64_t *out, size_t n)
    __attribute__((ifunc("radixs_u64_resolver")));
