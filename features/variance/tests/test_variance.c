#include "unity.h"
#include <dynemit/variance.h>

void setUp(void) {}
void tearDown(void) {}

void test_variance_basic(void)
{
    double d[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 2.5, variance_f64(d, 5));
}

void test_variance_single_element(void)
{
    double d[] = {42.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, variance_f64(d, 1));
}

void test_variance_constant_array(void)
{
    double d[64];
    for (int i = 0; i < 64; i++) d[i] = 7.0;
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, variance_f64(d, 64));
}

void test_variance_all_variants(void)
{
    double d[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        variance_f64_fn_t fn = variance_f64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, 2.5, fn(d, 5));
    }
}

void test_variance_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++)
        TEST_ASSERT_NOT_NULL(variance_f64_select((simd_level_t)lvl));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_variance_basic);
    RUN_TEST(test_variance_single_element);
    RUN_TEST(test_variance_constant_array);
    RUN_TEST(test_variance_all_variants);
    RUN_TEST(test_variance_select_all_levels);
    return UNITY_END();
}
