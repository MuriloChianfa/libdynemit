#include "unity.h"
#include <stdint.h>
#include <math.h>
#include <dynemit/concentration.h>

void setUp(void) {}
void tearDown(void) {}

void test_concentration_basic(void)
{
    uint64_t data[] = {500, 300, 150, 30, 15, 5};
    size_t ks[] = {1, 3};
    double topk_buf[2] = {0};
    concentration_result_t result = {
        .topk_ratios = topk_buf,
        .heavy_tail_index = 0.0,
        .concentration = 0.0,
    };

    concentration_f64(data, 6, 1000, ks, 2, &result);

    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.50, result.topk_ratios[0]);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.95, result.topk_ratios[1]);
    TEST_ASSERT_TRUE(isfinite(result.heavy_tail_index));
    TEST_ASSERT_TRUE(isfinite(result.concentration));
}

void test_concentration_all_variants(void)
{
    uint64_t data[] = {500, 300, 150, 30, 15, 5};
    size_t ks[] = {1, 3};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        concentration_f64_fn_t fn = concentration_f64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        double topk_buf[2] = {0};
        concentration_result_t result = {
            .topk_ratios = topk_buf,
            .heavy_tail_index = 0.0,
            .concentration = 0.0,
        };
        fn(data, 6, 1000, ks, 2, &result);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.50, result.topk_ratios[0]);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.95, result.topk_ratios[1]);
        TEST_ASSERT_TRUE(isfinite(result.heavy_tail_index));
        TEST_ASSERT_TRUE(isfinite(result.concentration));
    }
}

void test_concentration_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++)
        TEST_ASSERT_NOT_NULL(concentration_f64_select((simd_level_t)lvl));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_concentration_basic);
    RUN_TEST(test_concentration_all_variants);
    RUN_TEST(test_concentration_select_all_levels);
    return UNITY_END();
}
