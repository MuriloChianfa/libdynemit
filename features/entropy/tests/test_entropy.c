#include "unity.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <dynemit/entropy.h>

void setUp(void) {}
void tearDown(void) {}

void test_entropy_u16_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, entropy_u16(NULL, 0));
}

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

void test_entropy_u32_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, entropy_u32(NULL, 0));
}

void test_entropy_u32_constant(void)
{
    uint32_t d[] = {42, 42, 42, 42};
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, entropy_u32(d, 4));
}

void test_entropy_histogram_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, entropy_histogram(NULL, 0));
}

void test_entropy_histogram_equal(void)
{
    uint64_t c[] = {10, 10};
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, entropy_histogram(c, 2));
}

static void run_u16_variant_sizes(entropy_u16_fn_t fn)
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
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(d, n));
    }
}

static void run_u32_variant_sizes(entropy_u32_fn_t fn)
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
        TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(d, n));
    }
}

static void run_histogram_variant_sizes(entropy_histogram_fn_t fn)
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

void test_entropy_u16_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_u16_fn_t fn = entropy_u16_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u16_variant_sizes(fn);
    }
}

void test_entropy_u32_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_u32_fn_t fn = entropy_u32_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u32_variant_sizes(fn);
    }
}

void test_entropy_histogram_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_histogram_fn_t fn = entropy_histogram_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_histogram_variant_sizes(fn);
    }
}

void test_entropy_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++) {
        TEST_ASSERT_NOT_NULL(entropy_u16_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(entropy_u32_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(entropy_histogram_select(DYNEMIT_SIMD_LEVELS[i]));
    }
}

static double reference_entropy_u16(const uint16_t *data, size_t n)
{
    if (n == 0) return 0.0;
    uint32_t hist[65536] = {0};
    for (size_t i = 0; i < n; i++) hist[data[i]]++;
    double inv_n = 1.0 / (double)n;
    double h = 0.0;
    for (size_t i = 0; i < 65536; i++) {
        if (hist[i] == 0) continue;
        double p = (double)hist[i] * inv_n;
        h -= p * log2(p);
    }
    return h;
}

void test_entropy_u16_precision_all_variants(void)
{
    static const size_t N = 1024;
    uint16_t *d = malloc(N * sizeof(uint16_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++)
        d[i] = (uint16_t)(i % 64);

    double ref = reference_entropy_u16(d, N);
    TEST_ASSERT_TRUE(ref > 0.0);

    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_u16_fn_t fn = entropy_u16_select(DYNEMIT_SIMD_LEVELS[i]);
        double got = fn(d, N);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, ref, got);
    }
    free(d);
}

void test_entropy_u16_dense(void)
{
    static const size_t N = 65536;
    uint16_t *d = malloc(N * sizeof(uint16_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++)
        d[i] = (uint16_t)i;

    double ref = reference_entropy_u16(d, N);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 16.0, ref);

    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_u16_fn_t fn = entropy_u16_select(DYNEMIT_SIMD_LEVELS[i]);
        double got = fn(d, N);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, ref, got);
    }
    free(d);
}

void test_entropy_histogram_precision_all_variants(void)
{
    static const size_t N = 128;
    uint64_t c[128];
    for (size_t i = 0; i < N; i++)
        c[i] = (i + 1) * 3;

    entropy_histogram_fn_t scalar_fn = entropy_histogram_select(SIMD_SCALAR);
    double ref = scalar_fn(c, N);
    TEST_ASSERT_TRUE(ref > 0.0);

    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        entropy_histogram_fn_t fn = entropy_histogram_select(DYNEMIT_SIMD_LEVELS[i]);
        double got = fn(c, N);
        TEST_ASSERT_DOUBLE_WITHIN(1e-6, ref, got);
    }
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_entropy_u16_empty);
    RUN_TEST(test_entropy_u16_constant);
    RUN_TEST(test_entropy_u16_two_values);

    RUN_TEST(test_entropy_u32_empty);
    RUN_TEST(test_entropy_u32_constant);

    RUN_TEST(test_entropy_histogram_empty);
    RUN_TEST(test_entropy_histogram_equal);

    RUN_TEST(test_entropy_u16_all_variants);
    RUN_TEST(test_entropy_u32_all_variants);
    RUN_TEST(test_entropy_histogram_all_variants);

    RUN_TEST(test_entropy_select_all_levels);

    RUN_TEST(test_entropy_u16_precision_all_variants);
    RUN_TEST(test_entropy_u16_dense);
    RUN_TEST(test_entropy_histogram_precision_all_variants);

    return UNITY_END();
}
