#include "unity.h"
#include <stdint.h>
#include <dynemit/gini.h>

void setUp(void) {}
void tearDown(void) {}

void test_gini_f64_equal(void)
{
    double d[] = {5.0, 5.0, 5.0, 5.0, 5.0};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, gini_f64(d, 5));
}

void test_gini_f64_unequal(void)
{
    double d[] = {0.0, 0.0, 0.0, 0.0, 100.0};
    TEST_ASSERT_TRUE(gini_f64(d, 5) > 0.5);
}

void test_gini_u64_equal(void)
{
    uint64_t d[] = {10, 10, 10, 10};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, gini_u64(d, 4));
}

void test_gini_f64_all_variants(void)
{
    double d[] = {5.0, 5.0, 5.0, 5.0, 5.0};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        gini_f64_fn_t fn = gini_f64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, fn(d, 5));
    }
}

void test_gini_u64_all_variants(void)
{
    uint64_t d[] = {10, 10, 10, 10};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        gini_u64_fn_t fn = gini_u64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, fn(d, 4));
    }
}

void test_gini_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++) {
        TEST_ASSERT_NOT_NULL(gini_f64_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(gini_u64_select((simd_level_t)lvl));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_gini_f64_equal);
    RUN_TEST(test_gini_f64_unequal);
    RUN_TEST(test_gini_u64_equal);
    RUN_TEST(test_gini_f64_all_variants);
    RUN_TEST(test_gini_u64_all_variants);
    RUN_TEST(test_gini_select_all_levels);
    return UNITY_END();
}
