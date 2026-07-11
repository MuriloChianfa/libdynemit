/* SPDX-License-Identifier: BSL-1.0 */
#if DYNEMIT_TS

#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "mem.h"

#include "hll.h"

static pthread_key_t hll_regs_key;
static pthread_once_t hll_regs_once = PTHREAD_ONCE_INIT;
static int hll_regs_pthread_ok;

static _Thread_local uint8_t *hll_regs_fallback;

static void
hll_regs_destructor(void *p)
{
    free(p);
}

static void
hll_regs_init_once(void)
{
    hll_regs_pthread_ok = (pthread_key_create(&hll_regs_key, hll_regs_destructor) == 0);
}

uint8_t *
hll_get_regs(void)
{
    uint8_t *regs = nullptr;

    pthread_once(&hll_regs_once, hll_regs_init_once);
    if (hll_regs_pthread_ok)
        regs = (uint8_t *)pthread_getspecific(hll_regs_key);
    else
        regs = hll_regs_fallback;

    if (__builtin_expect(!regs, 0)) {
        regs = aligned_alloc(64, DYNEMIT_HLL_M);
        if (!regs) return nullptr;
        if (memsets(regs, DYNEMIT_HLL_M, 0, DYNEMIT_HLL_M) != 0) {
            free(regs);
            return nullptr;
        }
        if (hll_regs_pthread_ok) {
            if (pthread_setspecific(hll_regs_key, regs) != 0) {
                free(regs);
                return nullptr;
            }
        } else {
            hll_regs_fallback = regs;
        }
    }
    return regs;
}

#endif /* DYNEMIT_TS */
