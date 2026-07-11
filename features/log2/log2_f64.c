/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#endif
#include "fast_log2.h"
#include <dynemit/compiler.h>
#include <dynemit/log2.h>
#include <stddef.h>

#if defined(__x86_64__) || defined(__i386__)
DYNEMIT_TARGET_DEFAULT
#endif
DYNEMIT_NO_AUTOVECTORIZE
static void
log2_f64_scalar(const double *in, double *out, size_t n)
{
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        out[i] = fast_log2_scalar(in[i]);
    }
}


#if defined(__x86_64__) || defined(__i386__)

__attribute__((target("sse2")))
static void
log2_f64_sse2(const double *in, double *out, size_t n)
{
    size_t i = 0;
    for (; i + 2 <= n; i += 2) {
        __m128d v = _mm_loadu_pd(in + i);
        __m128d r = fast_log2_pd_sse2(v);
        _mm_storeu_pd(out + i, r);
    }
    for (; i < n; i++) {
        out[i] = fast_log2_scalar(in[i]);
    }
}

__attribute__((target("sse4.2")))
static void
log2_f64_sse42(const double *in, double *out, size_t n)
{
    log2_f64_sse2(in, out, n);
}

__attribute__((target("avx")))
static void
log2_f64_avx(const double *in, double *out, size_t n)
{
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        __m256d v = _mm256_loadu_pd(in + i);
        __m256d r = fast_log2_pd_avx(v);
        _mm256_storeu_pd(out + i, r);
    }
    for (; i < n; i++) {
        out[i] = fast_log2_scalar(in[i]);
    }
}

__attribute__((target("avx2,fma")))
static void
log2_f64_avx2(const double *in, double *out, size_t n)
{
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        __m256d v = _mm256_loadu_pd(in + i);
        __m256d r = fast_log2_pd_avx2_fma(v);
        _mm256_storeu_pd(out + i, r);
    }
    for (; i < n; i++) {
        out[i] = fast_log2_scalar(in[i]);
    }
}

__attribute__((target("avx512f")))
static void
log2_f64_avx512f(const double *in, double *out, size_t n)
{
    size_t i = 0;
    for (; i + 8 <= n; i += 8) {
        __m512d v = _mm512_loadu_pd(in + i);
        __m512d r = fast_log2_pd_avx512(v);
        _mm512_storeu_pd(out + i, r);
    }
    for (; i < n; i++) {
        out[i] = fast_log2_scalar(in[i]);
    }
}

#endif /* x86 */

#ifdef __aarch64__

static void
log2_f64_neon(const double *in, double *out, size_t n)
{
    log2_f64_scalar(in, out, n);
}

__attribute__((target("+sve")))
static void
log2_f64_sve(const double *in, double *out, size_t n)
{
    log2_f64_scalar(in, out, n);
}

__attribute__((target("+sve2")))
static void
log2_f64_sve2(const double *in, double *out, size_t n)
{
    log2_f64_scalar(in, out, n);
}

#endif /* aarch64 */

log2_f64_fn_t
log2_f64_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return log2_f64_avx512f;
    case SIMD_AVX2:    return log2_f64_avx2;
    case SIMD_AVX:     return log2_f64_avx;
    case SIMD_SSE4_2:  return log2_f64_sse42;
    case SIMD_SSE2:    return log2_f64_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return log2_f64_sve2;
    case SIMD_SVE:     return log2_f64_sve;
    case SIMD_NEON:    return log2_f64_neon;
#endif
    case SIMD_SCALAR:
    default:           return log2_f64_scalar;
}
}

EXPLICIT_RUNTIME_RESOLVER(log2_f64_resolver, log2_f64_fn_t)
{
    return log2_f64_select(detect_simd_level_ts());
}
DYNEMIT_IFUNC_SETUP(log2_f64_fn_t, log2_f64, log2_f64_resolver)

#if defined(DYNEMIT_NO_IFUNC)
void log2_f64(const double *in, double *out, size_t n)
{
    DYNEMIT_IFUNC_INVOKE(log2_f64, (in, out, n));
}
#else
#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
void log2_f64(const double *in, double *out, size_t n)
    DYNEMIT_IFUNC_ATTR("log2_f64_resolver");
#endif
