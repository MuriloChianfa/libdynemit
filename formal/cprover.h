/* SPDX-License-Identifier: BSL-1.0 */
#ifndef FORMAL_CPROVER_H
#define FORMAL_CPROVER_H

#include <stddef.h>

/*
 * CBMC treats an undefined function as a nondet source. clangd needs a
 * real initializer, so the non-CBMC stubs return 0.
 */
#ifndef __CPROVER__
static inline void cprover_assume(_Bool cond) { (void)cond; }
static inline void cprover_assert(_Bool cond, const char *msg)
{
    (void)cond;
    (void)msg;
}
#define __CPROVER_assume(cond) cprover_assume(cond)
#define __CPROVER_assert(cond, msg) cprover_assert(cond, msg)
static inline size_t nondet_size_t(void) { return 0; }
static inline int nondet_int(void) { return 0; }
static inline unsigned char nondet_uchar(void) { return 0; }
#else
size_t nondet_size_t(void);
int nondet_int(void);
unsigned char nondet_uchar(void);
#endif

#endif /* FORMAL_CPROVER_H */
