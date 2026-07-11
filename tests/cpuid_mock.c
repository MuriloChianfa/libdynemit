/* SPDX-License-Identifier: BSL-1.0 */
#include "cpuid_mock.h"

#include <dynemit/core.h>

void __real_cpuid_x86(uint32_t leaf, uint32_t subleaf,
                      uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);
uint64_t __real_xgetbv_x86(uint32_t xcr);

static cpuid_mock_mode_t mock_mode = CPUID_MOCK_NONE;

void
cpuid_mock_reset(void)
{
    mock_mode = CPUID_MOCK_NONE;
}

void
cpuid_mock_set(cpuid_mock_mode_t mode)
{
    mock_mode = mode;
}

void
__wrap_cpuid_x86(uint32_t leaf, uint32_t subleaf,
                 uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    if (mock_mode == CPUID_MOCK_EAX0_ZERO && leaf == 0) {
        if (eax) *eax = 0;
        if (ebx) *ebx = 0;
        if (ecx) *ecx = 0;
        if (edx) *edx = 0;
        return;
    }

    __real_cpuid_x86(leaf, subleaf, eax, ebx, ecx, edx);

    if (mock_mode == CPUID_MOCK_AVX512_NO_ZMM) {
        if (leaf == 1) {
            if (edx) *edx |= (1u << 26);
            if (ecx) {
                *ecx |= (1u << 27) | (1u << 28);
            }
        }
        if (leaf == 7 && subleaf == 0) {
            if (ebx) *ebx |= (1u << 5) | (1u << 16);
        }
    }
}

uint64_t
__wrap_xgetbv_x86(uint32_t xcr)
{
    if (mock_mode == CPUID_MOCK_AVX512_NO_ZMM && xcr == 0) {
        return 0x6;
    }
    return __real_xgetbv_x86(xcr);
}
