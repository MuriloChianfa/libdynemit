# CodeQL Security Analysis

This repository uses CodeQL for automated security scanning of C/C++ source code.

## Overview

The CodeQL workflow (`codeql.yml`) performs static analysis security testing (SAST) on:

- **C Code**: Core CPU detection in `src/`
- **C Code**: SIMD implementations in `features/`
- **C/C++ Headers**: Public API in `include/`
- **C++ Code**: C++ compatibility tests in `tests/`

## Workflow Triggers

The CodeQL analysis runs automatically on:

1. **Push events** to `main` branch
2. **Pull requests** targeting `main` branch
3. **Scheduled runs** every Monday at 2:30 AM UTC
4. **Manual dispatch** via GitHub Actions UI

## What Gets Analyzed

### C/C++ Analysis

- Core CPU detection (`src/dynemit.c`)
- SIMD feature implementations (`features/*/`)
- Public headers (`include/dynemit/`)
- ifunc resolvers and runtime dispatch
- Checks for:
  - Buffer overflows
  - Integer overflows
  - Out-of-bounds array access
  - NULL pointer dereferences
  - Unsafe pointer arithmetic
  - Memory alignment issues
  - Use-after-free
  - Double-free
  - Memory leaks

## Query Suites

The workflow uses enhanced query suites:

- `security-extended`: Extended set of security queries
- `security-and-quality`: Security queries plus code quality checks

## Build Process

### C/C++ Build

The C/C++ code is built using CMake:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=gcc-13 -DCMAKE_CXX_COMPILER=g++-13
cmake --build build -j$(nproc)
```

This ensures CodeQL can analyze the complete codebase including all SIMD implementations.

## Viewing Results

### Pull Requests

- CodeQL findings appear as annotations directly in PRs
- New issues are highlighted in the diff view
- Security issues block PR merging until resolved

### Security Tab

- Navigate to **Security** > **Code scanning alerts** in the repository
- Filter by language, severity, or rule
- View detailed analysis and remediation suggestions

## Configuration

The analysis is configured in:

- `.github/workflows/codeql.yml` - Main workflow
- `.github/workflows/codeql/codeql-config.yml` - Path configuration and query suites

### Paths Analyzed

The configuration analyzes these paths:

```yaml
paths:
  - src
  - features
  - include
```

### Paths Excluded

The following paths are excluded from analysis:

```yaml
paths-ignore:
  - tests
  - bench
  - build
  - docs
  - scripts
  - '**/*.md'
```

## Best Practices

1. **Review findings promptly**: Address security issues as soon as they appear
2. **Don't dismiss without investigation**: Understand each finding before closing
3. **Monitor scheduled scans**: Weekly scans catch newly discovered vulnerability patterns
4. **Test with sanitizers**: Use AddressSanitizer and UndefinedBehaviorSanitizer locally
5. **Follow secure coding**: Validate inputs, check bounds, handle NULL pointers

## Common Security Issues in SIMD Code

### Buffer Overflows

SIMD code processes multiple elements at once. Ensure loop bounds are correct:

```c
// Potential issue
for (size_t i = 0; i < n; i += 8) {
    // May read past array bounds if n is not multiple of 8
}

// Better
size_t i = 0;
for (; i + 8 <= n; i += 8) {
    // Process 8 elements
}
for (; i < n; i++) {
    // Handle remainder
}
```

### Integer Overflows

Size calculations can overflow:

```c
// Potential issue
size_t total = n * sizeof(float);  // May overflow

// Better
if (n > SIZE_MAX / sizeof(float)) {
    // Handle error
}
size_t total = n * sizeof(float);
```

### Memory Alignment

SIMD operations require aligned memory:

```c
// Potential issue
float *data = malloc(n * sizeof(float));  // May not be aligned

// Better
float *data = aligned_alloc(32, n * sizeof(float));  // 32-byte aligned for AVX
```

## Resources

- [CodeQL Documentation](https://codeql.github.com/docs/)
- [CodeQL for C/C++](https://codeql.github.com/docs/codeql-language-guides/codeql-for-cpp/)
- [GitHub Code Scanning](https://docs.github.com/en/code-security/code-scanning)
- [C/C++ Security Queries](https://codeql.github.com/codeql-query-help/cpp/)

## Support

For issues with CodeQL analysis:

1. Check the [Actions logs](https://github.com/MuriloChianfa/libdynemit/actions)
2. Review [Security tab](https://github.com/MuriloChianfa/libdynemit/security)
3. Open an issue with the `security` label
4. Contact repository maintainers

## False Positives

If CodeQL reports a false positive:

1. Verify it's actually a false positive
2. Add an inline comment explaining why it's safe
3. Use `// lgtm[cpp/rule-id]` to suppress specific warnings
4. Document the suppression reason

Example:

```c
// This is safe because n is validated to be <= MAX_SIZE before this point
// lgtm[cpp/integer-multiplication-cast-to-long]
size_t total = n * sizeof(float);
```

## Security Disclosure

If CodeQL identifies a security vulnerability:

- **DO NOT** discuss it in public issues
- Report via email: murilo.chianfa@outlook.com
- See [SECURITY.md](SECURITY.md) for full disclosure process
