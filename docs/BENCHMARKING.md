# Benchmarking Guide

This document explains how to run benchmarks, collect performance data, and generate visualization charts for libdynemit.

## Overview

The libdynemit benchmarking system allows you to:
- Measure **single-core** performance of vector operations across different array sizes
- Compare SIMD implementations (Scalar, SSE2, SSE4.2, AVX, AVX2, AVX-512F)
- Analyze cache effects and scalability
- Collect **statistical data** from multiple trials for robust performance analysis

**Note:** This benchmark measures single-threaded, single-core performance to isolate SIMD instruction efficiency without multi-threading overhead.

## Statistical Methodology

### Multiple Trials for Robustness

Each array size is benchmarked with **10 independent trials** to account for system noise and variability. This provides:

- **Median time**: More robust than mean, resistant to outliers from context switches or cache effects
- **Standard deviation**: Measures variability across trials
- **99th percentile (p99)**: Shows worst-case performance
- **Min/Max**: Captures the full range of observed performance

### Why Median Over Mean?

The benchmark reports **median** values as the primary metric because:
- System noise (context switches, interrupts) can cause occasional slow runs
- Median is resistant to these outliers while mean can be skewed
- More representative of "typical" performance
- Standard practice in performance benchmarking

### Interpreting Statistical Data

**Shaded regions (±1σ)**: Shows one standard deviation above and below the median. Narrower regions indicate more consistent performance.

**Error bars**: Visual representation of standard deviation. Larger bars suggest higher variability.

**P99 markers (x)**: Show the 99th percentile timing. If p99 is far from median, performance has occasional slow outliers.

**Example interpretation:**
- Small error bars + low p99 = Consistent, predictable performance
- Large error bars + high p99 = Variable performance, possibly cache/thermal effects

## Quick Start

### 1. Build the Benchmark

```bash
cd libdynemit
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make
```

The benchmark executable will be located at `build/bench/benchmark_vector_mul`.

**Note:** The benchmark is statically linked, making it portable across different Linux distributions and glibc versions without compatibility issues.

### 2. Run a Simple Benchmark

```bash
./build/bench/benchmark_vector_mul
```

This runs the benchmark with human-readable output showing:
- Detected SIMD level
- Performance metrics for each array size
- Correctness verification

Example output:
```
===========================================
Vector Multiply Benchmark
===========================================
Detected SIMD level: AVX2
(this is the version the ifunc dispatcher will pick)

--- Benchmarking size: 1024 elements ---
  n = 1024, iters = 5000, trials = 10
  median = 0.001234 ms, mean = 0.001245 ms
  stddev = 0.000023 ms, min = 0.001210 ms, max = 0.001289 ms
  p99 = 0.001278 ms
  GFLOP/s = 0.8296 (based on median)
  correctness: OK

...
```

### 3. Generate CSV Data

For post-processing and chart generation, use CSV output:

```bash
./build/bench/benchmark_vector_mul --csv > results_avx2.csv
```

Or use **auto-detect mode** to automatically generate a filename based on your CPU and SIMD level:

```bash
./build/bench/benchmark_vector_mul --auto-detect
# Creates: bench/data/results_amd_ryzen_9_9950x3d_avx_512f.csv
```

The auto-detect mode:
- Reads CPU model from `/proc/cpuinfo`
- Detects SIMD level automatically
- Saves to `bench/data/` directory
- Generates descriptive filename: `results_<cpu_model>_<simd_level>.csv`
- Perfect for collecting data from multiple machines without manual naming

CSV format:
```
array_size,median_ms,mean_ms,stddev_ms,min_ms,max_ms,p99_ms,gflops,simd_level
1024,0.001234,0.001245,0.000023,0.001210,0.001289,0.001278,0.8296,AVX2
2048,0.002456,0.002468,0.000045,0.002420,0.002534,0.002515,0.8341,AVX2
...
```

**Column descriptions:**
- `array_size`: Number of float elements in the arrays
- `median_ms`: Median time across 10 trials (primary metric)
- `mean_ms`: Mean time across 10 trials
- `stddev_ms`: Standard deviation of times
- `min_ms`: Fastest time observed
- `max_ms`: Slowest time observed
- `p99_ms`: 99th percentile time
- `gflops`: GFLOP/s calculated from median time
- `simd_level`: SIMD instruction set used

## Best Practices for Fair Benchmarking

### CPU Core Pinning with taskset

For more consistent and fair benchmark results, it's recommended to pin the benchmark process to a specific CPU core using `taskset`. This prevents the OS scheduler from moving your process between cores, which can introduce variability due to:

- **Cache invalidation**: Moving between cores loses cached data
- **Core-to-core migration overhead**: Context switching between different physical/logical cores
- **CPU frequency variations**: Different cores may run at different frequencies (especially on hybrid architectures)
- **Thermal throttling differences**: Some cores may be hotter than others

**Run the benchmark on a single core:**

```bash
taskset -c 0 ./build/bench/benchmark_vector_mul
```

This pins the process to CPU core 0. You can use any core number (0, 1, 2, etc.).

**With CSV output:**

```bash
taskset -c 0 ./build/bench/benchmark_vector_mul --csv > results.csv
```

**With auto-detect:**

```bash
taskset -c 0 ./build/bench/benchmark_vector_mul --auto-detect
```

**Checking which cores are available:**

```bash
# List all available CPU cores
lscpu

# Or check the number of cores
nproc
```

## Comparing Different SIMD Levels

Since the benchmark automatically detects and uses the best available SIMD level on your CPU, to compare different levels you need to:

### Option 1: Run on Different Machines (Recommended)

Use `--auto-detect` to automatically collect properly-named results from each machine:

```bash
# On Machine 1 (e.g., AMD Ryzen 9 9950X3D with AVX-512F)
./build/bench/benchmark_vector_mul --auto-detect
# Creates: results_amd_ryzen_9_9950x3d_avx_512f.csv

# On Machine 2 (e.g., Intel Xeon E5-2680 v4 with AVX2)
./build/bench/benchmark_vector_mul --auto-detect
# Creates: results_intel_xeon_e5_2680_v4_avx2.csv

# On Machine 3 (e.g., older Intel Core i5 with SSE4.2)
./build/bench/benchmark_vector_mul --auto-detect
# Creates: results_intel_core_i5_6500_sse4_2.csv
```

Copy all CSV files to one machine for chart generation.

### Option 2: Manual CSV Output

If you prefer manual filenames:

```bash
# On a machine with AVX2 support
./build/bench/benchmark_vector_mul --csv > results_avx2.csv

# On a machine with only SSE4.2 support
./build/bench/benchmark_vector_mul --csv > results_sse42.csv

# On a machine with AVX-512F support
./build/bench/benchmark_vector_mul --csv > results_avx512.csv
```

## Generating Charts

### Prerequisites

Install Python dependencies:

```bash
pip install -r scripts/requirements.txt
```

Or install manually:
```bash
pip install matplotlib numpy
```

### Basic Usage

Generate a chart from CSV files (labels auto-inferred from filenames):

```bash
# Single file
python3 scripts/plot_benchmark.py bench/data/results_amd_ryzen_9_9950x3d_avx_512f.csv

# Multiple files (comparison chart)
python3 scripts/plot_benchmark.py bench/data/*.csv

# With custom output
python3 scripts/plot_benchmark.py bench/data/*.csv --output my_chart.png
```

The script automatically:
- Infers CPU names and SIMD levels from filenames
- Saves to `docs/img/benchmark_vector_mul.png` by default
- Formats labels nicely (e.g., "AMD RYZEN 9 9950X3D (AVX-512F)")

**Custom labels (optional):**
```bash
python3 scripts/plot_benchmark.py \
  --input bench/data/results_amd_ryzen_9_9950x3d_avx_512f.csv:"My Custom Label"
```

### Comparing Multiple SIMD Levels

Combine results from different CPUs/SIMD levels:

```bash
# Collect all CSV files from bench/data/ and generate comparison
python3 scripts/plot_benchmark.py bench/data/*.csv

# Or be selective
python3 scripts/plot_benchmark.py \
  bench/data/results_intel_*.csv \
  bench/data/results_amd_*.csv \
  --title "Intel vs AMD Performance"
```

### Chart Types

**Time-based chart (default):**
```bash
python3 scripts/plot_benchmark.py bench/data/*.csv --metric time
```

**GFLOP/s chart:**
```bash
python3 scripts/plot_benchmark.py bench/data/*.csv --metric gflops
```

## Understanding the Results

### Array Sizes

The benchmark tests these array sizes:
- Small arrays (512 - 32K): L1/L2 cache performance
- Medium arrays (64K - 512K): L3 cache performance
- Large arrays (1M - 4M): Main memory bandwidth
- Very large arrays (5M - 16M): Memory bandwidth saturation

Each element is a 32-bit float (4 bytes).

**Size ranges:**
- **512 to 128K**: Dense sampling for cache behavior analysis
- **256K to 4M**: Transition from cache to memory-bound
- **5M to 16M**: Large-scale memory bandwidth testing

### Cache Effects

You'll typically see different performance characteristics at different scales:

- **1K-32K**: Data fits in L1/L2 cache (very fast)
- **64K-256K**: Data fits in L3 cache (fast)
- **512K-1M**: Data spills to main RAM (slower, memory-bound)

Actual speedup depends on:
- Memory bandwidth
- Cache behavior
- CPU microarchitecture

### Interpreting Charts

The generated charts include:

**Main line (median)**: The primary performance metric, showing typical execution time at each array size.

**Shaded region (±1σ)**: One standard deviation above and below the median. Narrower regions indicate more consistent performance.

**Error bars**: Visual representation of standard deviation across trials.

**P99 markers (×)**: Show 99th percentile performance. Distance from median indicates outlier frequency.

**Secondary y-axis**: Time in milliseconds for easier reading of small values.

**Performance characteristics to look for:**

**Linear scaling region:**
- Small arrays where compute dominates
- SIMD speedup is most visible
- Consistent performance (small error bars)

**Flat/sublinear region:**
- Large arrays where memory bandwidth dominates
- All SIMD levels may converge in performance
- May show higher variability (larger error bars) due to cache effects

**Transition points:**
- Sudden slope changes often correspond to cache boundaries
- L1 → L2 → L3 → RAM transitions visible as inflection points

### Benchmark Runtime

The benchmark runs 10 trials per array size, so expect longer execution times compared to single-run benchmarks:

- **Small arrays (< 100K elements)**: 5,000 iterations × 10 trials = 50,000 calls
- **Medium arrays (100K - 2M elements)**: 2,000 iterations × 10 trials = 20,000 calls
- **Large arrays (2M - 5M elements)**: 1,000 iterations × 10 trials = 10,000 calls
- **Very large arrays (> 5M elements)**: 500 iterations × 10 trials = 5,000 calls

Total benchmark runtime: typically 2-5 minutes depending on CPU speed and array sizes tested.

## Example Workflow

Complete workflow from build to chart:

```bash
# 1. Build
cd libdynemit
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make

# 2. Run benchmark (auto-saves to bench/data/)
cd ..
./build/bench/benchmark_vector_mul --auto-detect

# 3. Install Python dependencies (first time only)
pip install -r scripts/requirements.txt

# 4. Generate chart (labels auto-inferred)
python3 scripts/plot_benchmark.py bench/data/*.csv

# 5. View the chart
xdg-open docs/img/benchmark_vector_mul.png
```

## Contributing

Improvements welcome:
- Additional benchmark metrics (memory bandwidth, cache misses, etc.)
- Automated multi-CPU testing infrastructure
- Benchmark regression detection
- Further statistical analysis (confidence intervals, hypothesis testing)

## Appendix: Statistical Notes

### Why 10 Trials?

10 trials provides a good balance between:
- Statistical reliability (enough samples for median/stddev)
- Reasonable runtime (not too many iterations)
- Outlier detection (can identify anomalous runs)

