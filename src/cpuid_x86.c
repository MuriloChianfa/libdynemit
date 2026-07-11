/* SPDX-License-Identifier: BSL-1.0 */
#include <dynemit/core.h>
#include <stdint.h>

void
cpuid_x86(uint32_t leaf, uint32_t subleaf,
          uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
#if defined(__x86_64__) || defined(__i386__)
    uint32_t a, b, c, d;
    __asm__ volatile("cpuid"
                     : "=a"(a), "=b"(b), "=c"(c), "=d"(d)
                     : "a"(leaf), "c"(subleaf));
    if (eax) *eax = a;
    if (ebx) *ebx = b;
    if (ecx) *ecx = c;
    if (edx) *edx = d;
#else
    (void)leaf;
    (void)subleaf;
    if (eax) *eax = 0;
    if (ebx) *ebx = 0;
    if (ecx) *ecx = 0;
    if (edx) *edx = 0;
#endif
}

uint64_t
xgetbv_x86(uint32_t xcr)
{
#if defined(__x86_64__) || defined(__i386__)
    uint32_t eax, edx;
    __asm__ volatile (".byte 0x0f, 0x01, 0xd0"
                      : "=a"(eax), "=d"(edx)
                      : "c"(xcr));
    return ((uint64_t)edx << 32) | eax;
#else
    (void)xcr;
    return 0;
#endif
}
