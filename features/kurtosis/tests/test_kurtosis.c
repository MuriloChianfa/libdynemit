#include "unity.h"
#include <dynemit/kurtosis.h>

void setUp(void) {}
void tearDown(void) {}

void test_kurtosis_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, kurtosis_f64(NULL, 0));
}

void test_kurtosis_three_elements(void)
{
    double d[] = {1.0, 2.0, 3.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, kurtosis_f64(d, 3));
}

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

static void run_variant_sizes(kurtosis_f64_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 4, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n < 4) {
            double d[] = {1.0, 2.0, 3.0};
            TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, fn(d, n));
            continue;
        }
        double d[256];
        for (size_t i = 0; i < n; i++) d[i] = (double)(i + 1);
        TEST_ASSERT_TRUE(fn(d, n) <= 0.0);
    }
}

void test_kurtosis_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        kurtosis_f64_fn_t fn = kurtosis_f64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_variant_sizes(fn);
    }
}

void test_kurtosis_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++)
        TEST_ASSERT_NOT_NULL(kurtosis_f64_select(DYNEMIT_SIMD_LEVELS[i]));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_kurtosis_empty);
    RUN_TEST(test_kurtosis_three_elements);
    RUN_TEST(test_kurtosis_uniform);
    RUN_TEST(test_kurtosis_peaked);

    RUN_TEST(test_kurtosis_all_variants);
    RUN_TEST(test_kurtosis_select_all_levels);

    return UNITY_END();
}
