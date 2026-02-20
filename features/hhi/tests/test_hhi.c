#include "unity.h"
#include <stdint.h>
#include <dynemit/hhi.h>

void setUp(void) {}
void tearDown(void) {}

void test_hhi_u16_constant(void)
{
    uint16_t d[] = {5, 5, 5, 5, 5, 5, 5, 5};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, hhi_u16(d, 8));
}

void test_hhi_u32_constant(void)
{
    uint32_t d[] = {9, 9, 9, 9};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, hhi_u32(d, 4));
}

void test_hhi_histogram_concentrated(void)
{
    uint64_t c[] = {100, 0};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, hhi_histogram(c, 2));
}

void test_hhi_u16_all_variants(void)
{
    uint16_t d[] = {5, 5, 5, 5, 5, 5, 5, 5};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        hhi_u16_fn_t fn = hhi_u16_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, fn(d, 8));
    }
}

void test_hhi_u32_all_variants(void)
{
    uint32_t d[] = {9, 9, 9, 9};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        hhi_u32_fn_t fn = hhi_u32_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, fn(d, 4));
    }
}

void test_hhi_histogram_all_variants(void)
{
    uint64_t c[] = {100, 0};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        hhi_histogram_fn_t fn = hhi_histogram_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, fn(c, 2));
    }
}

void test_hhi_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++) {
        TEST_ASSERT_NOT_NULL(hhi_u16_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(hhi_u32_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(hhi_histogram_select((simd_level_t)lvl));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_hhi_u16_constant);
    RUN_TEST(test_hhi_u32_constant);
    RUN_TEST(test_hhi_histogram_concentrated);
    RUN_TEST(test_hhi_u16_all_variants);
    RUN_TEST(test_hhi_u32_all_variants);
    RUN_TEST(test_hhi_histogram_all_variants);
    RUN_TEST(test_hhi_select_all_levels);
    return UNITY_END();
}
