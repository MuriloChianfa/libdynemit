#include "unity.h"
#include <stdint.h>
#include <dynemit/simpson.h>

void setUp(void) {}
void tearDown(void) {}

void test_simpson_u16_constant(void)
{
    uint16_t d[] = {3, 3, 3, 3, 3, 3, 3, 3};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, simpson_u16(d, 8));
}

void test_simpson_u32_constant(void)
{
    uint32_t d[] = {7, 7, 7, 7};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, simpson_u32(d, 4));
}

void test_simpson_histogram_equal(void)
{
    uint64_t c[] = {10, 10};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.5, simpson_histogram(c, 2));
}

void test_simpson_u16_all_variants(void)
{
    uint16_t d[] = {3, 3, 3, 3, 3, 3, 3, 3};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        simpson_u16_fn_t fn = simpson_u16_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, fn(d, 8));
    }
}

void test_simpson_u32_all_variants(void)
{
    uint32_t d[] = {7, 7, 7, 7};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        simpson_u32_fn_t fn = simpson_u32_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.0, fn(d, 4));
    }
}

void test_simpson_histogram_all_variants(void)
{
    uint64_t c[] = {10, 10};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        simpson_histogram_fn_t fn = simpson_histogram_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 0.5, fn(c, 2));
    }
}

void test_simpson_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++) {
        TEST_ASSERT_NOT_NULL(simpson_u16_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(simpson_u32_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(simpson_histogram_select((simd_level_t)lvl));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_simpson_u16_constant);
    RUN_TEST(test_simpson_u32_constant);
    RUN_TEST(test_simpson_histogram_equal);
    RUN_TEST(test_simpson_u16_all_variants);
    RUN_TEST(test_simpson_u32_all_variants);
    RUN_TEST(test_simpson_histogram_all_variants);
    RUN_TEST(test_simpson_select_all_levels);
    return UNITY_END();
}
