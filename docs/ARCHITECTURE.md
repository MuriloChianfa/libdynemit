# Dynemit Architecture

This document explains the internal architecture of the dynemit library.

## Overview

Dynemit uses a **hybrid library architecture** that supports both:
1. **All-in-one mode**: Single library containing all features
2. **Modular mode**: Separate libraries for each feature

This is achieved through CMake's **object library** pattern, which allows compiling code once and linking it into multiple targets.

## Directory Structure

```
libdynemit/
├── src/                       # Core library (CPU detection)
│   ├── dynemit.c             # CPU / SIMD level detection
│   └── dynemit_features.c    # Feature name list (all-in-one only)
├── features/                  # One directory per SIMD feature (auto-discovered)
│   ├── max/
│   │   ├── max_u16.c         # Implementations + _select() + ifunc
│   │   ├── max_f64.c         # other type variants in the same feature
│   │   ├── CMakeLists.txt    # object lib + dynemit_max + test/bench helpers
│   │   ├── tests/            # Unity correctness tests
│   │   └── benchmarks/       # bench_max_f64, bench_max_u32, …
│   ├── sum/
│   ├── mean/
│   └── …                     # add, entropy, hll, radixs, …
├── cmake/
│   ├── FeatureExtras.cmake   # dynemit_add_feature_test / _bench helpers
│   └── ListFeatures.cmake    # LIST_FEATURES=ON listing
├── include/dynemit/           # Public headers
├── bench/                     # Shared benchmark utilities (bench_utils.h)
└── tests/                     # Core-only tests (detection, C++ compat, …)
```

## Build System Architecture

### Feature discovery

The root `CMakeLists.txt` Globs `features/*/` and `add_subdirectory`s each one.
Each feature contributes `${FEATURE_NAME}_obj` objects into the all-in-one
libraries; no hand-maintained feature list is required in the root CMake file
for linking.

### Object Libraries

Each feature is built as an **object library** (`*_obj` target):

```cmake
add_library(max_obj OBJECT ${MAX_SOURCES})
```

Object libraries compile source files but don't create an archive. The compiled
objects can then be:
1. Bundled into the all-in-one static/shared libraries
2. Packaged into individual static libraries (`libdynemit_max.a`, …)

### All-in-One Libraries

`libdynemit.a` / `libdynemit.so` combine core + every discovered feature object:

```cmake
add_library(dynemit_static STATIC
    $<TARGET_OBJECTS:dynemit_core_obj>
    ${FEATURE_OBJECTS}          # collected from features/*/
    src/dynemit_features.c
)
```

Benefits:
- Single library to link against (`-ldynemit`)
- All features included
- Runtime feature discovery via `dynemit_features()`

### Individual Libraries

Each feature also creates its own static library:

```cmake
add_library(dynemit_max STATIC $<TARGET_OBJECTS:max_obj>)
```

Benefits:
- Minimal binary size (only link what you need)
- Fine-grained control
- No unused code in final binary

### Core Library

The core library (`libdynemit_core.a`) contains only CPU detection:

```cmake
add_library(dynemit_core STATIC $<TARGET_OBJECTS:dynemit_core_obj>)
```

This is required by all feature libraries and applications.

### Tests and benchmarks

Feature tests and benchmarks are registered through helpers in
`cmake/FeatureExtras.cmake` (`dynemit_add_feature_test`,
`dynemit_add_feature_bench`). Those helpers no-op unless the matching option is
on:

| Option | Debug default | Release default |
|---|---|---|
| `DYNEMIT_BUILD_TESTS` | ON | OFF |
| `DYNEMIT_BUILD_BENCHMARKS` | ON | OFF |

Core tests under `tests/` are also gated by `DYNEMIT_BUILD_TESTS`. Release
builds therefore produce libraries only unless you pass `-D…=ON` explicitly.

## Runtime Dispatch Mechanism

### ifunc Attribute

Each feature uses the `ifunc` (indirect function) attribute, supported by both GCC and Clang on Linux/ELF targets:

```c
// Resolver runs once at program load (see features/max/max_u16.c)
EXPLICIT_RUNTIME_RESOLVER(max_u16_resolver, max_u16_fn_t)
{
    return max_u16_select(detect_simd_level_ts());
}
DYNEMIT_IFUNC_SETUP(max_u16_fn_t, max_u16, max_u16_resolver)

double max_u16(const uint16_t *data, size_t n)
    DYNEMIT_IFUNC_ATTR("max_u16_resolver");
```

**How it works:**
1. At program load time, the dynamic linker calls `max_u16_resolver()`
2. Resolver detects CPU features (via `_select`) and returns optimal implementation pointer
3. All subsequent calls to `max_u16()` go directly to the selected implementation
4. **Zero runtime overhead** after initial resolution

Tests and benchmarks call `max_u16_select(level)` directly to exercise every
reachable SIMD variant without relying on host CPU detection alone.

### CPU Detection

The `detect_simd_level()` function in `src/dynemit.c`:

```c
simd_level_t detect_simd_level(void)
{
    // 1. Check CPU vendor and max CPUID leaf
    cpuid_x86(0, 0, &eax, &ebx, &ecx, &edx);
    
    // 2. Check basic features (SSE2, SSE4.2, AVX)
    cpuid_x86(1, 0, &eax, &ebx, &ecx, &edx);
    
    // 3. Check OS support for YMM/ZMM registers
    if (osxsave)
        xcr0 = xgetbv_x86(0);
    
    // 4. Check extended features (AVX2, AVX-512F)
    cpuid_x86(7, 0, &eax7, &ebx7, &ecx7, &edx7);
    
    // 5. Return highest supported level
    return simd_level;
}
```

**Detection criteria:**
- **SSE2**: CPUID.01H:EDX[26]
- **SSE4.2**: CPUID.01H:ECX[20]
- **AVX**: CPUID.01H:ECX[28] + XCR0[2:1] (OS support)
- **AVX2**: CPUID.07H:EBX[5] + XCR0[2:1]
- **AVX-512F**: CPUID.07H:EBX[16] + XCR0[7:5] (ZMM state)
- **AVX-512 VBMI2**: CPUID.07H:ECX[6] + AVX-512F + XCR0[7:5]

## Feature Registry

### Weak Symbols

The `dynemit_features()` function uses weak symbols to provide different implementations:

**Default (in dynemit.c):**
```c
__attribute__((weak))
const char **
dynemit_features(void)
{
    static const char *features[] = { "core", NULL };
    return features;
}
```

**All-in-one override (in dynemit_features.c):**
```c
const char **
dynemit_features(void)
{
    static const char *features[] = {
        "core",
        /* … all shipped features … */
        nullptr
    };
    return features;
}
```

When linking:
- `libdynemit.a` / `libdynemit.so`: Uses the all-in-one version (includes `dynemit_features.c`) and defines `DYNEMIT_ALL_FEATURES`
- `libdynemit_core.a`: Uses the weak default version (`"core"` only)

## Header Organization

### Umbrella Header

`include/dynemit.h` includes core plus every public feature header:

```c
#include <dynemit/core.h>
#include <dynemit/err.h>
#include <dynemit/max.h>
/* … add, stats, entropy, hll, radixs, … */
```

Consumers can also include a single feature header (e.g. `#include <dynemit/max.h>`)
and link only `libdynemit_core` + `libdynemit_max`.

The all-in-one targets define `DYNEMIT_ALL_FEATURES` so `dynemit_features()` is
meaningful when linking `-ldynemit`:

```cmake
target_compile_definitions(dynemit_static PUBLIC DYNEMIT_ALL_FEATURES)
target_compile_definitions(dynemit_shared PUBLIC DYNEMIT_ALL_FEATURES)
```

### Feature Headers

Each feature has a minimal header in `include/dynemit/`:

```c
#ifndef DYNEMIT_MAX_H
#define DYNEMIT_MAX_H

#include <stddef.h>
#include <stdint.h>
#include <dynemit/core.h>

typedef double (*max_u16_fn_t)(const uint16_t *, size_t);

double max_u16(const uint16_t *data, size_t n);
max_u16_fn_t max_u16_select(simd_level_t level);

#endif
```

**Design principles:**
- Self-contained (only depends on standard headers and `core.h`)
- No implementation details leaked
- Can be included independently
- Exposes `_select()` for explicit SIMD-level testing and benchmarking

## SIMD Implementation Pattern

### Standard Structure

Every SIMD feature follows this pattern (illustrated with `max_u16`):

```c
// 1. Include intrinsics + public header
#include <immintrin.h>
#include <dynemit/compiler.h>
#include <dynemit/max.h>

// 2. Scalar fallback
DYNEMIT_TARGET_DEFAULT
DYNEMIT_NO_AUTOVECTORIZE
static double max_u16_scalar(const uint16_t *data, size_t n)
{
DYNEMIT_PRAGMA_NO_VECTORIZE_BEGIN
    /* ... */
}

// 3. SIMD implementations (SSE2, SSE4.2, AVX, AVX2, AVX-512F, …)
__attribute__((target("sse4.2")))
static double max_u16_sse42(const uint16_t *data, size_t n) { /* ... */ }

// 4. Explicit select + ifunc resolver macros
max_u16_fn_t max_u16_select(simd_level_t level) { /* switch on level */ }

EXPLICIT_RUNTIME_RESOLVER(max_u16_resolver, max_u16_fn_t)
{
    return max_u16_select(detect_simd_level_ts());
}
DYNEMIT_IFUNC_SETUP(max_u16_fn_t, max_u16, max_u16_resolver)

// 5. Public entry (ifunc)
double max_u16(const uint16_t *data, size_t n)
    DYNEMIT_IFUNC_ATTR("max_u16_resolver");
```

### Why This Pattern?

1. **Scalar fallback**: Ensures portability to non-x86 platforms
2. **Static implementations**: Prevents symbol conflicts
3. **Target attributes**: Enables specific CPU instructions
4. **Resolver pattern**: Centralizes dispatch logic
5. **ifunc attribute**: Achieves zero-overhead dispatch

## Testing Strategy

### Feature unit tests

Each feature owns Unity tests under `features/<name>/tests/`, registered with
`dynemit_add_feature_test(<name>)`. Typical coverage:

- Correctness vs scalar / expected values across input sizes
- All reachable SIMD levels via `_select()`
- Allocator failure paths where relevant (`FAULT_ALLOC`)

### Core tests

`tests/` covers library-wide behavior (gated by `DYNEMIT_BUILD_TESTS`):

- `test_features.c`: runtime feature enumeration / `DYNEMIT_ALL_FEATURES`
- SIMD detection, resolver macros, memory helpers
- Optional Google Test C++ compatibility tests

### Benchmarks

Feature benchmarks under `features/<name>/benchmarks/` (e.g. `bench_max_f64`,
`bench_max_u32`) share CLI/CSV infrastructure from `bench/bench_utils.h` and are
registered with `dynemit_add_feature_bench(<name> <type>)`. They double as
integration smoke tests when built (`DYNEMIT_BUILD_BENCHMARKS`).

## Performance Considerations

### Cache Efficiency

- Use unaligned loads (e.g. `_mm_loadu_si128` in `max_u16_sse42`) to avoid alignment overhead
- Process data sequentially to maximize cache hits
- Consider prefetching for large datasets

### Loop Structure

```c
size_t i = 0;
__m128i vmax = _mm_setzero_si128();

// Main SIMD loop (8 x u16 per SSE iteration)
for (; i + 8 <= n; i += 8) {
    vmax = _mm_max_epu16(vmax, _mm_loadu_si128((const __m128i *)(data + i)));
}
```

### Compiler Optimization

- Build with `-O3` for maximum performance
- Use `__restrict__` for pointer arguments if applicable
- Let compiler auto-vectorize scalar tail

## Portability

### Non-x86 Platforms

On non-x86 systems:
- CPUID returns 0 for all features
- `detect_simd_level()` returns `SIMD_SCALAR`
- Resolver always selects scalar implementation
- No SIMD intrinsics are used

### Compiler Requirements

**Required:**
- GCC 13+ or Clang 16+ with `ifunc` support (Linux/ELF targets)
- x86-64 target for SIMD optimizations

**Not supported:**
- MSVC (different intrinsics model)
- macOS/non-ELF platforms (no `ifunc` support)

## References

- [GCC Function Attributes](https://gcc.gnu.org/onlinedocs/gcc/Common-Function-Attributes.html)
- [Clang Attributes Reference](https://clang.llvm.org/docs/AttributeReference.html)
- [Intel Intrinsics Guide](https://www.intel.com/content/www/us/en/docs/intrinsics-guide/index.html)
- [CMake Object Libraries](https://cmake.org/cmake/help/latest/command/add_library.html#object-libraries)
- [CPUID Specification](https://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-vol-2a-manual.html)
