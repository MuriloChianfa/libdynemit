#include "unity.h"
#include <stdint.h>
#include <math.h>
#include <dynemit/hill.h>

void setUp(void) {}
void tearDown(void) {}

void test_hill_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, hill_estimator_f64(NULL, 0));
}

void test_hill_too_small(void)
{
    uint64_t d[] = {10, 5, 1};
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, hill_estimator_f64(d, 3));
}

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

static void run_variant_sizes(hill_estimator_f64_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 4, 8, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n < 4) {
            uint64_t d[] = {10, 5, 1};
            TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, fn(d, n));
            continue;
        }
        uint64_t d[256];
        for (size_t i = 0; i < n; i++) d[i] = (uint64_t)(n - i) + 1;
        double r = fn(d, n);
        TEST_ASSERT_TRUE(isfinite(r) || r == 0.0);
    }
}

void test_hill_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        hill_estimator_f64_fn_t fn = hill_estimator_f64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_variant_sizes(fn);
    }
}

void test_hill_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++)
        TEST_ASSERT_NOT_NULL(hill_estimator_f64_select(DYNEMIT_SIMD_LEVELS[i]));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_hill_empty);
    RUN_TEST(test_hill_too_small);
    RUN_TEST(test_hill_heavy_tail);
    RUN_TEST(test_hill_flat);

    RUN_TEST(test_hill_all_variants);
    RUN_TEST(test_hill_select_all_levels);

    return UNITY_END();
}
