---
name: Bug Report
about: Report a bug or unexpected behavior in libdynemit
title: "[BUG] "
labels: bug
assignees: ''

---

## Bug Description

<!-- A clear and concise description of what the bug is -->

## Environment

**Operating System:**
<!-- e.g., Ubuntu 24.04, Debian 12, Fedora 40, Arch Linux -->

**Compiler:**
<!-- e.g., GCC 13.2.0, Clang 16.0.6 (run `gcc --version` or `clang --version`) -->

**CMake Version:**
<!-- e.g., 3.25.1 (run `cmake --version`) -->

**libdynemit Version:**
<!-- e.g., 1.0.0, or commit hash if building from source -->

**CPU Architecture:**
<!-- e.g., x86_64 AMD Ryzen 9 9950X, Intel Core i7-13700K (run `lscpu | grep 'Model name'`) -->

**SIMD Support:**
<!-- Run: `lscpu | grep -E 'sse|avx'` or use the library's detect_simd_level() -->

**Build Type:**
<!-- Debug or Release -->

## Steps to Reproduce

<!-- Provide a minimal, reproducible example -->

1. Build with: <!-- e.g., cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -->
2. Run with: <!-- e.g., ./test_program -->
3. Observe: <!-- what happens -->

### Minimal Code Example

```c
// Minimal code that reproduces the issue
#include <dynemit.h>

int main(void) {
    // Your code here
    return 0;
}
```

### Build Commands

```bash
# Commands used to build and run
```

## Expected Behavior

<!-- What you expected to happen -->

## Actual Behavior

<!-- What actually happened -->

## Error Messages

<!-- If applicable, paste any error messages, compiler warnings, or runtime errors -->

```
Paste error messages here
```

### Compiler Output

<!-- If applicable, compilation errors or warnings -->

```
Paste compiler output here
```

### Runtime Output

<!-- If applicable, program output or crash information -->

```
Paste runtime output here
```

### Core Dump / Backtrace

<!-- If the program crashed, include backtrace from gdb -->

```
gdb ./your_program core
(gdb) bt
```

## Additional Context

### Severity

- [ ] Critical - Incorrect results, data corruption, or crashes
- [ ] High - Significant performance degradation or incorrect behavior
- [ ] Medium - Issues in specific scenarios or configurations
- [ ] Low - Minor inconvenience or cosmetic issue

### Frequency

- [ ] Always reproducible
- [ ] Intermittent
- [ ] Rare

### Affected Components

<!-- Check all that apply -->

- [ ] CPU feature detection (`detect_simd_level()`)
- [ ] SIMD implementations
  - [ ] Scalar fallback
  - [ ] SSE2
  - [ ] SSE4.2
  - [ ] AVX
  - [ ] AVX2
  - [ ] AVX-512F
- [ ] ifunc resolvers
- [ ] Specific feature:
  - [ ] vector_add
  - [ ] vector_mul
  - [ ] vector_sub
  - [ ] Other: <!-- specify -->
- [ ] CMake build system
- [ ] C++ compatibility
- [ ] Thread safety (`detect_simd_level_ts()`)
- [ ] Header includes
- [ ] Documentation
- [ ] Benchmarks
- [ ] Tests
- [ ] Other: <!-- specify -->

## Workarounds

<!-- If you found any workarounds, please describe them -->

## Possible Fix

<!-- If you have suggestions on how to fix the bug -->

## Related Issues

<!-- Link any related issues or PRs -->

## Testing

<!-- Have you tested with different configurations? -->

- [ ] Tested with different compiler (GCC vs Clang)
- [ ] Tested with different optimization levels (-O0, -O2, -O3)
- [ ] Tested on different CPU architecture
- [ ] Tested with Debug build
- [ ] Tested with Release build
- [ ] Verified with latest version from main branch
- [ ] Tested with sanitizers (AddressSanitizer, UndefinedBehaviorSanitizer)

### Sanitizer Output

<!-- If you tested with sanitizers, include the output -->

```
Paste sanitizer output here
```

---

**Note:** For security vulnerabilities, please **DO NOT** open a public issue. Instead, email murilo.chianfa@outlook.com. See [SECURITY.md](../SECURITY.md) for details.
