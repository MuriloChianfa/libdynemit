#include "unity.h"
#include "fault_alloc.h"
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>
#include <dynemit/entropy.h>

void setUp(void) {}
void tearDown(void)
{
    fault_alloc_reset();
}

static void *
entropy_u16_thread_fn(void *arg)
{
    (void)arg;
    static const size_t N = 128;
    uint16_t d[N];
    for (size_t i = 0; i < N; i++) {
        d[i] = (uint16_t)(i % 32);
    }
    entropy_u16_fn_t fn = entropy_u16_select(SIMD_SCALAR);
    double got = fn(d, N);
    TEST_ASSERT_TRUE(got > 0.0);
    return NULL;
}

static void *
entropy_u16_fail_thread(void *arg)
{
    (void)arg;
    uint16_t d[] = {0, 1, 2, 3};
    fault_alloc_fail_next_aligned_alloc();
    entropy_u16_fn_t fn = entropy_u16_select(SIMD_SCALAR);
    TEST_ASSERT_DOUBLE_WITHIN(1e-9, 0.0, fn(d, 4));
    return NULL;
}

void test_entropy_u16_threaded_tls(void)
{
    pthread_t threads[4];
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_INT(0, pthread_create(&threads[i], NULL, entropy_u16_thread_fn, NULL));
    }
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_EQUAL_INT(0, pthread_join(threads[i], NULL));
    }
}

void test_entropy_u16_threaded_alloc_fail(void)
{
    pthread_t warm_tid;
    TEST_ASSERT_EQUAL_INT(0, pthread_create(&warm_tid, NULL, entropy_u16_thread_fn, NULL));
    TEST_ASSERT_EQUAL_INT(0, pthread_join(warm_tid, NULL));

    pthread_t fail_tid;
    TEST_ASSERT_EQUAL_INT(0, pthread_create(&fail_tid, NULL, entropy_u16_fail_thread, NULL));
    TEST_ASSERT_EQUAL_INT(0, pthread_join(fail_tid, NULL));
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_entropy_u16_threaded_alloc_fail);
    RUN_TEST(test_entropy_u16_threaded_tls);
    return UNITY_END();
}
