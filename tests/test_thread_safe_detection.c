#include "unity.h"
#include <dynemit/core.h>
#include <pthread.h>

#define NUM_THREADS 8
#define ITERATIONS_PER_THREAD 10000

static simd_level_t thread_results[NUM_THREADS];

static void *thread_func(void *arg)
{
    int id = *(int *)arg;
    simd_level_t last = SIMD_SCALAR;
    for (int i = 0; i < ITERATIONS_PER_THREAD; i++)
        last = detect_simd_level_ts();
    thread_results[id] = last;
    return NULL;
}

void setUp(void) {}
void tearDown(void) {}

void test_ts_matches_non_ts(void)
{
    simd_level_t expected = detect_simd_level();
    simd_level_t cached  = detect_simd_level_ts();
    TEST_ASSERT_EQUAL_INT(expected, cached);
}

void test_ts_caching(void)
{
    simd_level_t a = detect_simd_level_ts();
    simd_level_t b = detect_simd_level_ts();
    simd_level_t c = detect_simd_level_ts();
    TEST_ASSERT_EQUAL_INT(a, b);
    TEST_ASSERT_EQUAL_INT(b, c);
}

void test_ts_thread_safety(void)
{
    pthread_t threads[NUM_THREADS];
    int ids[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++) {
        ids[i] = i;
        TEST_ASSERT_EQUAL_INT(0, pthread_create(&threads[i], NULL, thread_func, &ids[i]));
    }
    for (int i = 0; i < NUM_THREADS; i++)
        pthread_join(threads[i], NULL);

    simd_level_t expected = detect_simd_level();
    for (int i = 0; i < NUM_THREADS; i++)
        TEST_ASSERT_EQUAL_INT(expected, thread_results[i]);
}

void test_ts_valid_range(void)
{
    simd_level_t level = detect_simd_level_ts();
    TEST_ASSERT_TRUE(level >= SIMD_SCALAR && level <= SIMD_AVX512_VBMI2);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_ts_matches_non_ts);
    RUN_TEST(test_ts_caching);
    RUN_TEST(test_ts_thread_safety);
    RUN_TEST(test_ts_valid_range);
    return UNITY_END();
}
