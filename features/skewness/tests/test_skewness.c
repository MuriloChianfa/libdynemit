#include "unity.h"
#include <dynemit/skewness.h>

void setUp(void) {}
void tearDown(void) {}

void test_skewness_symmetric(void)
{
    double d[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, skewness_f64(d, 5));
}

void test_skewness_right_skewed(void)
{
    double d[] = {1.0, 1.0, 1.0, 1.0, 1.0, 10.0};
    TEST_ASSERT_TRUE(skewness_f64(d, 6) > 0.0);
}

void test_skewness_all_variants(void)
{
    double d[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        skewness_f64_fn_t fn = skewness_f64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, fn(d, 5));
    }
}

void test_skewness_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++)
        TEST_ASSERT_NOT_NULL(skewness_f64_select((simd_level_t)lvl));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_skewness_symmetric);
    RUN_TEST(test_skewness_right_skewed);
    RUN_TEST(test_skewness_all_variants);
    RUN_TEST(test_skewness_select_all_levels);
    return UNITY_END();
}
