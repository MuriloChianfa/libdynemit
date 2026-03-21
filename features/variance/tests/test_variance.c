#include "unity.h"
#include <dynemit/variance.h>

void setUp(void) {}
void tearDown(void) {}

void test_variance_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, variance_f64(NULL, 0));
}

void test_variance_single_element(void)
{
    double d[] = {42.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, variance_f64(d, 1));
}

void test_variance_basic(void)
{
    double d[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 2.5, variance_f64(d, 5));
}

void test_variance_constant_array(void)
{
    double d[64];
    for (int i = 0; i < 64; i++) d[i] = 7.0;
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, variance_f64(d, 64));
}

static void run_variant_sizes(variance_f64_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n < 2) {
            double d[] = {1.0};
            TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, fn(d, n));
            continue;
        }
        double d[256];
        for (size_t i = 0; i < n; i++) d[i] = (double)(i + 1);
        double expected = (double)n * (double)(n + 1) / 12.0;
        TEST_ASSERT_DOUBLE_WITHIN(1e-6 * expected + 1e-9, expected, fn(d, n));
    }
}

void test_variance_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        variance_f64_fn_t fn = variance_f64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_variant_sizes(fn);
    }
}

void test_variance_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++)
        TEST_ASSERT_NOT_NULL(variance_f64_select(DYNEMIT_SIMD_LEVELS[i]));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_variance_empty);
    RUN_TEST(test_variance_single_element);
    RUN_TEST(test_variance_basic);
    RUN_TEST(test_variance_constant_array);

    RUN_TEST(test_variance_all_variants);
    RUN_TEST(test_variance_select_all_levels);

    return UNITY_END();
}
