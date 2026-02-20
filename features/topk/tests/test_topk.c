#include "unity.h"
#include <stdint.h>
#include <dynemit/topk.h>

void setUp(void) {}
void tearDown(void) {}

void test_topk_ratios(void)
{
    uint64_t data[] = {50, 30, 15, 5};
    size_t ks[] = {1, 2, 3};
    double ratios[3] = {0};
    topk_ratios_f64(data, 4, 100, ks, 3, ratios);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.50, ratios[0]);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.80, ratios[1]);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.95, ratios[2]);
}

void test_topk_ratios_all_variants(void)
{
    uint64_t data[] = {50, 30, 15, 5};
    size_t ks[] = {1, 2, 3};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        topk_ratios_f64_fn_t fn = topk_ratios_f64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        double ratios[3] = {0};
        fn(data, 4, 100, ks, 3, ratios);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.50, ratios[0]);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.80, ratios[1]);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.95, ratios[2]);
    }
}

void test_topk_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++)
        TEST_ASSERT_NOT_NULL(topk_ratios_f64_select((simd_level_t)lvl));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_topk_ratios);
    RUN_TEST(test_topk_ratios_all_variants);
    RUN_TEST(test_topk_select_all_levels);
    return UNITY_END();
}
