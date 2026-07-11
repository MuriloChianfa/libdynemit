/* SPDX-License-Identifier: BSL-1.0 */
#if defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#elifdef __aarch64__
#include <arm_neon.h>
#include <arm_sve.h>
#endif
#include <dynemit/add.h>
#include <dynemit/compiler.h>
#include <stddef.h>

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("default")))
#endif
DYNEMIT_NO_AUTOVECTORIZE
static void
add_f32_scalar(const float *a, const float *b, float *out, size_t n)
{
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    for (size_t i = 0; i < n; i++) {
        out[i] = a[i] + b[i];
    }
}


#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("sse2")))
static void
add_f32_sse2(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 4;
    for (; i + step <= n; i += step) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 vc = _mm_add_ps(va, vb);
        _mm_storeu_ps(out + i, vc);
    }
    for (; i < n; i++) {
        out[i] = a[i] + b[i];
    }
}

__attribute__((target("sse4.2")))
static void
add_f32_sse42(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 4;
    for (; i + step <= n; i += step) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 vc = _mm_add_ps(va, vb);
        _mm_storeu_ps(out + i, vc);
    }
    for (; i < n; i++) {
        out[i] = a[i] + b[i];
    }
}

__attribute__((target("avx")))
static void
add_f32_avx(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 8;
    for (; i + step <= n; i += step) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 vc = _mm256_add_ps(va, vb);
        _mm256_storeu_ps(out + i, vc);
    }
    for (; i < n; i++) {
        out[i] = a[i] + b[i];
    }
}

__attribute__((target("avx2")))
static void
add_f32_avx2(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 8;
    for (; i + step <= n; i += step) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 vc = _mm256_add_ps(va, vb);
        _mm256_storeu_ps(out + i, vc);
    }
    for (; i < n; i++) {
        out[i] = a[i] + b[i];
    }
}

__attribute__((target("avx512f")))
static void
add_f32_avx512f(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 16;
    for (; i + step <= n; i += step) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        __m512 vc = _mm512_add_ps(va, vb);
        _mm512_storeu_ps(out + i, vc);
    }
    for (; i < n; i++) {
        out[i] = a[i] + b[i];
    }
}
#endif

#ifdef __aarch64__

static void
add_f32_neon(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    for (; i + 4 <= n; i += 4) {
        float32x4_t va = vld1q_f32(a + i);
        float32x4_t vb = vld1q_f32(b + i);
        vst1q_f32(out + i, vaddq_f32(va, vb));
    }
    for (; i < n; i++)
        out[i] = a[i] + b[i];
}

__attribute__((target("+sve")))
static void
add_f32_sve(const float *a, const float *b, float *out, size_t n)
{
    uint64_t i = 0;
    svbool_t pg = svwhilelt_b32(i, (uint64_t)n);
    do {
        svfloat32_t va = svld1_f32(pg, a + i);
        svfloat32_t vb = svld1_f32(pg, b + i);
        svst1_f32(pg, out + i, svadd_f32_x(pg, va, vb));
        i += svcntw();
        pg = svwhilelt_b32(i, (uint64_t)n);
    } while (svptest_any(svptrue_b32(), pg));
}

__attribute__((target("+sve2")))
static void
add_f32_sve2(const float *a, const float *b, float *out, size_t n)
{
    uint64_t i = 0;
    uint64_t vl = svcntw();
    svbool_t ptrue = svptrue_b32();
    for (; i + 2 * vl <= n; i += 2 * vl) {
        svfloat32_t va0 = svld1_f32(ptrue, a + i);
        svfloat32_t vb0 = svld1_f32(ptrue, b + i);
        svfloat32_t va1 = svld1_f32(ptrue, a + i + vl);
        svfloat32_t vb1 = svld1_f32(ptrue, b + i + vl);
        svst1_f32(ptrue, out + i, svadd_f32_x(ptrue, va0, vb0));
        svst1_f32(ptrue, out + i + vl, svadd_f32_x(ptrue, va1, vb1));
    }
    svbool_t pg = svwhilelt_b32(i, (uint64_t)n);
    while (svptest_any(ptrue, pg)) {
        svfloat32_t va = svld1_f32(pg, a + i);
        svfloat32_t vb = svld1_f32(pg, b + i);
        svst1_f32(pg, out + i, svadd_f32_x(pg, va, vb));
        i += vl;
        pg = svwhilelt_b32(i, (uint64_t)n);
    }
}

#endif /* aarch64 */

add_f32_fn_t
add_f32_select(simd_level_t level)
{
    switch (level) {
#if defined(__x86_64__) || defined(__i386__)
    case SIMD_AVX512_VBMI2:
    case SIMD_AVX512F: return add_f32_avx512f;
    case SIMD_AVX2:    return add_f32_avx2;
    case SIMD_AVX:     return add_f32_avx;
    case SIMD_SSE4_2:  return add_f32_sse42;
    case SIMD_SSE2:    return add_f32_sse2;
#endif
#ifdef __aarch64__
    case SIMD_SVE2:    return add_f32_sve2;
    case SIMD_SVE:     return add_f32_sve;
    case SIMD_NEON:    return add_f32_neon;
#endif
    case SIMD_SCALAR:
    default:           return add_f32_scalar;
}
}

EXPLICIT_RUNTIME_RESOLVER(add_f32_resolver, add_f32_fn_t)
{
    return add_f32_select(detect_simd_level_ts());
}

#if defined(__x86_64__) || defined(__i386__)
__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
#elifdef __aarch64__
__attribute__((target("+sve2,+sve")))
#endif
void add_f32(const float *a, const float *b, float *out, size_t n)
    __attribute__((ifunc("add_f32_resolver")));
