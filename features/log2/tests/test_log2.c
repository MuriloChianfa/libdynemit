#include "unity.h"
#include <math.h>
#include <stdlib.h>
#include <dynemit/log2.h>

void setUp(void) {}
void tearDown(void) {}

void test_log2_f64_empty(void)
{
    double in = 1.0, out = -999.0;
    log2_f64(&in, &out, 0);
    TEST_ASSERT_EQUAL_DOUBLE(-999.0, out);
}

void test_log2_f64_known_values(void)
{
    double in[]  = {1.0, 2.0, 4.0, 8.0, 0.5, 0.25};
    double out[6];
    double ref[] = {0.0, 1.0, 2.0, 3.0, -1.0, -2.0};

    log2_f64(in, out, 6);

    for (int i = 0; i < 6; i++)
        TEST_ASSERT_DOUBLE_WITHIN(1e-10, ref[i], out[i]);
}

static void run_variant_known_values(log2_f64_fn_t fn)
{
    double in[]  = {1.0, 2.0, 4.0, 8.0, 0.5, 0.25};
    double out[6];
    double ref[] = {0.0, 1.0, 2.0, 3.0, -1.0, -2.0};

    fn(in, out, 6);

    for (int i = 0; i < 6; i++)
        TEST_ASSERT_DOUBLE_WITHIN(1e-10, ref[i], out[i]);
}

void test_log2_f64_all_variants_known(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        log2_f64_fn_t fn = log2_f64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_variant_known_values(fn);
    }
}

static void run_variant_sizes(log2_f64_fn_t fn)
{
    static const size_t sizes[] = {0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 33, 64, 100, 256};
    for (int s = 0; s < (int)(sizeof(sizes) / sizeof(sizes[0])); s++) {
        size_t n = sizes[s];
        if (n == 0) {
            double in = 1.0, out = -999.0;
            fn(&in, &out, 0);
            TEST_ASSERT_EQUAL_DOUBLE(-999.0, out);
            continue;
        }
        double *in  = aligned_alloc(64, n * sizeof(double));
        double *out = aligned_alloc(64, n * sizeof(double));
        TEST_ASSERT_NOT_NULL(in);
        TEST_ASSERT_NOT_NULL(out);

        for (size_t i = 0; i < n; i++)
            in[i] = (double)(i + 1);

        fn(in, out, n);

        for (size_t i = 0; i < n; i++)
            TEST_ASSERT_DOUBLE_WITHIN(1e-10, log2((double)(i + 1)), out[i]);

        free(in);
        free(out);
    }
}

void test_log2_f64_all_variants_sizes(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        log2_f64_fn_t fn = log2_f64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_variant_sizes(fn);
    }
}

void test_log2_f64_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++)
        TEST_ASSERT_NOT_NULL(log2_f64_select(DYNEMIT_SIMD_LEVELS[i]));
}

void test_log2_f64_precision_all_variants(void)
{
    static const size_t N = 1024;
    double *in  = aligned_alloc(64, N * sizeof(double));
    double *out = aligned_alloc(64, N * sizeof(double));
    double *ref = aligned_alloc(64, N * sizeof(double));
    TEST_ASSERT_NOT_NULL(in);
    TEST_ASSERT_NOT_NULL(out);
    TEST_ASSERT_NOT_NULL(ref);

    for (size_t i = 0; i < N; i++) {
        in[i] = 0.001 + (double)i * 0.1;
        ref[i] = log2(in[i]);
    }

    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        log2_f64_fn_t fn = log2_f64_select(DYNEMIT_SIMD_LEVELS[i]);
        fn(in, out, N);
        for (size_t j = 0; j < N; j++)
            TEST_ASSERT_DOUBLE_WITHIN(1e-10, ref[j], out[j]);
    }

    free(in);
    free(out);
    free(ref);
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_log2_f64_empty);
    RUN_TEST(test_log2_f64_known_values);
    RUN_TEST(test_log2_f64_all_variants_known);
    RUN_TEST(test_log2_f64_all_variants_sizes);
    RUN_TEST(test_log2_f64_select_all_levels);
    RUN_TEST(test_log2_f64_precision_all_variants);

    return UNITY_END();
}
