#include "unity.h"
#include "fault_alloc.h"
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <dynemit/hll.h>

void setUp(void) {}
void tearDown(void)
{
    fault_alloc_reset();
}

/* ---- Basic edge cases ---- */

void test_hll_u32_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, hll_u32(NULL, 0));
}

void test_hll_u64_empty(void)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, hll_u64(NULL, 0));
}

void test_hll_u32_single(void)
{
    uint32_t d[] = {42};
    double got = hll_u32(d, 1);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, got);
}

void test_hll_u64_single(void)
{
    uint64_t d[] = {0x123456789abcdefULL};
    double got = hll_u64(d, 1);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, got);
}

void test_hll_u32_constant(void)
{
    uint32_t d[1024];
    for (size_t i = 0; i < 1024; i++) d[i] = 42u;
    double got = hll_u32(d, 1024);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, got);
}

void test_hll_u64_constant(void)
{
    uint64_t d[1024];
    for (size_t i = 0; i < 1024; i++) d[i] = 0xabcdef0123456789ULL;
    double got = hll_u64(d, 1024);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 1.0, got);
}

/* ---- Accuracy (p=10 gives ~3% std error) ---- */

void test_hll_u32_small_exact(void)
{
    /* 100 distinct: LinearCounting active (threshold 900 at p=10), very accurate */
    static const size_t N = 100;
    uint32_t *d = malloc(N * sizeof(uint32_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++) d[i] = (uint32_t)i;

    double got = hll_u32(d, N);
    double err = fabs(got - (double)N) / (double)N;
    TEST_ASSERT_TRUE_MESSAGE(err < 0.05, "relative error should be <5% for small N");
    free(d);
}

void test_hll_u64_small_exact(void)
{
    static const size_t N = 100;
    uint64_t *d = malloc(N * sizeof(uint64_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++) d[i] = (uint64_t)i * 0x9e3779b97f4a7c15ULL;

    double got = hll_u64(d, N);
    double err = fabs(got - (double)N) / (double)N;
    TEST_ASSERT_TRUE_MESSAGE(err < 0.05, "relative error should be <5% for small N");
    free(d);
}

void test_hll_u32_medium(void)
{
    /* 10_000 distinct: past HLL++ threshold, raw HLL estimator region */
    static const size_t N = 10000;
    uint32_t *d = malloc(N * sizeof(uint32_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++) d[i] = (uint32_t)i;

    double got = hll_u32(d, N);
    double err = fabs(got - (double)N) / (double)N;
    TEST_ASSERT_TRUE_MESSAGE(err < 0.10,
        "relative error should be <10% at medium cardinality (p=10 sigma ~3.25%)");
    free(d);
}

void test_hll_u64_medium(void)
{
    static const size_t N = 10000;
    uint64_t *d = malloc(N * sizeof(uint64_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++) d[i] = (uint64_t)i * 0x9e3779b97f4a7c15ULL;

    double got = hll_u64(d, N);
    double err = fabs(got - (double)N) / (double)N;
    TEST_ASSERT_TRUE_MESSAGE(err < 0.10,
        "relative error should be <10% at medium cardinality (p=10 sigma ~3.25%)");
    free(d);
}

void test_hll_u32_large(void)
{
    /* 500_000 distinct: well inside raw estimator range */
    static const size_t N = 500000;
    uint32_t *d = malloc(N * sizeof(uint32_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++) d[i] = (uint32_t)i;

    double got = hll_u32(d, N);
    double err = fabs(got - (double)N) / (double)N;
    TEST_ASSERT_TRUE_MESSAGE(err < 0.10,
        "relative error should be <10% at large cardinality (p=10 sigma ~3.25%)");
    free(d);
}

void test_hll_u64_large(void)
{
    static const size_t N = 500000;
    uint64_t *d = malloc(N * sizeof(uint64_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++) d[i] = (uint64_t)i * 0x9e3779b97f4a7c15ULL;

    double got = hll_u64(d, N);
    double err = fabs(got - (double)N) / (double)N;
    TEST_ASSERT_TRUE_MESSAGE(err < 0.10,
        "relative error should be <10% at large cardinality (p=10 sigma ~3.25%)");
    free(d);
}

/* ---- Cross-variant consistency ---- */

void test_hll_u32_all_variants_agree(void)
{
    static const size_t N = 8192;
    uint32_t *d = malloc(N * sizeof(uint32_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++) d[i] = (uint32_t)(i * 2654435761u);

    double ref = hll_u32_select(SIMD_SCALAR)(d, N);

    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        hll_u32_fn_t fn = hll_u32_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        double got = fn(d, N);
        /* All variants use the same register updates (deterministic);
         * finalizers differ only in the FP summation reduction order, so
         * results agree to within a few ULPs of the horizontal-sum rounding. */
        double rel = fabs(got - ref) / ref;
        TEST_ASSERT_TRUE_MESSAGE(rel < 1e-10,
            "u32 SIMD variants must agree with scalar within 1e-10 relative");
    }
    free(d);
}

void test_hll_u64_all_variants_agree(void)
{
    static const size_t N = 8192;
    uint64_t *d = malloc(N * sizeof(uint64_t));
    TEST_ASSERT_NOT_NULL(d);
    for (size_t i = 0; i < N; i++) d[i] = (uint64_t)i * 0x9e3779b97f4a7c15ULL;

    double ref = hll_u64_select(SIMD_SCALAR)(d, N);

    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        hll_u64_fn_t fn = hll_u64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        double got = fn(d, N);
        double rel = fabs(got - ref) / ref;
        TEST_ASSERT_TRUE_MESSAGE(rel < 1e-10,
            "u64 SIMD variants must agree with scalar within 1e-10 relative");
    }
    free(d);
}

/* ---- Selector completeness ---- */

void test_hll_select_all_levels(void)
{
    for (int i = 0; i < DYNEMIT_N_LEVELS; i++) {
        TEST_ASSERT_NOT_NULL(hll_u32_select(DYNEMIT_SIMD_LEVELS[i]));
        TEST_ASSERT_NOT_NULL(hll_u64_select(DYNEMIT_SIMD_LEVELS[i]));
    }
}

/* ---- Per-variant correctness across a few sizes ---- */

static void
run_u32_variant_sizes(hll_u32_fn_t fn)
{
    /* Edge: n=0 */
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(NULL, 0));

    /* Small + medium */
    static const size_t sizes[] = {1, 2, 5, 17, 64, 256, 1024, 4096};
    for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++) {
        size_t N = sizes[s];
        uint32_t *d = malloc(N * sizeof(uint32_t));
        TEST_ASSERT_NOT_NULL(d);
        for (size_t i = 0; i < N; i++) d[i] = (uint32_t)i;
        double got = fn(d, N);
        /* Result must be positive and within 20% of N (broad for very small) */
        TEST_ASSERT_TRUE(got > 0.0);
        double err = fabs(got - (double)N) / (double)N;
        TEST_ASSERT_TRUE_MESSAGE(err < 0.20, "per-variant sanity must be within 20%");
        free(d);
    }
}

static void
run_u64_variant_sizes(hll_u64_fn_t fn)
{
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(NULL, 0));

    static const size_t sizes[] = {1, 2, 5, 17, 64, 256, 1024, 4096};
    for (size_t s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++) {
        size_t N = sizes[s];
        uint64_t *d = malloc(N * sizeof(uint64_t));
        TEST_ASSERT_NOT_NULL(d);
        for (size_t i = 0; i < N; i++) d[i] = (uint64_t)i * 0x9e3779b97f4a7c15ULL;
        double got = fn(d, N);
        TEST_ASSERT_TRUE(got > 0.0);
        double err = fabs(got - (double)N) / (double)N;
        TEST_ASSERT_TRUE_MESSAGE(err < 0.20, "per-variant sanity must be within 20%");
        free(d);
    }
}

void test_hll_u32_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        hll_u32_fn_t fn = hll_u32_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u32_variant_sizes(fn);
    }
}

void test_hll_u64_all_variants(void)
{
    simd_level_t max_level = detect_simd_level();
    for (int i = 0; i < DYNEMIT_N_LEVELS && DYNEMIT_SIMD_LEVELS[i] <= max_level; i++) {
        hll_u64_fn_t fn = hll_u64_select(DYNEMIT_SIMD_LEVELS[i]);
        TEST_ASSERT_NOT_NULL(fn);
        run_u64_variant_sizes(fn);
    }
}

void test_hll_regs_alloc_fail(void)
{
    uint32_t d[] = {1, 2, 3, 4, 5};
    fault_alloc_fail_next_aligned_alloc();
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, hll_u32_select(SIMD_SCALAR)(d, 5));

    uint64_t d64[] = {1, 2, 3, 4, 5};
    fault_alloc_fail_next_aligned_alloc();
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, hll_u64_select(SIMD_SCALAR)(d64, 5));
}

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_hll_regs_alloc_fail);

    RUN_TEST(test_hll_u32_empty);
    RUN_TEST(test_hll_u64_empty);

    RUN_TEST(test_hll_u32_single);
    RUN_TEST(test_hll_u64_single);

    RUN_TEST(test_hll_u32_constant);
    RUN_TEST(test_hll_u64_constant);

    RUN_TEST(test_hll_u32_small_exact);
    RUN_TEST(test_hll_u64_small_exact);

    RUN_TEST(test_hll_u32_medium);
    RUN_TEST(test_hll_u64_medium);

    RUN_TEST(test_hll_u32_large);
    RUN_TEST(test_hll_u64_large);

    RUN_TEST(test_hll_u32_all_variants_agree);
    RUN_TEST(test_hll_u64_all_variants_agree);

    RUN_TEST(test_hll_select_all_levels);

    RUN_TEST(test_hll_u32_all_variants);
    RUN_TEST(test_hll_u64_all_variants);

    return UNITY_END();
}
