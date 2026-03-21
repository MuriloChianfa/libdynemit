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

void test_concentration_single_element(void)
{
    uint64_t data[] = {100};
    size_t ks[] = {1};
    double topk_buf[1] = {0};
    concentration_result_t result = {
        .topk_ratios = topk_buf,
        .heavy_tail_index = 0.0,
        .concentration = 0.0,
    };

    concentration_f64(data, 1, 100, ks, 1, &result);

    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, result.topk_ratios[0]);
    TEST_ASSERT_TRUE(isfinite(result.concentration));
}

void test_concentration_large(void)
{
    uint64_t data[256];
    uint64_t total = 0;
    for (int i = 0; i < 256; i++) {
        data[i] = (uint64_t)(256 - i);
        total += data[i];
    }
    size_t ks[] = {1, 10, 50};
    double topk_buf[3] = {0};
    concentration_result_t result = {
        .topk_ratios = topk_buf,
        .heavy_tail_index = 0.0,
        .concentration = 0.0,
    };

    concentration_f64(data, 256, total, ks, 3, &result);

    TEST_ASSERT_TRUE(result.topk_ratios[0] > 0.0);
    TEST_ASSERT_TRUE(result.topk_ratios[1] > result.topk_ratios[0]);
    TEST_ASSERT_TRUE(result.topk_ratios[2] > result.topk_ratios[1]);
    TEST_ASSERT_TRUE(isfinite(result.heavy_tail_index));
    TEST_ASSERT_TRUE(isfinite(result.concentration));
}

void test_concentration_all_variants(void)
{
    uint64_t data[64];
    uint64_t total = 0;
    for (int i = 0; i < 64; i++) {
        data[i] = (uint64_t)(64 - i);
        total += data[i];
    }
    size_t ks[] = {1, 5, 10};
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        concentration_f64_fn_t fn = concentration_f64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        double topk_buf[3] = {0};
        concentration_result_t result = {
            .topk_ratios = topk_buf,
            .heavy_tail_index = 0.0,
            .concentration = 0.0,
        };
        fn(data, 64, total, ks, 3, &result);
        TEST_ASSERT_TRUE(result.topk_ratios[0] > 0.0);
        TEST_ASSERT_TRUE(isfinite(result.heavy_tail_index));
        TEST_ASSERT_TRUE(isfinite(result.concentration));
    }
}

void test_concentration_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++)
        TEST_ASSERT_NOT_NULL(concentration_f64_select(DYNEMIT_SIMD_LEVELS[i]));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_concentration_basic);
    RUN_TEST(test_concentration_single_element);
    RUN_TEST(test_concentration_large);

    RUN_TEST(test_concentration_all_variants);
    RUN_TEST(test_concentration_select_all_levels);

    return UNITY_END();
}
