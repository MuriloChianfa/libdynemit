#include "unity.h"
#include <stdint.h>
#include <dynemit/max.h>

void setUp(void) {}
void tearDown(void) {}

void test_max_f64_basic(void)
{
    double d[] = {5.0, 3.0, 8.0, 1.0, 4.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 8.0, max_f64(d, 5));
}

void test_max_f64_single(void)
{
    double d[] = {42.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 42.0, max_f64(d, 1));
}

void test_max_f64_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 0.0, max_f64(NULL, 0));
}

void test_max_f64_ascending(void)
{
    double d[256];
    for (int i = 0; i < 256; i++) d[i] = (double)(i + 1);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 256.0, max_f64(d, 256));
}

void test_max_f64_descending(void)
{
    double d[256];
    for (int i = 0; i < 256; i++) d[i] = (double)(256 - i);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 256.0, max_f64(d, 256));
}

void test_max_f64_all_same(void)
{
    double d[64];
    for (int i = 0; i < 64; i++) d[i] = 7.0;
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 7.0, max_f64(d, 64));
}

void test_max_u64_basic(void)
{
    uint64_t d[] = {100, 5, 50, 200};
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 200.0, max_u64(d, 4));
}

void test_max_u64_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.0, max_u64(NULL, 0));
}

void test_max_u64_single(void)
{
    uint64_t d[] = {999};
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 999.0, max_u64(d, 1));
}

void test_max_u32_basic(void)
{
    uint32_t d[] = {10, 30, 7};
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 30.0, max_u32(d, 3));
}

void test_max_u32_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.0, max_u32(NULL, 0));
}

void test_max_u32_single(void)
{
    uint32_t d[] = {777};
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 777.0, max_u32(d, 1));
}

void test_max_u16_basic(void)
{
    uint16_t d[] = {500, 100, 300};
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 500.0, max_u16(d, 3));
}

void test_max_u16_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.0, max_u16(NULL, 0));
}

void test_max_u16_single(void)
{
    uint16_t d[] = {12345};
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 12345.0, max_u16(d, 1));
}

static void run_f64_variant_sizes(max_f64_fn_t fn)
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
        TEST_ASSERT_DOUBLE_WITHIN(1e-12, (double)n, fn(d, n));

        for (size_t i = 0; i < n; i++) d[i] = (double)(n - i);
        TEST_ASSERT_DOUBLE_WITHIN(1e-12, (double)n, fn(d, n));
    }
}

static void run_u64_variant_sizes(max_u64_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.0, fn(NULL, 0));
            continue;
        }
        uint64_t d[256];
        for (size_t i = 0; i < n; i++) d[i] = i + 1;
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, (double)n, fn(d, n));

        d[0] = n + 100;
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, (double)(n + 100), fn(d, n));
    }
}

static void run_u32_variant_sizes(max_u32_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.0, fn(NULL, 0));
            continue;
        }
        uint32_t d[256];
        for (size_t i = 0; i < n; i++) d[i] = (uint32_t)(i + 1);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, (double)n, fn(d, n));

        d[0] = (uint32_t)(n + 100);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, (double)(n + 100), fn(d, n));
    }
}

static void run_u16_variant_sizes(max_u16_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 5, 7, 9, 15, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            TEST_ASSERT_DOUBLE_WITHIN(1e-6, 0.0, fn(NULL, 0));
            continue;
        }
        uint16_t d[256];
        for (size_t i = 0; i < n; i++) d[i] = (uint16_t)(i + 1);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, (double)n, fn(d, n));

        d[0] = (uint16_t)(n + 100);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, (double)(n + 100), fn(d, n));
    }
}

void test_max_f64_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        max_f64_fn_t fn = max_f64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_f64_variant_sizes(fn);
    }
}

void test_max_u64_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        max_u64_fn_t fn = max_u64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u64_variant_sizes(fn);
    }
}

void test_max_u32_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        max_u32_fn_t fn = max_u32_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u32_variant_sizes(fn);
    }
}

void test_max_u16_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        max_u16_fn_t fn = max_u16_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u16_variant_sizes(fn);
    }
}

void test_max_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++) {
        TEST_ASSERT_NOT_NULL(max_f64_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(max_u64_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(max_u32_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(max_u16_select(DYNEMIT_SIMD_LEVELS[i]));
    }
}

void test_max_f64_max_in_tail(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        max_f64_fn_t fn = max_f64_select(DYNEMIT_SIMD_LEVELS[i]);
        double d[33];
        for (int j = 0; j < 33; j++) d[j] = 1.0;
        d[32] = 999.0;
        TEST_ASSERT_DOUBLE_WITHIN(1e-12, 999.0, fn(d, 33));
    }
}

void test_max_u32_max_in_tail(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        max_u32_fn_t fn = max_u32_select(DYNEMIT_SIMD_LEVELS[i]);
        uint32_t d[33];
        for (int j = 0; j < 33; j++) d[j] = 1;
        d[32] = 999;
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, 999.0, fn(d, 33));
    }
}

void test_max_u64_max_in_tail(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        max_u64_fn_t fn = max_u64_select(DYNEMIT_SIMD_LEVELS[i]);
        uint64_t d[33];
        for (int j = 0; j < 33; j++) d[j] = 1;
        d[32] = 999;
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, 999.0, fn(d, 33));
    }
}

void test_max_u16_max_in_tail(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        max_u16_fn_t fn = max_u16_select(DYNEMIT_SIMD_LEVELS[i]);
        uint16_t d[33];
        for (int j = 0; j < 33; j++) d[j] = 1;
        d[32] = 999;
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, 999.0, fn(d, 33));
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_max_f64_basic);
    RUN_TEST(test_max_f64_single);
    RUN_TEST(test_max_f64_empty);
    RUN_TEST(test_max_f64_ascending);
    RUN_TEST(test_max_f64_descending);
    RUN_TEST(test_max_f64_all_same);

    RUN_TEST(test_max_u64_basic);
    RUN_TEST(test_max_u64_empty);
    RUN_TEST(test_max_u64_single);

    RUN_TEST(test_max_u32_basic);
    RUN_TEST(test_max_u32_empty);
    RUN_TEST(test_max_u32_single);

    RUN_TEST(test_max_u16_basic);
    RUN_TEST(test_max_u16_empty);
    RUN_TEST(test_max_u16_single);

    RUN_TEST(test_max_f64_all_variants);
    RUN_TEST(test_max_u64_all_variants);
    RUN_TEST(test_max_u32_all_variants);
    RUN_TEST(test_max_u16_all_variants);

    RUN_TEST(test_max_select_all_levels);

    RUN_TEST(test_max_f64_max_in_tail);
    RUN_TEST(test_max_u32_max_in_tail);
    RUN_TEST(test_max_u64_max_in_tail);
    RUN_TEST(test_max_u16_max_in_tail);

    return UNITY_END();
}
