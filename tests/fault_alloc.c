/* SPDX-License-Identifier: BSL-1.0 */
#include "fault_alloc.h"

#include <stddef.h>
#include <stdlib.h>

void *__real_aligned_alloc(size_t alignment, size_t size);
void *__real_malloc(size_t size);
void *__real_calloc(size_t nmemb, size_t size);
void *__real_realloc(void *ptr, size_t size);

static int fail_next_aligned_alloc;
static int fail_next_malloc;
static int fail_next_calloc;
static int fail_next_realloc;

static unsigned aligned_alloc_nth_fail;
static unsigned aligned_alloc_call_count;
static unsigned malloc_nth_fail;
static unsigned malloc_call_count;
static unsigned calloc_nth_fail;
static unsigned calloc_call_count;
static unsigned realloc_nth_fail;
static unsigned realloc_call_count;

void
fault_alloc_reset(void)
{
    fail_next_aligned_alloc = 0;
    fail_next_malloc = 0;
    fail_next_calloc = 0;
    fail_next_realloc = 0;
    aligned_alloc_nth_fail = 0;
    aligned_alloc_call_count = 0;
    malloc_nth_fail = 0;
    malloc_call_count = 0;
    calloc_nth_fail = 0;
    calloc_call_count = 0;
    realloc_nth_fail = 0;
    realloc_call_count = 0;
}

void
fault_alloc_fail_next_aligned_alloc(void)
{
    fail_next_aligned_alloc = 1;
}

void
fault_alloc_fail_next_malloc(void)
{
    fail_next_malloc = 1;
}

void
fault_alloc_fail_next_calloc(void)
{
    fail_next_calloc = 1;
}

void
fault_alloc_fail_next_realloc(void)
{
    fail_next_realloc = 1;
}

void
fault_alloc_fail_nth_malloc(unsigned nth)
{
    malloc_nth_fail = nth;
    malloc_call_count = 0;
}

void
fault_alloc_fail_nth_aligned_alloc(unsigned nth)
{
    aligned_alloc_nth_fail = nth;
    aligned_alloc_call_count = 0;
}

void
fault_alloc_fail_nth_calloc(unsigned nth)
{
    calloc_nth_fail = nth;
    calloc_call_count = 0;
}

void
fault_alloc_fail_nth_realloc(unsigned nth)
{
    realloc_nth_fail = nth;
    realloc_call_count = 0;
}

void *
__wrap_aligned_alloc(size_t alignment, size_t size)
{
    if (aligned_alloc_nth_fail != 0) {
        aligned_alloc_call_count++;
        if (aligned_alloc_call_count == aligned_alloc_nth_fail) {
            aligned_alloc_nth_fail = 0;
            return NULL;
        }
    }
    if (fail_next_aligned_alloc) {
        fail_next_aligned_alloc = 0;
        return NULL;
    }
    return __real_aligned_alloc(alignment, size);
}

void *
__wrap_malloc(size_t size)
{
    if (malloc_nth_fail != 0) {
        malloc_call_count++;
        if (malloc_call_count == malloc_nth_fail) {
            malloc_nth_fail = 0;
            return NULL;
        }
    }
    if (fail_next_malloc) {
        fail_next_malloc = 0;
        return NULL;
    }
    return __real_malloc(size);
}

void *
__wrap_calloc(size_t nmemb, size_t size)
{
    if (calloc_nth_fail != 0) {
        calloc_call_count++;
        if (calloc_call_count == calloc_nth_fail) {
            calloc_nth_fail = 0;
            return NULL;
        }
    }
    if (fail_next_calloc) {
        fail_next_calloc = 0;
        return NULL;
    }
    return __real_calloc(nmemb, size);
}

void *
__wrap_realloc(void *ptr, size_t size)
{
    if (realloc_nth_fail != 0) {
        realloc_call_count++;
        if (realloc_call_count == realloc_nth_fail) {
            realloc_nth_fail = 0;
            return NULL;
        }
    }
    if (fail_next_realloc) {
        fail_next_realloc = 0;
        return NULL;
    }
    return __real_realloc(ptr, size);
}
