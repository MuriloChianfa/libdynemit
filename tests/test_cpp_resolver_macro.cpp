/**
 * C++ IFUNC Resolver Macro Test (GTest)
 *
 * Tests that the EXPLICIT_RUNTIME_RESOLVER macro works correctly
 * when used in C++ code. This ensures C++ projects can create
 * their own IFUNC resolvers using the library's helpers.
 */

#include <gtest/gtest.h>
#include <dynemit/core.h>
#include <dynemit/err.h>

static void test_func_scalar(float* out, const float* a, const float* b, size_t n)
{
    for (size_t i = 0; i < n; i++)
        out[i] = a[i] + b[i] + 1.0f;
}

static void test_func_sse2(float* out, const float* a, const float* b, size_t n)
{
    for (size_t i = 0; i < n; i++)
        out[i] = a[i] + b[i] + 2.0f;
}

static void test_func_avx(float* out, const float* a, const float* b, size_t n)
{
    for (size_t i = 0; i < n; i++)
        out[i] = a[i] + b[i] + 3.0f;
}

static void test_func_avx2(float* out, const float* a, const float* b, size_t n)
{
    for (size_t i = 0; i < n; i++)
        out[i] = a[i] + b[i] + 4.0f;
}

static void test_func_avx512(float* out, const float* a, const float* b, size_t n)
{
    for (size_t i = 0; i < n; i++)
        out[i] = a[i] + b[i] + 5.0f;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

extern "C" {
EXPLICIT_RUNTIME_RESOLVER(test_func_resolver)
{
    simd_level_t level = detect_simd_level_ts();

    switch (level) {
    case SIMD_AVX512F:
        return reinterpret_cast<void*>(test_func_avx512);
    case SIMD_AVX2:
        return reinterpret_cast<void*>(test_func_avx2);
    case SIMD_AVX:
        return reinterpret_cast<void*>(test_func_avx);
    case SIMD_SSE4_2:
    case SIMD_SSE2:
        return reinterpret_cast<void*>(test_func_sse2);
    default:
        return reinterpret_cast<void*>(test_func_scalar);
    }
}

void test_func_cpp(float* out, const float* a, const float* b, size_t n)
    __attribute__((ifunc("test_func_resolver")));
}

#pragma GCC diagnostic pop

TEST(CppResolverMacro, DispatchesCorrectImpl) {
    constexpr size_t N = 8;
    float a[N], b[N], result[N];

    for (size_t i = 0; i < N; i++) {
        a[i] = static_cast<float>(i);
        b[i] = static_cast<float>(i + 1);
    }

    test_func_cpp(result, a, b, N);

    simd_level_t level = detect_simd_level_ts();
    float expected_constant;
    switch (level) {
    case SIMD_AVX512F: expected_constant = 5.0f; break;
    case SIMD_AVX2:    expected_constant = 4.0f; break;
    case SIMD_AVX:     expected_constant = 3.0f; break;
    case SIMD_SSE4_2:
    case SIMD_SSE2:    expected_constant = 2.0f; break;
    default:           expected_constant = 1.0f; break;
    }

    for (size_t i = 0; i < N; i++) {
        EXPECT_NEAR(result[i], a[i] + b[i] + expected_constant, 1e-6f)
            << "Mismatch at index " << i
            << " (SIMD level: " << simd_level_name(level) << ")";
    }
}
