#include "unity.h"
#include <stdint.h>
#include <dynemit/min.h>

void setUp(void) {}
void tearDown(void) {}

void test_min_f64_basic(void)
{
    double d[] = {5.0, 3.0, 8.0, 1.0, 4.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 1.0, min_f64(d, 5));
}

void test_min_f64_single(void)
{
    double d[] = {42.0};
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 42.0, min_f64(d, 1));
}

void test_min_f64_descending(void)
{
    double d[256];
    for (int i = 0; i < 256; i++) d[i] = (double)(256 - i);
    TEST_ASSERT_DOUBLE_WITHIN(1e-12, 1.0, min_f64(d, 256));
}

void test_min_u64_basic(void)
{
    uint64_t d[] = {100, 5, 50, 200};
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 5.0, min_u64(d, 4));
}

void test_min_u32_basic(void)
{
    uint32_t d[] = {10, 3, 7};
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 3.0, min_u32(d, 3));
}

void test_min_u16_basic(void)
{
    uint16_t d[] = {500, 100, 300};
    TEST_ASSERT_DOUBLE_WITHIN(1e-6, 100.0, min_u16(d, 3));
}

void test_min_f64_all_variants(void)
{
    double d[] = {5.0, 3.0, 8.0, 1.0, 4.0};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        min_f64_fn_t fn = min_f64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-12, 1.0, fn(d, 5));
    }
}

void test_min_u64_all_variants(void)
{
    uint64_t d[] = {100, 5, 50, 200};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        min_u64_fn_t fn = min_u64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, 5.0, fn(d, 4));
    }
}

void test_min_u32_all_variants(void)
{
    uint32_t d[] = {10, 3, 7};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        min_u32_fn_t fn = min_u32_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, 3.0, fn(d, 3));
    }
}

void test_min_u16_all_variants(void)
{
    uint16_t d[] = {500, 100, 300};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        min_u16_fn_t fn = min_u16_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, 100.0, fn(d, 3));
    }
}

void test_min_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++) {
        TEST_ASSERT_NOT_NULL(min_f64_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(min_u64_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(min_u32_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(min_u16_select((simd_level_t)lvl));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_min_f64_basic);
    RUN_TEST(test_min_f64_single);
    RUN_TEST(test_min_f64_descending);
    RUN_TEST(test_min_u64_basic);
    RUN_TEST(test_min_u32_basic);
    RUN_TEST(test_min_u16_basic);
    RUN_TEST(test_min_f64_all_variants);
    RUN_TEST(test_min_u64_all_variants);
    RUN_TEST(test_min_u32_all_variants);
    RUN_TEST(test_min_u16_all_variants);
    RUN_TEST(test_min_select_all_levels);
    return UNITY_END();
}
