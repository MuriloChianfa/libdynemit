#include "unity.h"
#include <stdint.h>
#include <string.h>
#include <dynemit/histogram.h>

void setUp(void) {}
void tearDown(void) {}

void test_histogram_u16_all_low(void)
{
    uint16_t data[] = {10, 20, 30, 40, 50};
    uint16_t bounds[] = {100, 200};
    uint64_t out[3] = {0};
    histogram_u16(data, 5, bounds, 2, out);
    TEST_ASSERT_EQUAL_UINT64(5, out[0]);
    TEST_ASSERT_EQUAL_UINT64(0, out[1]);
    TEST_ASSERT_EQUAL_UINT64(0, out[2]);
}

void test_histogram_u16_spread(void)
{
    uint16_t data[] = {50, 150, 250};
    uint16_t bounds[] = {100, 200};
    uint64_t out[3] = {0};
    histogram_u16(data, 3, bounds, 2, out);
    TEST_ASSERT_EQUAL_UINT64(1, out[0]);
    TEST_ASSERT_EQUAL_UINT64(1, out[1]);
    TEST_ASSERT_EQUAL_UINT64(1, out[2]);
}

void test_histogram_u64_spread(void)
{
    uint64_t data[] = {5, 15, 25, 35};
    uint64_t bounds[] = {10, 20, 30};
    uint64_t out[4] = {0};
    histogram_u64(data, 4, bounds, 3, out);
    TEST_ASSERT_EQUAL_UINT64(1, out[0]);
    TEST_ASSERT_EQUAL_UINT64(1, out[1]);
    TEST_ASSERT_EQUAL_UINT64(1, out[2]);
    TEST_ASSERT_EQUAL_UINT64(1, out[3]);
}

void test_histogram_u16_all_variants(void)
{
    uint16_t data[] = {50, 150, 250};
    uint16_t bounds[] = {100, 200};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        histogram_u16_fn_t fn = histogram_u16_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        uint64_t out[3] = {0};
        fn(data, 3, bounds, 2, out);
        TEST_ASSERT_EQUAL_UINT64(1, out[0]);
        TEST_ASSERT_EQUAL_UINT64(1, out[1]);
        TEST_ASSERT_EQUAL_UINT64(1, out[2]);
    }
}

void test_histogram_u64_all_variants(void)
{
    uint64_t data[] = {5, 15, 25, 35};
    uint64_t bounds[] = {10, 20, 30};
    simd_level_t max_level = detect_simd_level();
    for (int lvl = SIMD_SCALAR; lvl <= (int)max_level; lvl++) {
        histogram_u64_fn_t fn = histogram_u64_select((simd_level_t)lvl);
        TEST_ASSERT_NOT_NULL(fn);
        uint64_t out[4] = {0};
        fn(data, 4, bounds, 3, out);
        TEST_ASSERT_EQUAL_UINT64(1, out[0]);
        TEST_ASSERT_EQUAL_UINT64(1, out[1]);
        TEST_ASSERT_EQUAL_UINT64(1, out[2]);
        TEST_ASSERT_EQUAL_UINT64(1, out[3]);
    }
}

void test_histogram_select_all_levels(void)
{
    for (int lvl = SIMD_SCALAR; lvl <= SIMD_AVX512F; lvl++) {
        TEST_ASSERT_NOT_NULL(histogram_u16_select((simd_level_t)lvl));
        TEST_ASSERT_NOT_NULL(histogram_u64_select((simd_level_t)lvl));
    }
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_histogram_u16_all_low);
    RUN_TEST(test_histogram_u16_spread);
    RUN_TEST(test_histogram_u64_spread);
    RUN_TEST(test_histogram_u16_all_variants);
    RUN_TEST(test_histogram_u64_all_variants);
    RUN_TEST(test_histogram_select_all_levels);
    return UNITY_END();
}
