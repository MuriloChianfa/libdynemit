#include "unity.h"
#include <dynemit/kurtosis.h>

void setUp(void) {}
void tearDown(void) {}

void test_kurtosis_uniform(void)
{
    double d[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    TEST_ASSERT_TRUE(kurtosis_f64(d, 10) <= 0.0);
}

void test_kurtosis_peaked(void)
{
    double d[] = {0, 0, 0, 0, 100, 0, 0, 0, 0, 0};
    TEST_ASSERT_TRUE(kurtosis_f64(d, 10) > 0.0);
}

void test_kurtosis_all_variants(void)
{
    double d[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        kurtosis_f64_fn_t fn = kurtosis_f64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_TRUE(fn(d, 10) <= 0.0);
    }
}

void test_kurtosis_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++)
        TEST_ASSERT_NOT_NULL(kurtosis_f64_select((simd_level_t)lvl));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_kurtosis_uniform);
    RUN_TEST(test_kurtosis_peaked);
    RUN_TEST(test_kurtosis_all_variants);
    RUN_TEST(test_kurtosis_select_all_levels);
    return UNITY_END();
}
