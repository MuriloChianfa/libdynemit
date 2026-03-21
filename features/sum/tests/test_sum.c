#include "unity.h"
#include <stdint.h>
#include <math.h>
#include <dynemit/sum.h>

void setUp(void) {}
void tearDown(void) {}

void test_sum_f64_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, sum_f64(NULL, 0));
}

void test_sum_f64_single(void)
{
    double d[] = {42.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 42.0, sum_f64(d, 1));
}

void test_sum_f64_small(void)
{
    double d[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 15.0, sum_f64(d, 5));
}

void test_sum_f64_large(void)
{
    double d[256];
    for (int i = 0; i < 256; i++) d[i] = (double)(i + 1);
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 256.0 * 257.0 / 2.0, sum_f64(d, 256));
}

void test_sum_u64_basic(void)
{
    uint64_t d[] = {1, 2, 3, 4, 5};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 15.0, sum_u64(d, 5));
}

void test_sum_u64_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, sum_u64(NULL, 0));
}

void test_sum_u64_single(void)
{
    uint64_t d[] = {999};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 999.0, sum_u64(d, 1));
}

void test_sum_u32_basic(void)
{
    uint32_t d[] = {10, 20, 30};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 60.0, sum_u32(d, 3));
}

void test_sum_u32_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, sum_u32(NULL, 0));
}

void test_sum_u32_single(void)
{
    uint32_t d[] = {777};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 777.0, sum_u32(d, 1));
}

void test_sum_u16_basic(void)
{
    uint16_t d[] = {100, 200, 300};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 600.0, sum_u16(d, 3));
}

void test_sum_u16_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, sum_u16(NULL, 0));
}

void test_sum_u16_single(void)
{
    uint16_t d[] = {12345};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 12345.0, sum_u16(d, 1));
}

static void run_f64_variant_sizes(sum_f64_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, fn(NULL, 0));
            continue;
        }
        double d[256];
        for (size_t i = 0; i < n; i++) d[i] = (double)(i + 1);
        double expected = (double)n * (double)(n + 1) / 2.0;
        TEST_ASSERT_DOUBLE_WITHIN(1e-6 * fabs(expected) + 1e-12, expected, fn(d, n));
    }
}

static void run_u64_variant_sizes(sum_u64_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(NULL, 0));
            continue;
        }
        uint64_t d[256];
        for (size_t i = 0; i < n; i++) d[i] = i + 1;
        double expected = (double)n * (double)(n + 1) / 2.0;
        TEST_ASSERT_DOUBLE_WITHIN(1e-6 * fabs(expected) + 1e-9, expected, fn(d, n));
    }
}

static void run_u32_variant_sizes(sum_u32_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(NULL, 0));
            continue;
        }
        uint32_t d[256];
        for (size_t i = 0; i < n; i++) d[i] = (uint32_t)(i + 1);
        double expected = (double)n * (double)(n + 1) / 2.0;
        TEST_ASSERT_DOUBLE_WITHIN(1e-6 * fabs(expected) + 1e-9, expected, fn(d, n));
    }
}

static void run_u16_variant_sizes(sum_u16_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(NULL, 0));
            continue;
        }
        uint16_t d[256];
        for (size_t i = 0; i < n; i++) d[i] = (uint16_t)(i + 1);
        double expected = (double)n * (double)(n + 1) / 2.0;
        TEST_ASSERT_DOUBLE_WITHIN(1e-6 * fabs(expected) + 1e-9, expected, fn(d, n));
    }
}

void test_sum_f64_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        sum_f64_fn_t fn = sum_f64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_f64_variant_sizes(fn);
    }
}

void test_sum_u64_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        sum_u64_fn_t fn = sum_u64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u64_variant_sizes(fn);
    }
}

void test_sum_u32_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        sum_u32_fn_t fn = sum_u32_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u32_variant_sizes(fn);
    }
}

void test_sum_u16_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        sum_u16_fn_t fn = sum_u16_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u16_variant_sizes(fn);
    }
}

void test_sum_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++) {
        TEST_ASSERT_NOT_NULL(sum_f64_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(sum_u64_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(sum_u32_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(sum_u16_select(DYNEMIT_SIMD_LEVELS[i]));
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_sum_f64_empty);
    RUN_TEST(test_sum_f64_single);
    RUN_TEST(test_sum_f64_small);
    RUN_TEST(test_sum_f64_large);

    RUN_TEST(test_sum_u64_basic);
    RUN_TEST(test_sum_u64_empty);
    RUN_TEST(test_sum_u64_single);

    RUN_TEST(test_sum_u32_basic);
    RUN_TEST(test_sum_u32_empty);
    RUN_TEST(test_sum_u32_single);

    RUN_TEST(test_sum_u16_basic);
    RUN_TEST(test_sum_u16_empty);
    RUN_TEST(test_sum_u16_single);

    RUN_TEST(test_sum_f64_all_variants);
    RUN_TEST(test_sum_u64_all_variants);
    RUN_TEST(test_sum_u32_all_variants);
    RUN_TEST(test_sum_u16_all_variants);

    RUN_TEST(test_sum_select_all_levels);

    return UNITY_END();
}
