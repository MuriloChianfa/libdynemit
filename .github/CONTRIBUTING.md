# Contributing to libdynemit

Thank you for your interest in contributing to libdynemit! We welcome contributions from the community and appreciate your efforts to make this SIMD library better.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Environment](#development-environment)
- [How to Contribute](#how-to-contribute)
- [Coding Standards](#coding-standards)
- [Testing Guidelines](#testing-guidelines)
- [Pull Request Process](#pull-request-process)
- [Reporting Bugs](#reporting-bugs)
- [Feature Requests](#feature-requests)

## Code of Conduct

This project and everyone participating in it is governed by our [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to murilo.chianfa@outlook.com.

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/libdynemit.git
   cd libdynemit
   ```
3. **Add upstream remote**:
   ```bash
   git remote add upstream https://github.com/MuriloChianfa/libdynemit.git
   ```
4. **Create a branch** for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   ```

## Development Environment

### Requirements

- **Linux** with x86_64 architecture
- **GCC 13+** or **Clang 16+**
- **CMake 3.16+**
- **make** or **ninja**

### Ubuntu/Debian

```bash
sudo apt update
sudo apt install -y gcc-13 g++-13 cmake make
```

### Fedora

```bash
sudo dnf install -y gcc gcc-c++ cmake make
```

### Arch Linux

```bash
sudo pacman -S gcc cmake make
```

### Build

```bash
# Clone the repository
git clone https://github.com/MuriloChianfa/libdynemit.git
cd libdynemit

# Create build directory
mkdir build && cd build

# Configure (Release build with optimizations)
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)

# Run tests
ctest --output-on-failure
```

## How to Contribute

### Types of Contributions

We welcome various types of contributions:

- **Bug fixes**: Fix issues reported in the issue tracker
- **New features**: Add new SIMD operations or optimizations
- **Documentation**: Improve README, guides, or code comments
- **Tests**: Add test cases or improve test coverage
- **Performance optimizations**: Improve SIMD implementations
- **Architecture support**: Add ARM NEON or other SIMD architectures
- **Benchmarks**: Add benchmarks for new features

### Before You Start

1. **Check existing issues** to see if someone is already working on it
2. **Open an issue** for discussion if you're proposing a significant change
3. **Keep changes focused**: One feature/fix per pull request
4. **Follow the coding standards** described below

## Coding Standards

### C Code Style

- Follow **C23** standard
- Use **tabs** for indentation (width: 4 spaces)
- Maximum line length: **120 characters**
- Use `clang-format` for formatting
- Document public API functions with comments
- Use descriptive variable names

### Include Order (C)

```c
// Standard library headers
#include <stddef.h>
#include <immintrin.h>

// Project headers
#include <dynemit/core.h>
#include <dynemit/compiler.h>
```

### SIMD Implementation Guidelines

When implementing new SIMD features:

1. **Provide all SIMD levels**: scalar, SSE2, SSE4.2, AVX, AVX2, AVX-512F
2. **Use target attributes**: `__attribute__((target("avx2")))`
3. **Disable auto-vectorization for scalar**: Use `DYNEMIT_NO_AUTOVECTORIZE`
4. **Use ifunc resolvers**: Implement runtime dispatch with `__attribute__((ifunc(...)))`
5. **Follow existing patterns**: See `features/add/` or `features/mul/` as examples

See [docs/ADDING_FEATURES.md](../docs/ADDING_FEATURES.md) for detailed instructions.

### CMake Style

- Use **2 spaces** for indentation in CMakeLists.txt
- Follow the existing project structure
- Use target properties over global settings

### Commit Messages

```
Brief summary (30-40 chars or less)

More detailed explanation if needed. Explain the problem
this commit solves and why you chose this approach.

Closes #123
```

## Testing Guidelines

### Running Tests

```bash
# All tests
cd build
ctest --output-on-failure

# Specific test
./tests/test_vector_ops

# Verbose output
ctest --verbose
```

### Writing Tests

- Add tests in the `tests/` directory
- Use descriptive test names
- Test edge cases: zero-length arrays, misaligned data, large arrays
- Verify correctness across all SIMD levels
- Test both C and C++ usage

### Running Benchmarks

```bash
# Run a single feature benchmark
./build/features/mul/bench_mul_f32
./build/features/mul/bench_mul_f32 --auto-detect

# Run all benchmarks and generate charts
sudo ./scripts/run_all_benchmarks.sh
```

## Pull Request Process

### Before Submitting

1. **Update your branch** with latest upstream:
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **Run all tests**:
   ```bash
   cd build
   ctest --output-on-failure
   ```

3. **Check formatting**:
   ```bash
   clang-format -i src/**/*.c features/**/*.c include/**/*.h
   ```

4. **Run static analysis** (optional but recommended):
   ```bash
   cppcheck --enable=warning,style,performance src/ features/
   ```

5. **Write clear commit messages**

### Submitting the PR

1. **Push your branch** to your fork
2. **Open a pull request** against the `main` branch
3. **Fill out the PR template** completely
4. **Link related issues** using "Closes #123" or "Fixes #456"
5. **Wait for CI** to complete
6. **Respond to review comments** promptly

### PR Templates

We provide templates for different types of PRs:

- General changes: Default template
- Bug fixes: `BUG_FIX_TEMPLATE.md`
- New features: `FEATURE_TEMPLATE.md`

Select the appropriate template when creating your PR.

### Review Process

- A maintainer will review your PR within a few days
- Address review comments by pushing new commits
- Once approved, a maintainer will merge your PR
- PRs require at least one approval from a maintainer

## Reporting Bugs

### Security Vulnerabilities

**DO NOT** open public issues for security vulnerabilities. Use email instead. See [SECURITY.md](SECURITY.md) for details.

### Regular Bugs

Use the **Bug Report** issue template and include:

1. **Description**: Clear description of the bug
2. **Environment**: OS, compiler version (GCC/Clang), CMake version, CPU architecture
3. **Steps to reproduce**: Minimal steps to reproduce the issue
4. **Expected behavior**: What you expected to happen
5. **Actual behavior**: What actually happened
6. **Additional context**: Build logs, test output, etc.

## Feature Requests

Use the **Feature Request** issue template and include:

1. **Problem statement**: What problem does this solve?
2. **Proposed solution**: Your suggested implementation
3. **Alternatives considered**: Other approaches you've thought about
4. **Additional context**: Use cases, examples, performance considerations

## Adding New Features

For detailed instructions on adding new SIMD-optimized features, see [docs/ADDING_FEATURES.md](../docs/ADDING_FEATURES.md).

Quick summary:

1. Create feature directory: `features/your_feature/`
2. Implement all SIMD levels (scalar through AVX-512F)
3. Add public header: `include/dynemit/your_feature.h`
4. Create CMakeLists.txt following the pattern
5. Update main CMakeLists.txt to include in all-in-one library
6. Add tests in `tests/`
7. Update umbrella header `include/dynemit.h`

## Recognition

Contributors will be:

- Credited in release notes for significant contributions
- Mentioned in commit history

Thank you for contributing to libdynemit!
