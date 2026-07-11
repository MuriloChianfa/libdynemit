/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elifdef __aarch64__
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include "fast_log2.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#if DYNEMIT_TS
#include <pthread.h>
#endif
#include "mem.h"
#include <dynemit/compiler.h>
#include <dynemit/entropy.h>

/*
 * Thread-local pre-zeroed buffers for entropy_u16.
 *
 * Eliminates calloc/free (mmap/munmap) per call and the full 256 KB memset.
 * Only the histogram bins actually touched are zeroed after each call
 * (selective cleanup), keeping the buffer ready for the next invocation.
 *
 * When the number of unique values exceeds DIRTY_THRESHOLD, selective cleanup
 * becomes more expensive than a full memset, so we fall back to memset.
 *
 * Layout: [hist: 65536 x uint32_t] [dirty: 65536 x uint16_t]
 */
enum {
    EU16_BINS = 65536
};

/*
 * Crossover where memset(256KB) is cheaper than N scattered zeroes.
 * memset ~1.3 us; scattered stores ~5 ns each → crossover ~260.
 */
enum {
    EU16_DIRTY_THRESHOLD = 512
};

/*
 * When n >= this threshold the histogram is dense enough that a fused scan
 * (entropy + inline zeroing in one pass) beats compaction + SIMD entropy +
 * memset cleanup.  The compaction path becomes a bottleneck because its
 * scalar scan of 65536 bins already costs as much as the fused path, and
 * the subsequent 256 KB memset (triggered when ndirty > DIRTY_THRESHOLD)
 * pushes total cost well above a single fused pass.
 */
#define EU16_FUSED_THRESHOLD (EU16_BINS / 4)

#if DYNEMIT_TS
static pthread_key_t eu16_tls_key;
static pthread_once_t eu16_tls_once = PTHREAD_ONCE_INIT;
static int eu16_tls_pthread_ok;

static _Thread_local void *eu16_tls_fallback;

static void
eu16_tls_destructor(void *p)
{
    free(p);
}

static void
eu16_tls_init_once(void)
{
    eu16_tls_pthread_ok = (pthread_key_create(&eu16_tls_key, eu16_tls_destructor) == 0);
}

static inline int
eu16_get_bufs(uint32_t **hist, uint16_t **dirty)
{
    void *eu16_tls = nullptr;

    pthread_once(&eu16_tls_once, eu16_tls_init_once);
    if (eu16_tls_pthread_ok)
        eu16_tls = pthread_getspecific(eu16_tls_key);
    else
        eu16_tls = eu16_tls_fallback;

    if (__builtin_expect(!eu16_tls, 0)) {
        size_t sz = EU16_BINS * sizeof(uint32_t) + EU16_BINS * sizeof(uint16_t);
        eu16_tls = aligned_alloc(64, sz);
        if (!eu16_tls) return -1;
        if (memsets(eu16_tls, EU16_BINS * sizeof(uint32_t), 0,
                             EU16_BINS * sizeof(uint32_t)) != 0) {
            free(eu16_tls);
            return -1;
        }
        if (eu16_tls_pthread_ok) {
            if (pthread_setspecific(eu16_tls_key, eu16_tls) != 0) {
                free(eu16_tls);
                return -1;
            }
        } else {
            eu16_tls_fallback = eu16_tls;
        }
    }
    *hist  = (uint32_t *)eu16_tls;
    *dirty = (uint16_t *)((char *)eu16_tls + EU16_BINS * sizeof(uint32_t));
    return 0;
}
#else
static void *eu16_bufs = nullptr;

static inline int
eu16_get_bufs(uint32_t **hist, uint16_t **dirty)
{
    if (__builtin_expect(!eu16_bufs, 0)) {
        size_t sz = (EU16_BINS * sizeof(uint32_t)) + (EU16_BINS * sizeof(uint16_t));
        eu16_bufs = aligned_alloc(64, sz);
        if (!eu16_bufs) {
            return -1;
        }
        if (memsets(eu16_bufs, EU16_BINS * sizeof(uint32_t), 0,
                             EU16_BINS * sizeof(uint32_t)) != 0) {
            free(eu16_bufs);
            return -1;
        }
    }
    *hist  = (uint32_t *)eu16_bufs;
    *dirty = (uint16_t *)((char *)eu16_bufs + (EU16_BINS * sizeof(uint32_t)));
    return 0;
}
#endif

static inline void
eu16_cleanup(uint32_t *hist, const uint16_t *dirty,
             size_t ndirty, int tracked)
{
    if (tracked) {
        for (size_t k = 0; k < ndirty; k++) {
            hist[k] = 0;
            hist[dirty[k]] = 0;
        }
    } else {
        if (memsets(hist, EU16_BINS * sizeof(uint32_t), 0,
                             EU16_BINS * sizeof(uint32_t)) != 0) {
            return;
        }
    }
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static double
entropy_u16_scalar(const uint16_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    uint32_t *hist = nullptr;
    uint16_t *dirty = nullptr;
    if (eu16_get_bufs(&hist, &dirty)) {
        return 0.0;
    }

DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        hist[data[i]]++;
    }

    double inv_n = 1.0 / (double)n;
    double h = 0.0;

DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t j = 0; j < EU16_BINS; j++) {
        uint32_t c = hist[j];
        if (c == 0) {
            continue;
        }
        hist[j] = 0;
        double p = (double)c * inv_n;
        h -= p * fast_log2_scalar(p);
    }
    return h;
}


#if defined(__x86_64__) || defined(__i386__)

/*
 * Fused scan for SSE2: entropy + inline cleanup in one pass over the histogram.
 * Processes 2 bins per iteration (one pair of doubles) for tight zero skipping.
 * SSE2 can only compute 2 doubles at a time, so 2-bin groups avoid wasting
 * log2 calls on zero values within a wider group.
 */
__attribute__((target("sse2")))
static double
entropy_u16_sse2_fused(uint32_t *hist, size_t n)
{
    double inv_n = 1.0 / (double)n;
    __m128d vinv   = _mm_set1_pd(inv_n);
    __m128d vsum   = _mm_setzero_pd();
    __m128i vizero = _mm_setzero_si128();

    for (size_t j = 0; j < EU16_BINS; j += 2) {
        __m128i ci = _mm_loadl_epi64((const __m128i *)(hist + j));
        if (_mm_cvtsi128_si64(ci) == 0) {
            continue;
        }
        _mm_storel_epi64((__m128i *)(hist + j), vizero);

        __m128d cd = _mm_cvtepi32_pd(ci);
        __m128d p  = _mm_mul_pd(cd, vinv);
        __m128d l2 = fast_log2_pd_sse2(p);
        vsum = _mm_sub_pd(vsum, _mm_mul_pd(p, l2));
    }

    __m128d sh = _mm_unpackhi_pd(vsum, vsum);
    vsum = _mm_add_pd(vsum, sh);
    return _mm_cvtsd_f64(vsum);
}

__attribute__((target("sse2")))
static double
entropy_u16_sse2(const uint16_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    uint32_t *hist = nullptr;
    uint16_t *dirty = nullptr;
    if (eu16_get_bufs(&hist, &dirty)) {
        return 0.0;
    }

    for (size_t i = 0; i < n; i++) {
        hist[data[i]]++;
    }

    if (n >= EU16_FUSED_THRESHOLD) {
        return entropy_u16_sse2_fused(hist, n);
    }

    size_t ndirty = 0;
    int tracked = 1;
    for (size_t j = 0; j < EU16_BINS; j++) {
        if (hist[j] != 0) {
            if (tracked) {
                dirty[ndirty] = (uint16_t)j;
            }
            hist[ndirty] = hist[j];
            ndirty++;
            if (ndirty > EU16_DIRTY_THRESHOLD) {
                tracked = 0;
            }
        }
    }

    double inv_n = 1.0 / (double)n;
    __m128d vinv = _mm_set1_pd(inv_n);
    __m128d vsum = _mm_setzero_pd();

    size_t i = 0;
    for (; i + 2 <= ndirty; i += 2) {
        __m128i ci = _mm_loadl_epi64((const __m128i *)(hist + i));
        __m128d cd = _mm_cvtepi32_pd(ci);
        __m128d p  = _mm_mul_pd(cd, vinv);
        __m128d l2 = fast_log2_pd_sse2(p);
        vsum = _mm_sub_pd(vsum, _mm_mul_pd(p, l2));
    }

    __m128d sh = _mm_unpackhi_pd(vsum, vsum);
    vsum = _mm_add_pd(vsum, sh);
    double h = _mm_cvtsd_f64(vsum);

    for (; i < ndirty; i++) {
        double p = (double)hist[i] * inv_n;
        h -= p * fast_log2_scalar(p);
    }

    eu16_cleanup(hist, dirty, ndirty, tracked);
    return h;
}

__attribute__((target("sse4.2")))
static double
entropy_u16_sse42(const uint16_t *data, size_t n)
{
    return entropy_u16_sse2(data, n);
}

__attribute__((target("avx")))
static double
entropy_u16_avx_fused(uint32_t *hist, size_t n)
{
    double inv_n = 1.0 / (double)n;
    __m256d vinv   = _mm256_set1_pd(inv_n);
    __m256d vsum   = _mm256_setzero_pd();
    __m128i vizero = _mm_setzero_si128();

    for (size_t j = 0; j < EU16_BINS; j += 4) {
        __m128i vi = _mm_load_si128((const __m128i *)(hist + j));
        if (_mm_movemask_epi8(_mm_cmpeq_epi32(vi, vizero)) == 0xFFFF) {
            continue;
        }
        _mm_store_si128((__m128i *)(hist + j), vizero);

        __m256d cd = _mm256_cvtepi32_pd(vi);
        __m256d p  = _mm256_mul_pd(cd, vinv);
        __m256d l2 = fast_log2_pd_avx(p);
        vsum = _mm256_sub_pd(vsum, _mm256_mul_pd(p, l2));
    }

    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    lo = _mm_add_pd(lo, hi);
    lo = _mm_add_pd(lo, _mm_unpackhi_pd(lo, lo));
    return _mm_cvtsd_f64(lo);
}

__attribute__((target("avx")))
static double
entropy_u16_avx(const uint16_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    uint32_t *hist = nullptr;
    uint16_t *dirty = nullptr;
    if (eu16_get_bufs(&hist, &dirty)) {
        return 0.0;
    }

    for (size_t i = 0; i < n; i++) {
        hist[data[i]]++;
    }

    if (n >= EU16_FUSED_THRESHOLD) {
        return entropy_u16_avx_fused(hist, n);
    }

    size_t ndirty = 0;
    int tracked = 1;
    for (size_t j = 0; j < EU16_BINS; j++) {
        if (hist[j] != 0) {
            if (tracked) {
                dirty[ndirty] = (uint16_t)j;
            }
            hist[ndirty] = hist[j];
            ndirty++;
            if (ndirty > EU16_DIRTY_THRESHOLD) {
                tracked = 0;
            }
        }
    }

    double inv_n = 1.0 / (double)n;
    __m256d vinv = _mm256_set1_pd(inv_n);
    __m256d vsum = _mm256_setzero_pd();

    size_t i = 0;
    for (; i + 4 <= ndirty; i += 4) {
        __m128i ci = _mm_loadu_si128((const __m128i *)(hist + i));
        __m256d cd = _mm256_cvtepi32_pd(ci);
        __m256d p  = _mm256_mul_pd(cd, vinv);
        __m256d l2 = fast_log2_pd_avx(p);
        vsum = _mm256_sub_pd(vsum, _mm256_mul_pd(p, l2));
    }

    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    lo = _mm_add_pd(lo, hi);
    lo = _mm_add_pd(lo, _mm_unpackhi_pd(lo, lo));
    double h = _mm_cvtsd_f64(lo);

    for (; i < ndirty; i++) {
        double p = (double)hist[i] * inv_n;
        h -= p * fast_log2_scalar(p);
    }

    eu16_cleanup(hist, dirty, ndirty, tracked);
    return h;
}

__attribute__((target("avx2,fma")))
static double
entropy_u16_avx2_fused(uint32_t *hist, size_t n)
{
    double inv_n = 1.0 / (double)n;
    __m256d vinv   = _mm256_set1_pd(inv_n);
    __m256d vsum   = _mm256_setzero_pd();
    __m128i vizero = _mm_setzero_si128();

    for (size_t j = 0; j < EU16_BINS; j += 4) {
        __m128i vi = _mm_load_si128((const __m128i *)(hist + j));
        if (_mm_movemask_epi8(_mm_cmpeq_epi32(vi, vizero)) == 0xFFFF) {
            continue;
        }
        _mm_store_si128((__m128i *)(hist + j), vizero);

        __m256d cd = _mm256_cvtepi32_pd(vi);
        __m256d p  = _mm256_mul_pd(cd, vinv);
        __m256d l2 = fast_log2_pd_avx2_fma(p);
        vsum = _mm256_fnmadd_pd(p, l2, vsum);
    }

    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    lo = _mm_add_pd(lo, hi);
    lo = _mm_add_pd(lo, _mm_unpackhi_pd(lo, lo));
    return _mm_cvtsd_f64(lo);
}

__attribute__((target("avx2,fma")))
static double
entropy_u16_avx2(const uint16_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    uint32_t *hist = nullptr;
    uint16_t *dirty = nullptr;
    if (eu16_get_bufs(&hist, &dirty)) {
        return 0.0;
    }

    for (size_t i = 0; i < n; i++) {
        hist[data[i]]++;
    }

    if (n >= EU16_FUSED_THRESHOLD) {
        return entropy_u16_avx2_fused(hist, n);
    }

    size_t ndirty = 0;
    int tracked = 1;
    for (size_t j = 0; j < EU16_BINS; j++) {
        if (hist[j] != 0) {
            if (tracked) {
                dirty[ndirty] = (uint16_t)j;
            }
            hist[ndirty] = hist[j];
            ndirty++;
            if (ndirty > EU16_DIRTY_THRESHOLD) {
                tracked = 0;
            }
        }
    }

    double inv_n = 1.0 / (double)n;
    __m256d vinv = _mm256_set1_pd(inv_n);
    __m256d vsum = _mm256_setzero_pd();

    size_t i = 0;
    for (; i + 4 <= ndirty; i += 4) {
        __m128i ci = _mm_loadu_si128((const __m128i *)(hist + i));
        __m256d cd = _mm256_cvtepi32_pd(ci);
        __m256d p  = _mm256_mul_pd(cd, vinv);
        __m256d l2 = fast_log2_pd_avx2_fma(p);
        vsum = _mm256_fnmadd_pd(p, l2, vsum);
    }

    __m128d lo = _mm256_castpd256_pd128(vsum);
    __m128d hi = _mm256_extractf128_pd(vsum, 1);
    lo = _mm_add_pd(lo, hi);
    lo = _mm_add_pd(lo, _mm_unpackhi_pd(lo, lo));
    double h = _mm_cvtsd_f64(lo);

    for (; i < ndirty; i++) {
        double p = (double)hist[i] * inv_n;
        h -= p * fast_log2_scalar(p);
    }

    eu16_cleanup(hist, dirty, ndirty, tracked);
    return h;
}

/*
 * Fused scan: compute entropy + inline cleanup in one pass over the histogram.
 * Eliminates compaction and separate cleanup.  Best when n >= EU16_FUSED_THRESHOLD:
 * the histogram is dense enough that the compaction + memset overhead exceeds
 * the cost of scanning all 65536 bins with SIMD early-out on zero chunks.
 */
__attribute__((target("avx512f")))
static double
entropy_u16_avx512f_fused(uint32_t *hist, size_t n)
{
    double inv_n = 1.0 / (double)n;
    __m512d vinv = _mm512_set1_pd(inv_n);
    __m512d vsum = _mm512_setzero_pd();
    __m512i vizero = _mm512_setzero_si512();

    for (size_t j = 0; j < EU16_BINS; j += 16) {
        __m512i vi = _mm512_load_si512(hist + j);
        __mmask16 nz = _mm512_cmpneq_epi32_mask(vi, vizero);
        if (nz == 0) {
            continue;
        }
        _mm512_store_si512(hist + j, vizero);

        __m256i lo8 = _mm512_castsi512_si256(vi);
        __m512d cd = _mm512_cvtepi32_pd(lo8);
        __mmask8 nzlo = (__mmask8)(nz & 0xFF);
        __m512d p  = _mm512_mul_pd(cd, vinv);
        __m512d l2 = fast_log2_pd_avx512(p);
        vsum = _mm512_mask3_fnmadd_pd(p, l2, vsum, nzlo);

        __m256i hi8 = _mm256_castpd_si256(
            _mm512_extractf64x4_pd(_mm512_castsi512_pd(vi), 1));
        cd = _mm512_cvtepi32_pd(hi8);
        __mmask8 nzhi = (__mmask8)((nz >> 8) & 0xFF);
        p  = _mm512_mul_pd(cd, vinv);
        l2 = fast_log2_pd_avx512(p);
        vsum = _mm512_mask3_fnmadd_pd(p, l2, vsum, nzhi);
    }

    __m256d lo4 = _mm512_castpd512_pd256(vsum);
    __m256d hi4 = _mm512_extractf64x4_pd(vsum, 1);
    __m256d s4  = _mm256_add_pd(lo4, hi4);
    __m128d lo2 = _mm256_castpd256_pd128(s4);
    __m128d hi2 = _mm256_extractf128_pd(s4, 1);
    __m128d s2  = _mm_add_pd(lo2, hi2);
    s2 = _mm_add_pd(s2, _mm_unpackhi_pd(s2, s2));
    return _mm_cvtsd_f64(s2);
}

__attribute__((target("avx512f")))
static double
entropy_u16_avx512f(const uint16_t *data, size_t n)
{
    if (n == 0) {
        return 0.0;
    }
    uint32_t *hist = nullptr;
    uint16_t *dirty = nullptr;
    if (eu16_get_bufs(&hist, &dirty)) {
        return 0.0;
    }

    for (size_t i = 0; i < n; i++) {
        hist[data[i]]++;
    }

    if (n >= EU16_FUSED_THRESHOLD) {
        return entropy_u16_avx512f_fused(hist, n);
    }

    /* compress-store compaction + dirty index recording for small n */
    size_t ndirty = 0;
    int tracked = 1;
    __m512i vzero = _mm512_setzero_si512();
    for (size_t j = 0; j < EU16_BINS; j += 16) {
        __m512i v  = _mm512_load_si512(hist + j);
        __mmask16 nz = _mm512_cmpneq_epi32_mask(v, vzero);
        if (nz == 0) {
            continue;
        }
        _mm512_mask_compressstoreu_epi32(hist + ndirty, nz, v);
        if (tracked) {
            unsigned mask = (unsigned)nz;
            size_t pos = 0;
            while (mask) {
                dirty[ndirty + pos] = (uint16_t)(j + (unsigned)__builtin_ctz(mask));
                pos++;
                mask &= mask - 1;
            }
        }
        ndirty += (size_t)_mm_popcnt_u32((unsigned)nz);
        if (ndirty > EU16_DIRTY_THRESHOLD) {
            tracked = 0;
        }
    }

    double inv_n = 1.0 / (double)n;
    __m512d vinv = _mm512_set1_pd(inv_n);
    __m512d vsum = _mm512_setzero_pd();

    size_t i = 0;
    for (; i + 8 <= ndirty; i += 8) {
        __m256i ci = _mm256_loadu_si256((const __m256i *)(hist + i));
        __m512d cd = _mm512_cvtepi32_pd(ci);
        __m512d p  = _mm512_mul_pd(cd, vinv);
        __m512d l2 = fast_log2_pd_avx512(p);
        vsum = _mm512_fnmadd_pd(p, l2, vsum);
    }

    __m256d lo4 = _mm512_castpd512_pd256(vsum);
    __m256d hi4 = _mm512_extractf64x4_pd(vsum, 1);
    __m256d s4  = _mm256_add_pd(lo4, hi4);
    __m128d lo2 = _mm256_castpd256_pd128(s4);
    __m128d hi2 = _mm256_extractf128_pd(s4, 1);
    __m128d s2  = _mm_add_pd(lo2, hi2);
    s2 = _mm_add_pd(s2, _mm_unpackhi_pd(s2, s2));
    double h = _mm_cvtsd_f64(s2);

    for (; i < ndirty; i++) {
        double p = (double)hist[i] * inv_n;
        h -= p * fast_log2_scalar(p);
    }

    eu16_cleanup(hist, dirty, ndirty, tracked);
    return h;
}

#endif /* x86 */


#ifdef __aarch64__

static double
entropy_u16_neon(const uint16_t *data, size_t n)
{
    return entropy_u16_scalar(data, n);
}

__attribute__((target("+sve")))
static double
entropy_u16_sve(const uint16_t *data, size_t n)
{
    return entropy_u16_scalar(data, n);
}

__attribute__((target("+sve2")))
static double
entropy_u16_sve2(const uint16_t *data, size_t n)
{
    return entropy_u16_scalar(data, n);
}

#endif /* aarch64 */

entropy_u16_fn_t
entropy_u16_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return entropy_u16_avx512f;
    case SIMD_AVX2:    return entropy_u16_avx2;
    case SIMD_AVX:     return entropy_u16_avx;
    case SIMD_SSE4_2:  return entropy_u16_sse42;
    case SIMD_SSE2:    return entropy_u16_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return entropy_u16_sve2;
    case SIMD_SVE:     return entropy_u16_sve;
    case SIMD_NEON:    return entropy_u16_neon;
#endif
    case SIMD_SCALAR:
    default:           return entropy_u16_scalar;
}
}

EXPLICIT_RUNTIME_RESOLVER(entropy_u16_resolver, entropy_u16_fn_t)
{
    return entropy_u16_select(detect_simd_level_ts());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
double entropy_u16(const uint16_t *data, size_t n)
    __attribute__((ifunc("entropy_u16_resolver")));
