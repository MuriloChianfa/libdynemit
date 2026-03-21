#include "unity.h"
#include <stdint.h>
#include <dynemit/hhi.h>

void setUp(void) {}
void tearDown(void) {}

void test_hhi_u16_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, hhi_u16(NULL, 0));
}

void test_hhi_u16_constant(void)
{
    uint16_t d[] = {5, 5, 5, 5, 5, 5, 5, 5};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, hhi_u16(d, 8));
}

void test_hhi_u32_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, hhi_u32(NULL, 0));
}

void test_hhi_u32_constant(void)
{
    uint32_t d[] = {9, 9, 9, 9};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, hhi_u32(d, 4));
}

void test_hhi_histogram_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, hhi_histogram(NULL, 0));
}

void test_hhi_histogram_concentrated(void)
{
    uint64_t c[] = {100, 0};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, hhi_histogram(c, 2));
}

static void run_u16_variant_sizes(hhi_u16_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(NULL, 0));
            continue;
        }
        uint16_t d[256];
        for (size_t i = 0; i < n; i++) d[i] = 42;
        double result = fn(d, n);
        TEST_ASSERT_TRUE(result >= 0.0);
    }
}

static void run_u32_variant_sizes(hhi_u32_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(NULL, 0));
            continue;
        }
        uint32_t d[256];
        for (size_t i = 0; i < n; i++) d[i] = 42;
        double result = fn(d, n);
        TEST_ASSERT_TRUE(result >= 0.0);
    }
}

static void run_histogram_variant_sizes(hhi_histogram_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(NULL, 0));
            continue;
        }
        uint64_t c[256];
        for (size_t i = 0; i < n; i++) c[i] = 10;
        double result = fn(c, n);
        TEST_ASSERT_TRUE(result >= 0.0);
    }
}

void test_hhi_u16_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        hhi_u16_fn_t fn = hhi_u16_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u16_variant_sizes(fn);
    }
}

void test_hhi_u32_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        hhi_u32_fn_t fn = hhi_u32_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u32_variant_sizes(fn);
    }
}

void test_hhi_histogram_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        hhi_histogram_fn_t fn = hhi_histogram_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_histogram_variant_sizes(fn);
    }
}

void test_hhi_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++) {
        TEST_ASSERT_NOT_NULL(hhi_u16_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(hhi_u32_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(hhi_histogram_select(DYNEMIT_SIMD_LEVELS[i]));
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_hhi_u16_empty);
    RUN_TEST(test_hhi_u16_constant);

    RUN_TEST(test_hhi_u32_empty);
    RUN_TEST(test_hhi_u32_constant);

    RUN_TEST(test_hhi_histogram_empty);
    RUN_TEST(test_hhi_histogram_concentrated);

    RUN_TEST(test_hhi_u16_all_variants);
    RUN_TEST(test_hhi_u32_all_variants);
    RUN_TEST(test_hhi_histogram_all_variants);

    RUN_TEST(test_hhi_select_all_levels);

    return UNITY_END();
}
