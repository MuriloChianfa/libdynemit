# Benchmarking

Every feature variant ships with its own benchmark binary under
`features/<name>/benchmarks/`. Benchmark binaries use explicit type suffixes
(e.g. `bench_max_f64`, `bench_max_u32`). All benchmarks share the same CLI
interface and produce the same CSV format, so the plotting script works with
any of them.

## Quick Start

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DDYNEMIT_BUILD_BENCHMARKS=ON
cmake --build build -j$(nproc)

# Run all variants, all SIMD levels, pinned to a single core
sudo ./scripts/run_all_benchmarks.sh --cpu 15

# Or regenerate charts from existing CSV data only
bash ./scripts/run_all_benchmarks.sh --charts-only
```

## CLI Options

Every benchmark supports the same flags:

```
Usage: bench_<name>_<type> [OPTIONS]

Options:
  --csv              Output results in CSV format to stdout
  --force-level LVL  Force a specific SIMD level instead of auto-detection
                     Valid levels: scalar, sse2, sse4.2, avx, avx2, avx512f
  --auto-detect      Auto-detect CPU and SIMD level, write CSV to file
  --help, -h         Show help
```

### Default mode (human-readable)

Uses the best SIMD level available on the current CPU (identical to ifunc dispatch):

```bash
./build/features/mul/bench_mul_f32
```

### Force a specific SIMD level

Directly calls a particular SIMD implementation via the `_select()` API:

```bash
./build/features/mul/bench_mul_f32 --force-level scalar
./build/features/mul/bench_mul_f32 --force-level avx2
```

### CSV output

```bash
./build/features/mul/bench_mul_f32 --csv
./build/features/mul/bench_mul_f32 --csv --force-level sse2
```

### Auto-detect (save to file)

Detects the CPU model, architecture, and SIMD level, then writes CSV to the
structured benchmark directory:

```bash
./build/features/mul/bench_mul_f32 --auto-detect
# Creates: bench/cpus/x86_64/<cpu_model>/data/mul_f32_<simd_level>.csv

# Combine with --force-level to save a specific SIMD level result:
./build/features/mul/bench_mul_f32 --auto-detect --force-level avx2
# Creates: bench/cpus/x86_64/<cpu_model>/data/mul_f32_avx2.csv
```

## CSV Format

All benchmarks emit the same columns:

```
array_size,median_ms,mean_ms,stddev_ms,min_ms,max_ms,p99_ms,gflops,simd_level
```

10 trials are run per array size. The `median_ms` column is used for chart plotting
and `gflops` is derived from the median.

## Directory Layout

All benchmark artefacts live under `bench/`:

```
bench/
  cpus/                                          # Per-CPU data and charts
    x86_64/                                      # Architecture grouping
      amd_ryzen_9_9950x3d/
        data/                                    # CSV results
          max_f64_scalar.csv
          max_f64_avx2.csv
          max_u32_avx_512f.csv
          ...
        features/                                # Per-variant charts
          max_f64/
            timing.png                           # SIMD comparison (time)
            throughput.png                        # SIMD comparison (GFLOP/s)
          max_u32/
            timing.png
            throughput.png
          ...
    aarch64/
      arm_neoverse_v2/
        data/
          max_u32_neon.csv
          max_u32_sve.csv
          ...
        features/
          max_u32/
            timing.png
            throughput.png
  features/                                      # Cross-CPU comparison charts
    max_f64/
      timing.png                                 # Best SIMD per CPU, all CPUs
      throughput.png
    max_u32/
      timing.png
      throughput.png
    ...
```

## Running All Benchmarks

The `scripts/run_all_benchmarks.sh` script runs all feature variants across all
SIMD levels with CPU pinning and max scheduling priority for
fair, reproducible results.

```bash
# Full run: build + benchmark + charts
sudo ./scripts/run_all_benchmarks.sh

# Pin to specific CPU core
sudo ./scripts/run_all_benchmarks.sh --cpu 15

# Skip build step (binaries already compiled)
sudo ./scripts/run_all_benchmarks.sh --skip-build

# Only regenerate charts from existing CSV data (no sudo needed)
bash ./scripts/run_all_benchmarks.sh --charts-only
```

The script uses `taskset -c <core>` to pin each benchmark to a single CPU core
and `nice -n -20` for maximum scheduling priority. By default it picks the last
physical core to avoid core 0, which typically handles hardware interrupts.

## Chart Layout

The script generates two types of charts per variant, each as both timing and throughput:

| Type | Path | Description |
|------|------|-------------|
| SIMD comparison | `bench/cpus/{arch}/{cpu}/features/{variant}/timing.png` | All SIMD levels on one CPU |
| SIMD comparison | `bench/cpus/{arch}/{cpu}/features/{variant}/throughput.png` | Same, in GFLOP/s |
| CPU comparison | `bench/features/{variant}/timing.png` | Best SIMD per CPU, across CPUs |
| CPU comparison | `bench/features/{variant}/throughput.png` | Same, in GFLOP/s |

SIMD charts use clean labels (Scalar, SSE2, SSE4.2, AVX, AVX2, AVX-512F).
CPU charts label each line with the CPU name and its best SIMD level.

## Generating Charts

Charts are generated by `scripts/plot_benchmark.py`, which is called
automatically by `run_all_benchmarks.sh`. You can also invoke it directly:

Prerequisites:

```bash
pip install matplotlib numpy
```

### Plotting options

```bash
# Explicit labels
python scripts/plot_benchmark.py --input file1.csv:Label1 file2.csv:Label2

# Plot GFLOP/s instead of time
python scripts/plot_benchmark.py --input file1.csv:Label1 --metric gflops

# Custom title and output
python scripts/plot_benchmark.py --input file1.csv:Label1 --title "My Title" -o chart.png
```

## Portable Benchmark Bundle

To benchmark on remote servers without building from source, create a portable
bundle with statically-linked binaries:

```bash
# Build the bundle (on your dev machine)
./scripts/bundle_benchmarks.sh --strip
# Produces: dynemit-bench-x86_64.tar.gz
```

The bundle contains static binaries and a self-contained runner script.
No compiler, cmake, or libraries needed on the target machine.

### Upload and run on a server

```bash
scp dynemit-bench-x86_64.tar.gz server:
ssh server

tar xzf dynemit-bench-x86_64.tar.gz
cd dynemit-bench
sudo ./run.sh --cpu 0
```

The bundled `run.sh` supports:

```
Usage: sudo ./run.sh [--cpu CORE] [--levels LEVELS] [--features FEATURES]

Options:
  --cpu CORE          Pin to CPU core (default: last physical core)
  --levels LEVELS     Comma-separated: scalar,sse2,sse4.2,avx,avx2,avx512f
  --features FEATURES Comma-separated variant names (default: all)
```

### Collect results and generate charts

```bash
# Copy the bench/cpus/ tree back to your dev machine
scp -r 'server:dynemit-bench/bench/cpus' bench/

# Regenerate charts (picks up the new CPU data automatically)
bash scripts/run_all_benchmarks.sh --charts-only
```

The chart generator automatically discovers all CPUs from the directory structure,
so adding data from new servers produces multi-line CPU comparison charts
without any configuration changes.

## Static Build Option

Benchmarks can be statically linked via the CMake option:

```bash
cmake -B build-static -DCMAKE_BUILD_TYPE=Release \
  -DDYNEMIT_BUILD_BENCHMARKS=ON \
  -DDYNEMIT_STATIC_BENCHMARKS=ON
cmake --build build-static -j$(nproc)
```

This is used internally by `scripts/bundle_benchmarks.sh`. The resulting
binaries run on any Linux of the matching architecture regardless of glibc version.
