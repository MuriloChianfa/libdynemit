#include "unity.h"
#include <dynemit/core.h>
#include <dynemit/err.h>

static int sample_func_avx2(int x) { return x * 4; }
static int sample_func_sse2(int x) { return x * 2; }
static int sample_func_scalar(int x) { return x * 1; }

typedef int (*sample_func_t)(int);

EXPLICIT_RUNTIME_RESOLVER(sample_resolver, sample_func_t)
{
    simd_level_t level = detect_simd_level_ts();
    switch (level) {
    case SIMD_AVX512F:
    case SIMD_AVX2:    return sample_func_avx2;
    case SIMD_AVX:
    case SIMD_SSE4_2:
    case SIMD_SSE2:    return sample_func_sse2;
    case SIMD_SCALAR:
    default:           return sample_func_scalar;
    }
}

EXPLICIT_RUNTIME_RESOLVER(scalar_only_resolver, sample_func_t)
{
    (void)detect_simd_level_ts();
    return sample_func_scalar;
}

void setUp(void) {}
void tearDown(void) {}

void test_resolver_returns_non_null(void)
{
    TEST_ASSERT_NOT_NULL(sample_resolver());
}

void test_resolver_matches_simd_level(void)
{
    simd_level_t level = detect_simd_level_ts();
    sample_func_t func = sample_resolver();
    int result = func(10);

    int expected;
    switch (level) {
    case SIMD_AVX512F:
    case SIMD_AVX2:    expected = 40; break;
    case SIMD_AVX:
    case SIMD_SSE4_2:
    case SIMD_SSE2:    expected = 20; break;
    default:           expected = 10; break;
    }
    TEST_ASSERT_EQUAL_INT(expected, result);
}

void test_resolver_consistent(void)
{
    sample_func_t a = sample_resolver();
    sample_func_t b = sample_resolver();
    TEST_ASSERT_EQUAL_PTR(a, b);
}

void test_scalar_only_resolver(void)
{
    sample_func_t func = scalar_only_resolver();
    TEST_ASSERT_NOT_NULL(func);
    TEST_ASSERT_EQUAL_INT(10, func(10));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_resolver_returns_non_null);
    RUN_TEST(test_resolver_matches_simd_level);
    RUN_TEST(test_resolver_consistent);
    RUN_TEST(test_scalar_only_resolver);
    return UNITY_END();
}
