/* SPDX-License-Identifier: BSL-1.0 */
#include <dynemit/core.h>
#include <stddef.h>
#include <stdatomic.h>

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
    (void)leaf; (void)subleaf;
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

simd_level_t
detect_simd_level(void)
{
#if !(defined(__x86_64__) || defined(__i386__))
    return SIMD_SCALAR; // non-x86
#else
    uint32_t eax, ebx, ecx, edx;
    cpuid_x86(0, 0, &eax, &ebx, &ecx, &edx);
    if (eax == 0)
        return SIMD_SCALAR;

    cpuid_x86(1, 0, &eax, &ebx, &ecx, &edx);
    int sse2    = (edx >> 26) & 1;
    int sse42   = (ecx >> 20) & 1;
    int osxsave = (ecx >> 27) & 1;
    int avx     = (ecx >> 28) & 1;

    uint64_t xcr0 = 0;
    if (osxsave)
        xcr0 = xgetbv_x86(0);

    // Extended features
    uint32_t eax7, ebx7, ecx7, edx7;
    cpuid_x86(7, 0, &eax7, &ebx7, &ecx7, &edx7);
    int avx2    = (ebx7 >> 5) & 1;
    int avx512f = (ebx7 >> 16) & 1;

    int ymm_ok = osxsave && ((xcr0 & 0x6) == 0x6);
    int zmm_ok = osxsave && ((xcr0 & 0xE0) == 0xE0);

    // Prioritize fastest version
    if (avx && avx512f && zmm_ok) return SIMD_AVX512F;
    if (avx && avx2 && ymm_ok) return SIMD_AVX2;
    if (avx && ymm_ok) return SIMD_AVX;
    if (sse42) return SIMD_SSE4_2;
    if (sse2) return SIMD_SSE2;
    return SIMD_SCALAR;
#endif
}

simd_level_t
detect_simd_level_ts(void)
{
    // Use -1 as sentinel for "not yet initialized"
    // Valid simd_level_t values are 0-5 (SIMD_SCALAR to SIMD_AVX512F)
    static _Atomic int cached_level = -1;
    
    int level = atomic_load_explicit(&cached_level, memory_order_acquire);
    
    if (level < 0) {
        // Not yet initialized - detect now
        simd_level_t detected = detect_simd_level();
        
        // Try to cache it (race is okay - all threads will get same result)
        int expected = -1;
        if (atomic_compare_exchange_strong(&cached_level, &expected, (int)detected)) {
            // We won the race
            level = (int)detected;
        } else {
            // Someone else cached it first, use their value
            level = atomic_load_explicit(&cached_level, memory_order_acquire);
        }
    }
    
    return (simd_level_t)level;
}

const char *
simd_level_name(simd_level_t level)
{
    switch (level) {
        case SIMD_AVX512F: return "AVX-512F";
        case SIMD_AVX2:    return "AVX2";
        case SIMD_AVX:     return "AVX";
        case SIMD_SSE4_2:  return "SSE4.2";
        case SIMD_SSE2:    return "SSE2";
        case SIMD_SCALAR:  return "Scalar";
        default:           return "Unknown";
    }
}

// Default implementation (weak symbol, can be overridden)
__attribute__((weak))
const char **
dynemit_features(void)
{
    // Default: just core
    static const char *features[] = {
        "core",
        nullptr
    };
    return features;
}

