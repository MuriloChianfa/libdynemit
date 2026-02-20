#include "unity.h"
#include <stdint.h>
#include <math.h>
#include <dynemit/hill.h>

void setUp(void) {}
void tearDown(void) {}

void test_hill_heavy_tail(void)
{
    uint64_t d[] = {1000, 500, 200, 100, 50, 20, 10, 5, 2, 1};
    double r = hill_estimator_f64(d, 10);
    TEST_ASSERT_TRUE(isfinite(r));
    TEST_ASSERT_TRUE(r > 0.0);
}

void test_hill_flat(void)
{
    uint64_t d[] = {10, 10, 10, 10, 10};
    double r = hill_estimator_f64(d, 5);
    TEST_ASSERT_TRUE(isfinite(r) || isnan(r));
}

void test_hill_all_variants(void)
{
    uint64_t d[] = {1000, 500, 200, 100, 50, 20, 10, 5, 2, 1};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        hill_estimator_f64_fn_t fn = hill_estimator_f64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        double r = fn(d, 10);
        TEST_ASSERT_TRUE(isfinite(r));
        TEST_ASSERT_TRUE(r > 0.0);
    }
}

void test_hill_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++)
        TEST_ASSERT_NOT_NULL(hill_estimator_f64_select((simd_level_t)lvl));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_hill_heavy_tail);
    RUN_TEST(test_hill_flat);
    RUN_TEST(test_hill_all_variants);
    RUN_TEST(test_hill_select_all_levels);
    return UNITY_END();
}
