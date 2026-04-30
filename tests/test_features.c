#include "unity.h"
#include <string.h>
#include <dynemit.h>

void setUp(void) {}
void tearDown(void) {}

void test_features_returns_non_null(void)
{
    const char **features = dynemit_features();
    TEST_ASSERT_NOT_NULL(features);
}

void test_features_has_at_least_one(void)
{
    const char **features = dynemit_features();
    TEST_ASSERT_NOT_NULL(features[0]);
}

void test_features_list_is_terminated(void)
{
    const char **features = dynemit_features();
    int count = 0;
    while (features[count] != nullptr)
        count++;
    TEST_ASSERT_GREATER_THAN(0, count);
}

void test_features_count(void)
{
    const char **features = dynemit_features();
    int count = 0;
    while (features[count] != nullptr)
        count++;
    TEST_ASSERT_EQUAL_INT(1, count);
}

void test_features_first_is_core(void)
{
    const char **features = dynemit_features();
    TEST_ASSERT_EQUAL_STRING("core", features[0]);
}

void test_simd_detection(void)
{
    simd_level_t level = detect_simd_level();
    TEST_ASSERT_TRUE(level >= SIMD_SCALAR && level <= SIMD_AVX512F);
    const char *name = simd_level_name(level);
    TEST_ASSERT_NOT_NULL(name);
}

void test_simd_level_name_all_values(void)
{
    TEST_ASSERT_EQUAL_STRING("Scalar",   simd_level_name(SIMD_SCALAR));
    TEST_ASSERT_EQUAL_STRING("SSE2",     simd_level_name(SIMD_SSE2));
    TEST_ASSERT_EQUAL_STRING("SSE4.2",   simd_level_name(SIMD_SSE4_2));
    TEST_ASSERT_EQUAL_STRING("AVX",      simd_level_name(SIMD_AVX));
    TEST_ASSERT_EQUAL_STRING("AVX2",     simd_level_name(SIMD_AVX2));
    TEST_ASSERT_EQUAL_STRING("AVX-512F", simd_level_name(SIMD_AVX512F));
}

void test_simd_level_name_unknown(void)
{
    const char *name = simd_level_name((simd_level_t)999);
    TEST_ASSERT_EQUAL_STRING("Unknown", name);
}

void test_detect_simd_level_ts_matches(void)
{
    simd_level_t direct = detect_simd_level();
    simd_level_t cached = detect_simd_level_ts();
    TEST_ASSERT_EQUAL_INT(direct, cached);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_features_returns_non_null);
    RUN_TEST(test_features_has_at_least_one);
    RUN_TEST(test_features_list_is_terminated);
    RUN_TEST(test_features_count);
    RUN_TEST(test_features_first_is_core);
    RUN_TEST(test_simd_detection);
    RUN_TEST(test_simd_level_name_all_values);
    RUN_TEST(test_simd_level_name_unknown);
    RUN_TEST(test_detect_simd_level_ts_matches);
    return UNITY_END();
}
