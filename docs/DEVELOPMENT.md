# Development Setup

## Prerequisites

- GCC 13+ or Clang 16+ (C23 required)
- CMake 3.16+

## Build

```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build -j$(nproc)
```

Symlink the compilation database so your IDE (clangd) resolves headers correctly:

```bash
ln -sf build/compile_commands.json compile_commands.json
```

## Tests

All C tests use the [Unity](https://github.com/ThrowTheSwitch/Unity) framework
(fetched automatically via CMake FetchContent). Unity provides structured assertions
like `TEST_ASSERT_DOUBLE_WITHIN`, `TEST_ASSERT_EQUAL_UINT64`, etc., with clear
per-test-function failure reporting.

Tests are enabled by default for Debug. For Release, pass `-DDYNEMIT_BUILD_TESTS=ON`.
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

### CI coverage matrix

Codecov merges five coverage flags so every compiled ISA path is exercised:

| Flag | Runner | Purpose |
|------|--------|---------|
| `x86_native` | `gcc:14` container on x86 | Scalar through host-max x86 (typically AVX2) |
| `x86_sde` | Ubuntu + Intel SDE `-spr` | AVX-512F and AVX-512-VBMI2 kernel bodies |
| `x86_ts` | Ubuntu, `DYNEMIT_TS=ON` | Thread-safe TLS scratch paths in entropy/hll |
| `aarch64_native` | `ubuntu-24.04-arm` | Scalar through host-max aarch64 (NEON/SVE) |
| `aarch64_sve` | ARM + QEMU user-mode | SVE/SVE2 when the host CPU reports below SVE2 |

Each job uploads `build/coverage.info` with its flag; Codecov unions hits across
flags (see `.codecov.yml` carryforward rules).

### Emulator-backed coverage (local)

When the host CPU cannot execute AVX-512 or SVE kernels, run tests under an
emulator wrapper instead of bare `ctest`:

```bash
# Install Intel SDE (x86_64 only), then:
./scripts/install_intel_sde.sh /opt/intel-sde
export SDE_BIN=/opt/intel-sde/sde64

cmake -B build-sde -DCMAKE_BUILD_TYPE=Debug -DDYNEMIT_COVERAGE=ON \
  "-DDYNEMIT_COVERAGE_TEST_WRAPPER=${SDE_BIN} -spr --"
cmake --build build-sde -j$(nproc)

# Verify the emulator exposes AVX-512 before collecting coverage:
"${SDE_BIN}" -spr -- ./build-sde/dynemit_simd_level_probe 5

cmake --build build-sde --target coverage
```

Alternative without reconfiguring CMake:

```bash
export DYNEMIT_TEST_WRAPPER="${SDE_BIN} -spr --"
./scripts/run_tests_under_emu.sh --build-dir build-sde --emu "${SDE_BIN}" -spr --
# then run lcov capture manually, or use the coverage target with the wrapper set
```

On aarch64 hosts that lack SVE2, use QEMU user-mode:

```bash
cmake -B build-sve -DCMAKE_BUILD_TYPE=Debug -DDYNEMIT_COVERAGE=ON \
  "-DDYNEMIT_COVERAGE_TEST_WRAPPER=qemu-aarch64 -cpu max,sve=on,sve2=on --"
cmake --build build-sve -j$(nproc)
qemu-aarch64 -cpu max,sve=on,sve2=on -- ./build-sve/dynemit_simd_level_probe 11
cmake --build build-sve --target coverage
```

The `dynemit_simd_level_probe` binary (built when `DYNEMIT_COVERAGE=ON`) prints
`detect_simd_level()` and exits non-zero when below the required floor.

## Static Analysis (clang-tidy)

Requires clang and clang-tidy. Configure a dedicated clang build so `compile_commands.json` reflects clang
flags (the default `build/` directory may have been generated with GCC):

```bash
cmake -B build-tidy -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
```

Run clang-tidy on feature implementations only (excludes `tests/` and
`benchmarks/` subdirectories). Checks come from the root `.clang-tidy`
(performance, bugprone, clang-analyzer, plus a small mechanical style set):

```bash
run-clang-tidy-21 -p build-tidy 'features/[^/]+/[^/]+\.c'
```

Feature code uses `memcpys` / `memsets` and `mem_aligned_count()` from `"mem.h"` (under `src/`) instead of raw libc `memcpy`/`memset`.

This also runs in CI (see the `clang-tidy` job in `.github/workflows/ci.yml`).

## Mutation Testing (Mull)

Mull injects mutations into compiled bitcode to verify test quality. It requires
Clang and a matching Mull package (Clang x LLVM matching major).

Install from the official Mull project [GitHub Releases](https://github.com/mull-project/mull/releases):

```bash
MULL_VERSION=0.34.0
MULL_DEB="Mull-18-${MULL_VERSION}-LLVM-18.1.3-ubuntu-amd64-24.04.deb"
curl -fsSL -o "/tmp/${MULL_DEB}" \
  "https://github.com/mull-project/mull/releases/download/${MULL_VERSION}/${MULL_DEB}"
sudo apt install -y "/tmp/${MULL_DEB}"
```

Build with the Mull pass plugin enabled:

```bash
cmake -B build-mull -DCMAKE_C_COMPILER=clang-18 -DDYNEMIT_MULL=ON
cmake --build build-mull -j$(nproc)
```

Run mutation testing on individual test binaries:

```bash
mull-runner-18 ./build-mull/features/add/test_add
mull-runner-18 ./build-mull/features/sum/test_sum
```

The `mull.yml` config at the project root controls which mutators are active.
Mull primarily tests scalar implementations; SIMD stub functions that delegate
to the scalar path produce trivial mutants that can be ignored in the report.

CI installs Mull the same way (see `.github/workflows/mutation.yml`).

## Build Types

```bash
# Debug: tests and benchmarks on by default
cmake -B build -DCMAKE_BUILD_TYPE=Debug

# Release: library only by default
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Release with tests/benchmarks
cmake -B build -DCMAKE_BUILD_TYPE=Release \
  -DDYNEMIT_BUILD_TESTS=ON \
  -DDYNEMIT_BUILD_BENCHMARKS=ON
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
src/
  └── mem.h                 # bounded memcpys / memsets; mem_aligned_count()
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
