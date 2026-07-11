/* SPDX-License-Identifier: BSL-1.0 */
/*
 * Linker --wrap fault injection for allocation failure tests.
 *
 * Usage (GNU ld):
 *   target_sources(test_foo PRIVATE ${PROJECT_SOURCE_DIR}/tests/fault_alloc.c)
 *   target_link_options(test_foo PRIVATE
 *       -Wl,--wrap=aligned_alloc -Wl,--wrap=malloc -Wl,--wrap=calloc
 *       -Wl,--wrap=realloc)
 *
 * Call fault_alloc_fail_next_*() immediately before the API under test.
 * Exactly one subsequent call to that allocator fails with NULL, then
 * wrapping transparently forwards to libc.
 */
#ifndef FAULT_ALLOC_H
#define FAULT_ALLOC_H

void fault_alloc_fail_next_aligned_alloc(void);
void fault_alloc_fail_next_malloc(void);
void fault_alloc_fail_next_calloc(void);
void fault_alloc_fail_next_realloc(void);

/* Fail exactly the Nth upcoming call (1 = first). Resets after one failure. */
void fault_alloc_fail_nth_malloc(unsigned nth);
void fault_alloc_fail_nth_aligned_alloc(unsigned nth);
void fault_alloc_fail_nth_calloc(unsigned nth);
void fault_alloc_fail_nth_realloc(unsigned nth);

/* Clear all pending fail-next / fail-nth state. Call from tearDown. */
void fault_alloc_reset(void);

#endif /* FAULT_ALLOC_H */
