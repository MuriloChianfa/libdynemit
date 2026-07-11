# Development Setup

## Prerequisites

- GCC 13+ or Clang 16+ (C23 required)
- CMake 3.16+

## Build

```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build -j$(nproc)
```

Symlink the compilation database so your IDE resolves headers correctly:

```bash
ln -sf build/compile_commands.json compile_commands.json
```

## Tests

All C tests use the [Unity](https://github.com/ThrowTheSwitch/Unity) framework
(fetched automatically via CMake FetchContent). Unity provides structured assertions
like `TEST_ASSERT_DOUBLE_WITHIN`, `TEST_ASSERT_EQUAL_UINT64`, etc., with clear
per-test-function failure reporting.

Run the full test suite (core + all features):

```bash
ctest --test-dir build --output-on-failure
```

### Test layout

Core library tests live in `tests/` and cover SIMD detection, resolver macros, feature
discovery, and C++ compatibility. Each feature has its own tests under
`features/<name>/tests/`. All tests are registered with CTest automatically.

Run a single feature test directly:

```bash
./build/features/add/test_add
./build/features/sum/test_sum
```

## Valgrind

Memory and thread checking for feature tests. Build Debug with TLS enabled so
thread-local scratch paths in `hll` / `entropy` are exercised:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DDYNEMIT_TS=ON
cmake --build build -j$(nproc)
```

Run a single Valgrind tool over all `features/*/test_*` binaries:

```bash
./scripts/run_valgrind.sh --build-dir build-valgrind --tool memcheck
./scripts/run_valgrind.sh --build-dir build-valgrind --tool helgrind --parallel 4
./scripts/run_valgrind.sh --build-dir build-valgrind --tool massif --output-dir reports
```

## Code Coverage (lcov)

Generate an HTML coverage report over all tests:

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DDYNEMIT_COVERAGE=ON
cmake --build build -j$(nproc)
cmake --build build --target coverage
```

Open `build/coverage_report/index.html` in a browser. Requires GCC (for gcov),
`lcov`, and `genhtml`.

## Static Analysis (clang-tidy)

Requires clang and clang-tidy. Configure a dedicated clang build so `compile_commands.json` reflects clang
flags (the default `build/` directory may have been generated with GCC):

```bash
cmake -B build-tidy -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

Run clang-tidy on feature implementations only (excludes `tests/` and
`benchmarks/` subdirectories):

```bash
run-clang-tidy-21 -p build-tidy -checks='performance-*' -header-filter=.* 'features/[^/]+/[^/]+\.c'
```

This check also runs in CI (see the `clang-tidy-performance` job in `.github/workflows/ci.yml`).

## Mutation Testing (Mull)

Mull injects mutations into compiled bitcode to verify test quality. It requires
Clang and the `mull` package.

```bash
curl -1sLf 'https://dl.cloudsmith.io/public/mull-project/mull-stable/setup.deb.sh' | sudo -E bash
sudo apt-get install mull-20
```

Build with the Mull pass plugin enabled:

```bash
cmake -B build-mull -DCMAKE_C_COMPILER=clang -DDYNEMIT_MULL=ON
cmake --build build-mull -j$(nproc)
```

Run mutation testing on individual test binaries:

```bash
mull-runner-20 ./build-mull/features/add/test_add
mull-runner-20 ./build-mull/features/sum/test_sum
```

The `mull.yml` config at the project root controls which mutators are active.
Mull primarily tests scalar implementations; SIMD stub functions that delegate
to the scalar path produce trivial mutants that can be ignored in the report.

## Build Types

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake -B build -DCMAKE_BUILD_TYPE=Release
```

## List Available Features

```bash
cmake -B build -DLIST_FEATURES=ON
```

Features are auto-discovered from `features/*/` subdirectories at configure time.

## Project Structure

```
features/<name>/
  ├── CMakeLists.txt        # library + test + benchmark targets
  ├── <name>_<type>.c       # SIMD implementations + _select() + ifunc
  ├── tests/
  │   └── test_<name>.c     # correctness tests
  └── benchmarks/
      └── bench_<name>.c    # performance benchmarks

tests/                      # core-only tests (SIMD detection, C++ compat)
bench/
  └── bench_utils.h         # shared benchmark infrastructure (header-only)
include/dynemit/
  └── <name>.h              # public API: function + typedef + _select()
```

## Adding a New Feature

1. Create `features/<name>/` with implementation `.c` files following the existing
   SIMD variant + `_select()` + ifunc resolver pattern (see `features/add/add_f32.c`).
2. Create the public header `include/dynemit/<name>.h` with the function prototype,
   function pointer typedef, and `_select()` declaration.
3. Add the header include to `include/dynemit.h`.
4. Add a test in `features/<name>/tests/test_<name>.c`.
5. Add a benchmark in `features/<name>/benchmarks/bench_<name>.c`.
6. Create `features/<name>/CMakeLists.txt` with library, test, and benchmark targets.
7. The root `CMakeLists.txt` auto-discovers the new directory, no edits needed there.
