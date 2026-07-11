#include "unity.h"
#include <stdint.h>
#include <dynemit/gini.h>

void setUp(void) {}
void tearDown(void) {}

void test_gini_f64_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, gini_f64(NULL, 0));
}

void test_gini_f64_single(void)
{
    double d[] = {5.0};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, gini_f64(d, 1));
}

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

void test_gini_f64_all_zero(void)
{
    double d[] = {0.0, 0.0, 0.0, 0.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, gini_f64(d, 4));
}

void test_gini_u64_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, gini_u64(NULL, 0));
}

void test_gini_u64_single(void)
{
    uint64_t d[] = {10};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, gini_u64(d, 1));
}

void test_gini_u64_equal(void)
{
    uint64_t d[] = {10, 10, 10, 10};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, gini_u64(d, 4));
}

void test_gini_u64_all_zero(void)
{
    uint64_t d[] = {0, 0, 0, 0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, gini_u64(d, 4));
}

static void run_f64_variant_sizes(gini_f64_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(NULL, 0));
            continue;
        }
        double d[256];
        for (size_t i = 0; i < n; i++) d[i] = 5.0;
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, fn(d, n));
    }
}

static void run_u64_variant_sizes(gini_u64_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(NULL, 0));
            continue;
        }
        uint64_t d[256];
        for (size_t i = 0; i < n; i++) d[i] = 10;
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, fn(d, n));
    }
}

void test_gini_f64_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        gini_f64_fn_t fn = gini_f64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_f64_variant_sizes(fn);
    }
}

void test_gini_u64_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        gini_u64_fn_t fn = gini_u64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u64_variant_sizes(fn);
    }
}

void test_gini_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++) {
        TEST_ASSERT_NOT_NULL(gini_f64_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(gini_u64_select(DYNEMIT_SIMD_LEVELS[i]));
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_gini_f64_empty);
    RUN_TEST(test_gini_f64_single);
    RUN_TEST(test_gini_f64_equal);
    RUN_TEST(test_gini_f64_unequal);
    RUN_TEST(test_gini_f64_all_zero);

    RUN_TEST(test_gini_u64_empty);
    RUN_TEST(test_gini_u64_single);
    RUN_TEST(test_gini_u64_equal);
    RUN_TEST(test_gini_u64_all_zero);

    RUN_TEST(test_gini_f64_all_variants);
    RUN_TEST(test_gini_u64_all_variants);

    RUN_TEST(test_gini_select_all_levels);

    return UNITY_END();
}
