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

void test_max_f64_ascending(void)
{
    double d[256];
    for (int i = 0; i < 256; i++) d[i] = (double)(i + 1);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 256.0, max_f64(d, 256));
}

void test_max_u64_basic(void)
{
    uint64_t d[] = {100, 5, 50, 200};
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 200.0, max_u64(d, 4));
}

void test_max_u32_basic(void)
{
    uint32_t d[] = {10, 30, 7};
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 30.0, max_u32(d, 3));
}

void test_max_u16_basic(void)
{
    uint16_t d[] = {500, 100, 300};
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 500.0, max_u16(d, 3));
}

void test_max_f64_all_variants(void)
{
    double d[] = {5.0, 3.0, 8.0, 1.0, 4.0};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        max_f64_fn_t fn = max_f64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-12, 8.0, fn(d, 5));
    }
}

void test_max_u64_all_variants(void)
{
    uint64_t d[] = {100, 5, 50, 200};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        max_u64_fn_t fn = max_u64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, 200.0, fn(d, 4));
    }
}

void test_max_u32_all_variants(void)
{
    uint32_t d[] = {10, 30, 7};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        max_u32_fn_t fn = max_u32_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, 30.0, fn(d, 3));
    }
}

void test_max_u16_all_variants(void)
{
    uint16_t d[] = {500, 100, 300};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        max_u16_fn_t fn = max_u16_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, 500.0, fn(d, 3));
    }
}

void test_max_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++) {
        TEST_ASSERT_NOT_NULL(max_f64_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(max_u64_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(max_u32_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(max_u16_select((simd_level_t)lvl));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_max_f64_basic);
    RUN_TEST(test_max_f64_single);
    RUN_TEST(test_max_f64_ascending);
    RUN_TEST(test_max_u64_basic);
    RUN_TEST(test_max_u32_basic);
    RUN_TEST(test_max_u16_basic);
    RUN_TEST(test_max_f64_all_variants);
    RUN_TEST(test_max_u64_all_variants);
    RUN_TEST(test_max_u32_all_variants);
    RUN_TEST(test_max_u16_all_variants);
    RUN_TEST(test_max_select_all_levels);
    return UNITY_END();
}
