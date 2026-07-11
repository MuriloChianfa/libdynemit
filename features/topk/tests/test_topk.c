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

void test_topk_ratios_single(void)
{
    uint64_t data[] = {100};
    size_t ks[] = {1};
    double ratios[1] = {0};
    topk_ratios_f64(data, 1, 100, ks, 1, ratios);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, ratios[0]);
}

void test_topk_ratios_empty(void)
{
    size_t ks[] = {1, 3};
    double ratios[2] = {0.42, 0.42};
    topk_ratios_f64(NULL, 0, 0, ks, 2, ratios);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, ratios[0]);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, ratios[1]);
}

void test_topk_ratios_large(void)
{
    uint64_t data[256];
    uint64_t total = 0;
    for (int i = 0; i < 256; i++) {
        data[i] = (uint64_t)(256 - i);
        total += data[i];
    }
    size_t ks[] = {1, 10, 50, 100};
    double ratios[4] = {0};
    topk_ratios_f64(data, 256, total, ks, 4, ratios);
    for (int i = 0; i < 4; i++)
        TEST_ASSERT_TRUE(ratios[i] > 0.0 && ratios[i] <= 1.0);
    for (int i = 1; i < 4; i++)
        TEST_ASSERT_TRUE(ratios[i] >= ratios[i - 1]);
}

void test_topk_ratios_all_variants(void)
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
        topk_ratios_f64_fn_t fn = topk_ratios_f64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        double ratios[3] = {0};
        fn(data, 64, total, ks, 3, ratios);
        for (int j = 0; j < 3; j++)
            TEST_ASSERT_TRUE(ratios[j] > 0.0 && ratios[j] <= 1.0);
    }
}

void test_topk_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++)
        TEST_ASSERT_NOT_NULL(topk_ratios_f64_select(DYNEMIT_SIMD_LEVELS[i]));
}

void test_topk_ratios_k_gt_n(void)
{
    uint64_t data[] = {50, 30, 15, 5};
    size_t ks[] = {10, 100};
    double ratios[2] = {0};

#if defined(__x86_64__) || defined(__i386__)
    if (detect_simd_level() >= SIMD_AVX512F) {
        topk_ratios_f64_fn_t fn = topk_ratios_f64_select(SIMD_AVX512F);
        fn(data, 4, 100, ks, 2, ratios);
    } else {
        topk_ratios_f64(data, 4, 100, ks, 2, ratios);
    }
#else
    topk_ratios_f64(data, 4, 100, ks, 2, ratios);
#endif
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, ratios[0]);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, ratios[1]);
}

void test_topk_ratios_avx512_tail(void)
{
#if defined(__x86_64__) || defined(__i386__)
    if (detect_simd_level() < SIMD_AVX512F) {
        TEST_PASS();
    }
    uint64_t data[20];
    for (int i = 0; i < 20; i++) {
        data[i] = (uint64_t)(20 - i);
    }
    uint64_t total = 0;
    for (int i = 0; i < 20; i++) {
        total += data[i];
    }
    size_t ks[] = {13};
    double ratios[1] = {0};
    topk_ratios_f64_fn_t fn = topk_ratios_f64_select(SIMD_AVX512F);
    fn(data, 20, total, ks, 1, ratios);
    TEST_ASSERT_TRUE(ratios[0] > 0.0 && ratios[0] <= 1.0);
#else
    TEST_PASS();
#endif
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_topk_ratios);
    RUN_TEST(test_topk_ratios_single);
    RUN_TEST(test_topk_ratios_empty);
    RUN_TEST(test_topk_ratios_large);

    RUN_TEST(test_topk_ratios_all_variants);
    RUN_TEST(test_topk_ratios_k_gt_n);
    RUN_TEST(test_topk_ratios_avx512_tail);
    RUN_TEST(test_topk_select_all_levels);

    return UNITY_END();
}
