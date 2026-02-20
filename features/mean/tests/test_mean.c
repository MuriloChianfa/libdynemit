#include "unity.h"
#include <stdint.h>
#include <dynemit/mean.h>

void setUp(void) {}
void tearDown(void) {}

void test_mean_f64_basic(void)
{
    double d[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 3.0, mean_f64(d, 5));
}

void test_mean_f64_single(void)
{
    double d[] = {42.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 42.0, mean_f64(d, 1));
}

void test_mean_f64_large(void)
{
    double d[256];
    double sum = 0;
    for (int i = 0; i < 256; i++) { d[i] = (double)i; sum += d[i]; }
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, sum / 256.0, mean_f64(d, 256));
}

void test_mean_u64_basic(void)
{
    uint64_t d[] = {2, 4, 6, 8, 10};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 6.0, mean_u64(d, 5));
}

void test_mean_u32_basic(void)
{
    uint32_t d[] = {10, 20, 30};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 20.0, mean_u32(d, 3));
}

void test_mean_u16_basic(void)
{
    uint16_t d[] = {100, 200, 300};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 200.0, mean_u16(d, 3));
}

void test_mean_f64_all_variants(void)
{
    double d[] = {1.0, 2.0, 3.0, 4.0, 5.0};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        mean_f64_fn_t fn = mean_f64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-12, 3.0, fn(d, 5));
    }
}

void test_mean_u64_all_variants(void)
{
    uint64_t d[] = {2, 4, 6, 8, 10};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        mean_u64_fn_t fn = mean_u64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, 6.0, fn(d, 5));
    }
}

void test_mean_u32_all_variants(void)
{
    uint32_t d[] = {10, 20, 30};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        mean_u32_fn_t fn = mean_u32_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, 20.0, fn(d, 3));
    }
}

void test_mean_u16_all_variants(void)
{
    uint16_t d[] = {100, 200, 300};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        mean_u16_fn_t fn = mean_u16_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, 200.0, fn(d, 3));
    }
}

void test_mean_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++) {
        TEST_ASSERT_NOT_NULL(mean_f64_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(mean_u64_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(mean_u32_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(mean_u16_select((simd_level_t)lvl));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_mean_f64_basic);
    RUN_TEST(test_mean_f64_single);
    RUN_TEST(test_mean_f64_large);
    RUN_TEST(test_mean_u64_basic);
    RUN_TEST(test_mean_u32_basic);
    RUN_TEST(test_mean_u16_basic);
    RUN_TEST(test_mean_f64_all_variants);
    RUN_TEST(test_mean_u64_all_variants);
    RUN_TEST(test_mean_u32_all_variants);
    RUN_TEST(test_mean_u16_all_variants);
    RUN_TEST(test_mean_select_all_levels);
    return UNITY_END();
}
