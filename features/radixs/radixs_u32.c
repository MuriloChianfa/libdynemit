/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elif defined(__aarch64__)
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <dynemit/radixs.h>
#include <dynemit/compiler.h>

#define RADIXS_U32_PASSES 4
#define RADIXS_U32_BUCKETS 256

/*
 * ISO C11 aligned_alloc() requires the requested size to be a multiple of
 * the alignment, and glibc's allocator (as well as the AddressSanitizer
 * allocator interceptor) treats violations as hard failures. Round the scratch
 * size up to the next multiple of 64 bytes so the call is well-defined for
 * every n. The overhead is at most 63 bytes per call.
 */
#define RADIXS_U32_TMP_BYTES(n) \
    ((((size_t)(n) * sizeof(uint32_t)) + (size_t)63) & ~(size_t)63)

/*
 * Comparator and qsort fallback used when scratch allocation fails so the
 * function still returns a sorted result rather than silently failing.
 */
static int
radixs_u32_cmp(const void *a, const void *b)
{
    uint32_t x = *(const uint32_t *)a;
    uint32_t y = *(const uint32_t *)b;
    return (x > y) - (x < y);
}

static void
radixs_u32_qsort_fallback(const uint32_t *in, uint32_t *out, size_t n)
{
    if (in != out)
        memcpy(out, in, n * sizeof(uint32_t));
    qsort(out, n, sizeof(uint32_t), radixs_u32_cmp);
}

/*
 * Build all 4 byte-position histograms in one pass over the input.
 * Returns a per-pass `pass_skip` mask: bit p set iff every element shares
 * the same byte at position p (so the radix pass for that digit is a no-op).
 */
static unsigned
radixs_u32_build_histograms(const uint32_t *in, size_t n,
                           uint32_t hist[RADIXS_U32_PASSES][RADIXS_U32_BUCKETS])
{
    memset(hist, 0, sizeof(uint32_t) * RADIXS_U32_PASSES * RADIXS_U32_BUCKETS);

    for (size_t i = 0; i < n; i++) {
        uint32_t v = in[i];
        hist[0][(v      ) & 0xff]++;
        hist[1][(v >>  8) & 0xff]++;
        hist[2][(v >> 16) & 0xff]++;
        hist[3][(v >> 24) & 0xff]++;
    }

    unsigned skip = 0;
    for (int p = 0; p < RADIXS_U32_PASSES; p++) {
        for (int b = 0; b < RADIXS_U32_BUCKETS; b++) {
            if (hist[p][b] == n) {
                skip |= (1u << p);
                break;
            }
        }
    }
    return skip;
}

/*
 * Convert per-pass histograms into exclusive prefix sums in place,
 * yielding a running write offset per bucket for the scatter phase.
 */
static void
radixs_u32_prefix_sum(uint32_t hist[RADIXS_U32_PASSES][RADIXS_U32_BUCKETS])
{
    for (int p = 0; p < RADIXS_U32_PASSES; p++) {
        uint32_t sum = 0;
        for (int b = 0; b < RADIXS_U32_BUCKETS; b++) {
            uint32_t c = hist[p][b];
            hist[p][b] = sum;
            sum += c;
        }
    }
}

/*
 * One LSD scatter pass: read from src, scatter into dst by digit p of src[i].
 * `hist[b]` holds the running write offset for bucket b and is post-incremented.
 */
static inline void
radixs_u32_pass_scalar(const uint32_t *src, uint32_t *dst, size_t n,
                      uint32_t *hist, int byte_idx)
{
    int shift = byte_idx * 8;
    for (size_t i = 0; i < n; i++) {
        uint32_t v = src[i];
        uint32_t b = (v >> shift) & 0xff;
        dst[hist[b]++] = v;
    }
}

/*
 * Drive the 4-pass LSD ping-pong, choosing the initial dst so the final
 * pass lands in `out`. Falls back to qsort if the scratch alloc fails.
 */
static void
radixs_u32_run(const uint32_t *in, uint32_t *out, size_t n)
{
    if (n == 0) return;
    if (n == 1) { out[0] = in[0]; return; }

    uint32_t (*hist)[RADIXS_U32_BUCKETS] =
        aligned_alloc(64, sizeof(uint32_t) * RADIXS_U32_PASSES * RADIXS_U32_BUCKETS);
    if (!hist) {
        radixs_u32_qsort_fallback(in, out, n);
        return;
    }
    uint32_t *tmp = aligned_alloc(64, RADIXS_U32_TMP_BYTES(n));
    if (!tmp) {
        free(hist);
        radixs_u32_qsort_fallback(in, out, n);
        return;
    }

    unsigned skip = radixs_u32_build_histograms(in, n, hist);
    radixs_u32_prefix_sum(hist);

    int active_passes = 0;
    int pass_list[RADIXS_U32_PASSES];
    for (int p = 0; p < RADIXS_U32_PASSES; p++) {
        if (!(skip & (1u << p)))
            pass_list[active_passes++] = p;
    }

    if (active_passes == 0) {
        memcpy(out, in, n * sizeof(uint32_t));
        free(tmp);
        free(hist);
        return;
    }

    /*
     * Choose initial dst so the final active pass writes to `out`. With
     * an even number of active passes the first dst is `tmp`; with odd
     * it is `out`.
     */
    const uint32_t *src = in;
    uint32_t *dst = (active_passes & 1) ? out : tmp;
    uint32_t *other = (dst == out) ? tmp : out;

    for (int idx = 0; idx < active_passes; idx++) {
        radixs_u32_pass_scalar(src, dst, n, hist[pass_list[idx]], pass_list[idx]);
        src = dst;
        dst = (dst == tmp) ? out : tmp;
        (void)other;
    }

    free(tmp);
    free(hist);
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static void
radixs_u32_scalar(const uint32_t *in, uint32_t *out, size_t n)
{
    radixs_u32_run(in, out, n);
}


#if defined(__x86_64__) || defined(__i386__)

/*
 * For SSE2 through AVX2, the dominant cost is the histogram and scatter
 * loops which are inherently scatter-pattern dependent. The compiler with
 * the relevant -m target can still partially vectorize the histogram
 * count load/store sequence; we therefore delegate to the scalar driver
 * and let target-specific codegen kick in.
 */
__attribute__((target("sse2")))
static void
radixs_u32_sse2(const uint32_t *in, uint32_t *out, size_t n)
{
    radixs_u32_run(in, out, n);
}

__attribute__((target("sse4.2")))
static void
radixs_u32_sse42(const uint32_t *in, uint32_t *out, size_t n)
{
    radixs_u32_run(in, out, n);
}

__attribute__((target("avx")))
static void
radixs_u32_avx(const uint32_t *in, uint32_t *out, size_t n)
{
    radixs_u32_run(in, out, n);
}

/*
 * AVX2 histogram acceleration: count by 8-element blocks, extracting all
 * four byte positions per element via shift+mask in 256-bit registers
 * before falling back to scalar increments (no AVX2 scatter exists).
 */
__attribute__((target("avx2,bmi2")))
static unsigned
radixs_u32_build_histograms_avx2(const uint32_t *in, size_t n,
                                uint32_t hist[RADIXS_U32_PASSES][RADIXS_U32_BUCKETS])
{
    memset(hist, 0, sizeof(uint32_t) * RADIXS_U32_PASSES * RADIXS_U32_BUCKETS);

    const __m256i mask = _mm256_set1_epi32(0xff);
    size_t i = 0;
    alignas(32) uint32_t b0[8], b1[8], b2[8], b3[8];

    for (; i + 8 <= n; i += 8) {
        __m256i v   = _mm256_loadu_si256((const __m256i *)(in + i));
        __m256i v0  = _mm256_and_si256(v, mask);
        __m256i v1  = _mm256_and_si256(_mm256_srli_epi32(v,  8), mask);
        __m256i v2  = _mm256_and_si256(_mm256_srli_epi32(v, 16), mask);
        __m256i v3  =                  _mm256_srli_epi32(v, 24);
        _mm256_store_si256((__m256i *)b0, v0);
        _mm256_store_si256((__m256i *)b1, v1);
        _mm256_store_si256((__m256i *)b2, v2);
        _mm256_store_si256((__m256i *)b3, v3);
        for (int j = 0; j < 8; j++) {
            hist[0][b0[j]]++;
            hist[1][b1[j]]++;
            hist[2][b2[j]]++;
            hist[3][b3[j]]++;
        }
    }
    for (; i < n; i++) {
        uint32_t v = in[i];
        hist[0][(v      ) & 0xff]++;
        hist[1][(v >>  8) & 0xff]++;
        hist[2][(v >> 16) & 0xff]++;
        hist[3][(v >> 24) & 0xff]++;
    }

    unsigned skip = 0;
    for (int p = 0; p < RADIXS_U32_PASSES; p++) {
        for (int b = 0; b < RADIXS_U32_BUCKETS; b++) {
            if (hist[p][b] == n) {
                skip |= (1u << p);
                break;
            }
        }
    }
    return skip;
}

__attribute__((target("avx2,bmi2")))
static void
radixs_u32_avx2_run(const uint32_t *in, uint32_t *out, size_t n)
{
    if (n == 0) return;
    if (n == 1) { out[0] = in[0]; return; }

    uint32_t (*hist)[RADIXS_U32_BUCKETS] =
        aligned_alloc(64, sizeof(uint32_t) * RADIXS_U32_PASSES * RADIXS_U32_BUCKETS);
    if (!hist) { radixs_u32_qsort_fallback(in, out, n); return; }
    uint32_t *tmp = aligned_alloc(64, RADIXS_U32_TMP_BYTES(n));
    if (!tmp) { free(hist); radixs_u32_qsort_fallback(in, out, n); return; }

    unsigned skip = radixs_u32_build_histograms_avx2(in, n, hist);
    radixs_u32_prefix_sum(hist);

    int active = 0, pass_list[RADIXS_U32_PASSES];
    for (int p = 0; p < RADIXS_U32_PASSES; p++)
        if (!(skip & (1u << p))) pass_list[active++] = p;

    if (active == 0) {
        memcpy(out, in, n * sizeof(uint32_t));
        free(tmp); free(hist); return;
    }

    const uint32_t *src = in;
    uint32_t *dst = (active & 1) ? out : tmp;
    for (int idx = 0; idx < active; idx++) {
        radixs_u32_pass_scalar(src, dst, n, hist[pass_list[idx]], pass_list[idx]);
        src = dst;
        dst = (dst == tmp) ? out : tmp;
    }

    free(tmp);
    free(hist);
}

__attribute__((target("avx2,bmi2")))
static void
radixs_u32_avx2(const uint32_t *in, uint32_t *out, size_t n)
{
    radixs_u32_avx2_run(in, out, n);
}

/*
 * AVX-512F scatter pass: extract 16 byte digits via vpsrld+vpand, then
 * use vpconflictd to detect within-vector duplicates. Lanes with no
 * conflict are scatter-stored via vpscatterdd; the rest fall back to
 * scalar increments. The skip-pass optimization still applies.
 */
__attribute__((target("avx512f,avx512cd")))
static void
radixs_u32_pass_avx512f(const uint32_t *src, uint32_t *dst, size_t n,
                       uint32_t *hist, int byte_idx)
{
    const int shift = byte_idx * 8;
    const __m512i mask = _mm512_set1_epi32(0xff);
    size_t i = 0;
    alignas(64) uint32_t buf_v[16];
    alignas(64) uint32_t buf_b[16];

    for (; i + 16 <= n; i += 16) {
        __m512i v = _mm512_loadu_si512((const void *)(src + i));
        __m512i b = (shift == 0)
                    ? _mm512_and_si512(v, mask)
                    : _mm512_and_si512(_mm512_srli_epi32(v, shift), mask);
        __m512i conflict = _mm512_conflict_epi32(b);
        __mmask16 unique = _mm512_cmpeq_epi32_mask(conflict, _mm512_setzero_si512());

        if (unique == 0xffff) {
            _mm512_store_si512((void *)buf_b, b);
            __m512i offsets = _mm512_set_epi32(
                hist[buf_b[15]], hist[buf_b[14]], hist[buf_b[13]], hist[buf_b[12]],
                hist[buf_b[11]], hist[buf_b[10]], hist[buf_b[ 9]], hist[buf_b[ 8]],
                hist[buf_b[ 7]], hist[buf_b[ 6]], hist[buf_b[ 5]], hist[buf_b[ 4]],
                hist[buf_b[ 3]], hist[buf_b[ 2]], hist[buf_b[ 1]], hist[buf_b[ 0]]);
            _mm512_i32scatter_epi32(dst, offsets, v, 4);
            for (int j = 0; j < 16; j++) hist[buf_b[j]]++;
        } else {
            _mm512_store_si512((void *)buf_v, v);
            _mm512_store_si512((void *)buf_b, b);
            for (int j = 0; j < 16; j++)
                dst[hist[buf_b[j]]++] = buf_v[j];
        }
    }
    for (; i < n; i++) {
        uint32_t vi = src[i];
        uint32_t bi = (vi >> shift) & 0xff;
        dst[hist[bi]++] = vi;
    }
}

__attribute__((target("avx512f,avx512cd,avx2,bmi2")))
static void
radixs_u32_avx512f(const uint32_t *in, uint32_t *out, size_t n)
{
    if (n == 0) return;
    if (n == 1) { out[0] = in[0]; return; }

    uint32_t (*hist)[RADIXS_U32_BUCKETS] =
        aligned_alloc(64, sizeof(uint32_t) * RADIXS_U32_PASSES * RADIXS_U32_BUCKETS);
    if (!hist) { radixs_u32_qsort_fallback(in, out, n); return; }
    uint32_t *tmp = aligned_alloc(64, RADIXS_U32_TMP_BYTES(n));
    if (!tmp) { free(hist); radixs_u32_qsort_fallback(in, out, n); return; }

    unsigned skip = radixs_u32_build_histograms_avx2(in, n, hist);
    radixs_u32_prefix_sum(hist);

    int active = 0, pass_list[RADIXS_U32_PASSES];
    for (int p = 0; p < RADIXS_U32_PASSES; p++)
        if (!(skip & (1u << p))) pass_list[active++] = p;

    if (active == 0) {
        memcpy(out, in, n * sizeof(uint32_t));
        free(tmp); free(hist); return;
    }

    const uint32_t *src = in;
    uint32_t *dst = (active & 1) ? out : tmp;
    for (int idx = 0; idx < active; idx++) {
        radixs_u32_pass_avx512f(src, dst, n, hist[pass_list[idx]], pass_list[idx]);
        src = dst;
        dst = (dst == tmp) ? out : tmp;
    }

    free(tmp);
    free(hist);
}

/*
 * VBMI2 variant: uses vpermb to materialize 16 byte digits in a single
 * permute, replacing the shift+mask used by the AVX-512F scatter pass.
 * Falls through to the AVX-512F scatter for the actual store.
 */
__attribute__((target("avx512vbmi2,avx512vbmi,avx512bw,avx512f,avx512cd")))
static void
radixs_u32_pass_vbmi2(const uint32_t *src, uint32_t *dst, size_t n,
                     uint32_t *hist, int byte_idx)
{
    /*
     * Per-byte permute index: for byte_idx p, lane k of the 16xu32 vector
     * needs the byte at offset (k*4 + p) within the 64-byte input block.
     * We build that index once per pass.
     */
    alignas(64) uint8_t idx_bytes[64] = {0};
    for (int k = 0; k < 16; k++)
        idx_bytes[k * 4] = (uint8_t)(k * 4 + byte_idx);
    __m512i perm = _mm512_load_si512((const void *)idx_bytes);
    const __m512i mask = _mm512_set1_epi32(0xff);

    size_t i = 0;
    alignas(64) uint32_t buf_v[16];
    alignas(64) uint32_t buf_b[16];

    for (; i + 16 <= n; i += 16) {
        __m512i v = _mm512_loadu_si512((const void *)(src + i));
        __m512i b = _mm512_and_si512(_mm512_permutexvar_epi8(perm, v), mask);
        __m512i conflict = _mm512_conflict_epi32(b);
        __mmask16 unique = _mm512_cmpeq_epi32_mask(conflict, _mm512_setzero_si512());

        if (unique == 0xffff) {
            _mm512_store_si512((void *)buf_b, b);
            __m512i offsets = _mm512_set_epi32(
                hist[buf_b[15]], hist[buf_b[14]], hist[buf_b[13]], hist[buf_b[12]],
                hist[buf_b[11]], hist[buf_b[10]], hist[buf_b[ 9]], hist[buf_b[ 8]],
                hist[buf_b[ 7]], hist[buf_b[ 6]], hist[buf_b[ 5]], hist[buf_b[ 4]],
                hist[buf_b[ 3]], hist[buf_b[ 2]], hist[buf_b[ 1]], hist[buf_b[ 0]]);
            _mm512_i32scatter_epi32(dst, offsets, v, 4);
            for (int j = 0; j < 16; j++) hist[buf_b[j]]++;
        } else {
            _mm512_store_si512((void *)buf_v, v);
            _mm512_store_si512((void *)buf_b, b);
            for (int j = 0; j < 16; j++)
                dst[hist[buf_b[j]]++] = buf_v[j];
        }
    }
    int shift = byte_idx * 8;
    for (; i < n; i++) {
        uint32_t vi = src[i];
        uint32_t bi = (vi >> shift) & 0xff;
        dst[hist[bi]++] = vi;
    }
}

__attribute__((target("avx512vbmi2,avx512vbmi,avx512bw,avx512f,avx512cd,avx2,bmi2")))
static void
radixs_u32_avx512_vbmi2(const uint32_t *in, uint32_t *out, size_t n)
{
    if (n == 0) return;
    if (n == 1) { out[0] = in[0]; return; }

    uint32_t (*hist)[RADIXS_U32_BUCKETS] =
        aligned_alloc(64, sizeof(uint32_t) * RADIXS_U32_PASSES * RADIXS_U32_BUCKETS);
    if (!hist) { radixs_u32_qsort_fallback(in, out, n); return; }
    uint32_t *tmp = aligned_alloc(64, RADIXS_U32_TMP_BYTES(n));
    if (!tmp) { free(hist); radixs_u32_qsort_fallback(in, out, n); return; }

    unsigned skip = radixs_u32_build_histograms_avx2(in, n, hist);
    radixs_u32_prefix_sum(hist);

    int active = 0, pass_list[RADIXS_U32_PASSES];
    for (int p = 0; p < RADIXS_U32_PASSES; p++)
        if (!(skip & (1u << p))) pass_list[active++] = p;

    if (active == 0) {
        memcpy(out, in, n * sizeof(uint32_t));
        free(tmp); free(hist); return;
    }

    const uint32_t *src = in;
    uint32_t *dst = (active & 1) ? out : tmp;
    for (int idx = 0; idx < active; idx++) {
        radixs_u32_pass_vbmi2(src, dst, n, hist[pass_list[idx]], pass_list[idx]);
        src = dst;
        dst = (dst == tmp) ? out : tmp;
    }

    free(tmp);
    free(hist);
}
#endif /* x86 */

#if defined(__aarch64__)

static void
radixs_u32_neon(const uint32_t *in, uint32_t *out, size_t n)
{
    radixs_u32_run(in, out, n);
}

__attribute__((target("+sve")))
static void
radixs_u32_sve(const uint32_t *in, uint32_t *out, size_t n)
{
    radixs_u32_run(in, out, n);
}

__attribute__((target("+sve2")))
static void
radixs_u32_sve2(const uint32_t *in, uint32_t *out, size_t n)
{
    radixs_u32_run(in, out, n);
}

#endif /* aarch64 */

radixs_u32_fn_t
radixs_u32_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2: return radixs_u32_avx512_vbmi2;
    case SIMD_AVX512F: return radixs_u32_avx512f;
    case SIMD_AVX2:    return radixs_u32_avx2;
    case SIMD_AVX:     return radixs_u32_avx;
    case SIMD_SSE4_2:  return radixs_u32_sse42;
    case SIMD_SSE2:    return radixs_u32_sse2;
#endif
#if defined(__aarch64__)
    case SIMD_SVE2:    return radixs_u32_sve2;
    case SIMD_SVE:     return radixs_u32_sve;
    case SIMD_NEON:    return radixs_u32_neon;
#endif
    case SIMD_SCALAR:
    default:           return radixs_u32_scalar;
    }
}

static radixs_u32_fn_t
radixs_u32_resolver(void)
{
    return radixs_u32_select(detect_simd_level());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512vbmi2,avx512vbmi,avx512bw,avx512f,avx512cd,avx2,bmi2,avx,sse4.2,sse2")))
#elif defined(__aarch64__)
__attribute__((target("+sve2,+sve")))
#endif
void radixs_u32(const uint32_t *in, uint32_t *out, size_t n)
    __attribute__((ifunc("radixs_u32_resolver")));
