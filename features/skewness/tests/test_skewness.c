#include "unity.h"
#include <dynemit/skewness.h>

void setUp(void) {}
void tearDown(void) {}

void test_skewness_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, skewness_f64(NULL, 0));
}

void test_skewness_two_elements(void)
{
    double d[] = {1.0, 2.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, skewness_f64(d, 2));
}

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

static void run_variant_sizes(skewness_f64_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n < 3) {
            double d[] = {1.0, 2.0};
            TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, fn(d, n));
            continue;
        }
        double d[256];
        for (size_t i = 0; i < n; i++) d[i] = (double)(i + 1);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, fn(d, n));
    }
}

void test_skewness_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        skewness_f64_fn_t fn = skewness_f64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_variant_sizes(fn);
    }
}

void test_skewness_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++)
        TEST_ASSERT_NOT_NULL(skewness_f64_select(DYNEMIT_SIMD_LEVELS[i]));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_skewness_empty);
    RUN_TEST(test_skewness_two_elements);
    RUN_TEST(test_skewness_symmetric);
    RUN_TEST(test_skewness_right_skewed);

    RUN_TEST(test_skewness_all_variants);
    RUN_TEST(test_skewness_select_all_levels);

    return UNITY_END();
}
