#include "unity.h"
#include <stdint.h>
#include <dynemit/entropy.h>

void setUp(void) {}
void tearDown(void) {}

void test_entropy_u16_constant(void)
{
    uint16_t d[] = {5, 5, 5, 5, 5, 5, 5, 5};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, entropy_u16(d, 8));
}

void test_entropy_u16_two_values(void)
{
    uint16_t d[] = {0, 1, 0, 1, 0, 1, 0, 1};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, entropy_u16(d, 8));
}

void test_entropy_u32_constant(void)
{
    uint32_t d[] = {42, 42, 42, 42};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, entropy_u32(d, 4));
}

void test_entropy_histogram_equal(void)
{
    uint64_t c[] = {10, 10};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, entropy_histogram(c, 2));
}

void test_entropy_u16_all_variants(void)
{
    uint16_t d[] = {0, 1, 0, 1, 0, 1, 0, 1};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        entropy_u16_fn_t fn = entropy_u16_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, fn(d, 8));
    }
}

void test_entropy_u32_all_variants(void)
{
    uint32_t d[] = {42, 42, 42, 42};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        entropy_u32_fn_t fn = entropy_u32_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(d, 4));
    }
}

void test_entropy_histogram_all_variants(void)
{
    uint64_t c[] = {10, 10};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        entropy_histogram_fn_t fn = entropy_histogram_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, fn(c, 2));
    }
}

void test_entropy_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++) {
        TEST_ASSERT_NOT_NULL(entropy_u16_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(entropy_u32_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(entropy_histogram_select((simd_level_t)lvl));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_entropy_u16_constant);
    RUN_TEST(test_entropy_u16_two_values);
    RUN_TEST(test_entropy_u32_constant);
    RUN_TEST(test_entropy_histogram_equal);
    RUN_TEST(test_entropy_u16_all_variants);
    RUN_TEST(test_entropy_u32_all_variants);
    RUN_TEST(test_entropy_histogram_all_variants);
    RUN_TEST(test_entropy_select_all_levels);
    return UNITY_END();
}
