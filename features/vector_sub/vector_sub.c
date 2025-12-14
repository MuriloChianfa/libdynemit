#include <immintrin.h>
#include <stddef.h>
#include <dynemit/core.h>

// Scalar version - disable auto-vectorization to get true scalar code
__attribute__((target("default")))
__attribute__((optimize("no-tree-vectorize")))
static void
vector_sub_f32_scalar(const float *a, const float *b, float *out, size_t n)
{
    for (size_t i = 0; i < n; i++)
        out[i] = a[i] - b[i];
}

__attribute__((target("sse2")))
static void
vector_sub_f32_sse2(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 4;
    for (; i + step <= n; i += step) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 vc = _mm_sub_ps(va, vb);
        _mm_storeu_ps(out + i, vc);
    }
    for (; i < n; i++)
        out[i] = a[i] - b[i];
}

__attribute__((target("sse4.2")))
static void
vector_sub_f32_sse42(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 4;
    for (; i + step <= n; i += step) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 vc = _mm_sub_ps(va, vb);
        _mm_storeu_ps(out + i, vc);
    }
    for (; i < n; i++)
        out[i] = a[i] - b[i];
}

__attribute__((target("avx")))
static void
vector_sub_f32_avx(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 8;
    for (; i + step <= n; i += step) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 vc = _mm256_sub_ps(va, vb);
        _mm256_storeu_ps(out + i, vc);
    }
    for (; i < n; i++)
        out[i] = a[i] - b[i];
}

__attribute__((target("avx2")))
static void
vector_sub_f32_avx2(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 8;
    for (; i + step <= n; i += step) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 vc = _mm256_sub_ps(va, vb);
        _mm256_storeu_ps(out + i, vc);
    }
    for (; i < n; i++)
        out[i] = a[i] - b[i];
}

__attribute__((target("avx512f")))
static void
vector_sub_f32_avx512f(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 16;
    for (; i + step <= n; i += step) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        __m512 vc = _mm512_sub_ps(va, vb);
        _mm512_storeu_ps(out + i, vc);
    }
    for (; i < n; i++)
        out[i] = a[i] - b[i];
}

// ===================================================
// Resolver function for ifunc
// ===================================================

typedef void (*vector_sub_f32_func_t)(const float *, const float *, float *, size_t);

static vector_sub_f32_func_t
vector_sub_f32_resolver(void)
{
    simd_level_t level = detect_simd_level();
    
    switch (level) {
    case SIMD_AVX512F: return vector_sub_f32_avx512f;
    case SIMD_AVX2:    return vector_sub_f32_avx2;
    case SIMD_AVX:     return vector_sub_f32_avx;
    case SIMD_SSE4_2:  return vector_sub_f32_sse42;
    case SIMD_SSE2:    return vector_sub_f32_sse2;
    case SIMD_SCALAR:
    default:           return vector_sub_f32_scalar;
    }
}

__attribute__((target("avx512f,avx2,avx,sse4.2,sse2")))
void vector_sub_f32(const float *a, const float *b, float *out, size_t n)
    __attribute__((ifunc("vector_sub_f32_resolver")));
