/* SPDX-License-Identifier: BSL-1.0 */
#ifndef CPUID_MOCK_H
#define CPUID_MOCK_H

#include <stdint.h>

typedef enum {
    CPUID_MOCK_NONE = 0,
    CPUID_MOCK_EAX0_ZERO,
    CPUID_MOCK_AVX512_NO_ZMM,
} cpuid_mock_mode_t;

void cpuid_mock_reset(void);
void cpuid_mock_set(cpuid_mock_mode_t mode);

#endif /* CPUID_MOCK_H */
