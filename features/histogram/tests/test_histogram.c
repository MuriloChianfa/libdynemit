#include "unity.h"
#include <stdint.h>
#include <string.h>
#include <dynemit/histogram.h>

void setUp(void) {}
void tearDown(void) {}

void test_histogram_u16_empty_data(void)
{
    uint16_t bounds[] = {100, 200};
    uint64_t out[3] = {99, 99, 99};
    histogram_u16(NULL, 0, bounds, 2, out);
    TEST_ASSERT_EQUAL_UINT64(0, out[0]);
    TEST_ASSERT_EQUAL_UINT64(0, out[1]);
    TEST_ASSERT_EQUAL_UINT64(0, out[2]);
}

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

void test_histogram_u16_large(void)
{
    uint16_t data[256];
    for (int i = 0; i < 256; i++) data[i] = (uint16_t)i;
    uint16_t bounds[] = {64, 128, 192};
    uint64_t out[4] = {0};
    histogram_u16(data, 256, bounds, 3, out);
    uint64_t total = out[0] + out[1] + out[2] + out[3];
    TEST_ASSERT_EQUAL_UINT64(256, total);
    TEST_ASSERT_EQUAL_UINT64(64, out[0]);
    TEST_ASSERT_EQUAL_UINT64(64, out[1]);
    TEST_ASSERT_EQUAL_UINT64(64, out[2]);
    TEST_ASSERT_EQUAL_UINT64(64, out[3]);
}

void test_histogram_u64_empty_data(void)
{
    uint64_t bounds[] = {100, 200};
    uint64_t out[3] = {99, 99, 99};
    histogram_u64(NULL, 0, bounds, 2, out);
    TEST_ASSERT_EQUAL_UINT64(0, out[0]);
    TEST_ASSERT_EQUAL_UINT64(0, out[1]);
    TEST_ASSERT_EQUAL_UINT64(0, out[2]);
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

void test_histogram_u64_large(void)
{
    uint64_t data[256];
    for (int i = 0; i < 256; i++) data[i] = (uint64_t)i;
    uint64_t bounds[] = {64, 128, 192};
    uint64_t out[4] = {0};
    histogram_u64(data, 256, bounds, 3, out);
    uint64_t total = out[0] + out[1] + out[2] + out[3];
    TEST_ASSERT_EQUAL_UINT64(256, total);
    TEST_ASSERT_EQUAL_UINT64(64, out[0]);
    TEST_ASSERT_EQUAL_UINT64(64, out[1]);
    TEST_ASSERT_EQUAL_UINT64(64, out[2]);
    TEST_ASSERT_EQUAL_UINT64(64, out[3]);
}

void test_histogram_u16_all_variants(void)
{
    uint16_t data[64];
    for (int i = 0; i < 64; i++) data[i] = (uint16_t)i;
    uint16_t bounds[] = {16, 32, 48};
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        histogram_u16_fn_t fn = histogram_u16_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        uint64_t out[4] = {0};
        fn(data, 64, bounds, 3, out);
        uint64_t total = out[0] + out[1] + out[2] + out[3];
        TEST_ASSERT_EQUAL_UINT64(64, total);
    }
}

void test_histogram_u64_all_variants(void)
{
    uint64_t data[64];
    for (int i = 0; i < 64; i++) data[i] = (uint64_t)i;
    uint64_t bounds[] = {16, 32, 48};
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        histogram_u64_fn_t fn = histogram_u64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        uint64_t out[4] = {0};
        fn(data, 64, bounds, 3, out);
        uint64_t total = out[0] + out[1] + out[2] + out[3];
        TEST_ASSERT_EQUAL_UINT64(64, total);
    }
}

void test_histogram_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++) {
        TEST_ASSERT_NOT_NULL(histogram_u16_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(histogram_u64_select(DYNEMIT_SIMD_LEVELS[i]));
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_histogram_u16_empty_data);
    RUN_TEST(test_histogram_u16_all_low);
    RUN_TEST(test_histogram_u16_spread);
    RUN_TEST(test_histogram_u16_large);

    RUN_TEST(test_histogram_u64_empty_data);
    RUN_TEST(test_histogram_u64_spread);
    RUN_TEST(test_histogram_u64_large);

    RUN_TEST(test_histogram_u16_all_variants);
    RUN_TEST(test_histogram_u64_all_variants);

    RUN_TEST(test_histogram_select_all_levels);

    return UNITY_END();
}
