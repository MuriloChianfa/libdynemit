#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <math.h>
#include <immintrin.h>
#include <dynemit/core.h>

/* ---------- SIMD Implementation Functions ---------- */

// Scalar version - disable auto-vectorization to get true scalar code
__attribute__((target("default")))
__attribute__((optimize("no-tree-vectorize")))
static void
vector_mul_f32_scalar(const float *a, const float *b, float *out, size_t n)
{
    for (size_t i = 0; i < n; i++)
        out[i] = a[i] * b[i];
}

__attribute__((target("sse2")))
static void
vector_mul_f32_sse2(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 4;
    for (; i + step <= n; i += step) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 vc = _mm_mul_ps(va, vb);
        _mm_storeu_ps(out + i, vc);
    }
    for (; i < n; i++)
        out[i] = a[i] * b[i];
}

__attribute__((target("sse4.2")))
static void
vector_mul_f32_sse42(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 4;
    for (; i + step <= n; i += step) {
        __m128 va = _mm_loadu_ps(a + i);
        __m128 vb = _mm_loadu_ps(b + i);
        __m128 vc = _mm_mul_ps(va, vb);
        _mm_storeu_ps(out + i, vc);
    }
    for (; i < n; i++)
        out[i] = a[i] * b[i];
}

__attribute__((target("avx")))
static void
vector_mul_f32_avx(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 8;
    for (; i + step <= n; i += step) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 vc = _mm256_mul_ps(va, vb);
        _mm256_storeu_ps(out + i, vc);
    }
    for (; i < n; i++)
        out[i] = a[i] * b[i];
}

__attribute__((target("avx2")))
static void
vector_mul_f32_avx2(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 8;
    for (; i + step <= n; i += step) {
        __m256 va = _mm256_loadu_ps(a + i);
        __m256 vb = _mm256_loadu_ps(b + i);
        __m256 vc = _mm256_mul_ps(va, vb);
        _mm256_storeu_ps(out + i, vc);
    }
    for (; i < n; i++)
        out[i] = a[i] * b[i];
}

__attribute__((target("avx512f")))
static void
vector_mul_f32_avx512f(const float *a, const float *b, float *out, size_t n)
{
    size_t i = 0;
    const size_t step = 16;
    for (; i + step <= n; i += step) {
        __m512 va = _mm512_loadu_ps(a + i);
        __m512 vb = _mm512_loadu_ps(b + i);
        __m512 vc = _mm512_mul_ps(va, vb);
        _mm512_storeu_ps(out + i, vc);
    }
    for (; i < n; i++)
        out[i] = a[i] * b[i];
}

/* ---------- CPU model detection ---------- */
static void
get_cpu_model_name(char *buffer, size_t bufsize)
{
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        snprintf(buffer, bufsize, "unknown_cpu");
        return;
    }

    char line[256];
    int found = 0;
    
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "model name", 10) == 0) {
            // Found "model name : ..."
            char *colon = strchr(line, ':');
            if (colon) {
                colon++; // skip the colon
                while (*colon == ' ' || *colon == '\t') colon++; // skip whitespace
                
                // Copy the CPU model name
                size_t len = strlen(colon);
                if (len > 0 && colon[len-1] == '\n') {
                    colon[len-1] = '\0'; // remove newline
                }
                
                // Remove core count information (e.g., "16-Core", "14-Core Processor")
                // Common patterns: "N-Core", "N Core", "Processor"
                char temp[256];
                strncpy(temp, colon, sizeof(temp) - 1);
                temp[sizeof(temp) - 1] = '\0';
                
                // Find and remove patterns like "16-Core", "14 Core", "Processor"
                char *patterns[] = {"-core", " core", "processor", NULL};
                for (int p = 0; patterns[p]; p++) {
                    char *pos = strcasestr(temp, patterns[p]);
                    if (pos) {
                        // Look backwards for the number
                        char *num_start = pos;
                        while (num_start > temp && (isdigit(*(num_start-1)) || *(num_start-1) == ' ' || *(num_start-1) == '-')) {
                            num_start--;
                        }
                        // Remove from num_start to after the pattern
                        char *pattern_end = pos + strlen(patterns[p]);
                        // Skip any trailing spaces
                        while (*pattern_end == ' ') pattern_end++;
                        
                        // Shift the rest of the string
                        memmove(num_start, pattern_end, strlen(pattern_end) + 1);
                    }
                }
                
                // Sanitize: convert to lowercase and replace spaces/special chars with underscores
                size_t j = 0;
                for (size_t i = 0; temp[i] && j < bufsize - 1; i++) {
                    char c = temp[i];
                    if (isalnum(c)) {
                        buffer[j++] = tolower(c);
                    } else if (c == ' ' || c == '-' || c == '(' || c == ')' || c == '@') {
                        if (j > 0 && buffer[j-1] != '_') {
                            buffer[j++] = '_';
                        }
                    }
                }
                // Remove trailing underscore
                while (j > 0 && buffer[j-1] == '_') {
                    j--;
                }
                buffer[j] = '\0';
                found = 1;
                break;
            }
        }
    }
    
    fclose(fp);
    
    if (!found) {
        snprintf(buffer, bufsize, "unknown_cpu");
    }
}

/* ---------- timing helper ---------- */
static double
now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* ---------- statistical helper functions ---------- */
static int
compare_double(const void *a, const void *b)
{
    double da = *(const double *)a;
    double db = *(const double *)b;
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

static double
calculate_median(double *values, int n)
{
    double sorted[n];
    memcpy(sorted, values, n * sizeof(double));
    qsort(sorted, n, sizeof(double), compare_double);
    
    if (n % 2 == 0) {
        return (sorted[n/2 - 1] + sorted[n/2]) / 2.0;
    } else {
        return sorted[n/2];
    }
}

static double
calculate_mean(double *values, int n)
{
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += values[i];
    }
    return sum / (double)n;
}

static double
calculate_stddev(double *values, int n, double mean)
{
    double sum_sq_diff = 0.0;
    for (int i = 0; i < n; i++) {
        double diff = values[i] - mean;
        sum_sq_diff += diff * diff;
    }
    return sqrt(sum_sq_diff / (double)n);
}

static double
calculate_percentile(double *values, int n, double percentile)
{
    double sorted[n];
    memcpy(sorted, values, n * sizeof(double));
    qsort(sorted, n, sizeof(double), compare_double);
    
    double index = percentile * (n - 1);
    int lower = (int)floor(index);
    int upper = (int)ceil(index);
    
    if (lower == upper) {
        return sorted[lower];
    }
    
    double weight = index - lower;
    return sorted[lower] * (1.0 - weight) + sorted[upper] * weight;
}

static double
find_min(double *values, int n)
{
    double min = values[0];
    for (int i = 1; i < n; i++) {
        if (values[i] < min) {
            min = values[i];
        }
    }
    return min;
}

static double
find_max(double *values, int n)
{
    double max = values[0];
    for (int i = 1; i < n; i++) {
        if (values[i] > max) {
            max = values[i];
        }
    }
    return max;
}

/* ---------- Function pointer type and selector ---------- */
typedef void (*vector_mul_func_t)(const float *, const float *, float *, size_t);

static vector_mul_func_t
get_function_for_level(simd_level_t level)
{
    switch (level) {
    case SIMD_AVX512F: return vector_mul_f32_avx512f;
    case SIMD_AVX2:    return vector_mul_f32_avx2;
    case SIMD_AVX:     return vector_mul_f32_avx;
    case SIMD_SSE4_2:  return vector_mul_f32_sse42;
    case SIMD_SSE2:    return vector_mul_f32_sse2;
    case SIMD_SCALAR:
    default:           return vector_mul_f32_scalar;
    }
}

/* ---------- benchmark a single array size ---------- */
static void
benchmark_size(size_t n, int csv_mode, simd_level_t lvl, vector_mul_func_t func)
{
    const size_t bytes = n * sizeof(float);
    const int num_trials = 10;
    
    // Adjust iterations based on array size
    int iters;
    if (n < 100000) {
        iters = 5000;
    } else if (n < 2000000) {
        iters = 2000;
    } else if (n < 5000000) {
        iters = 1000;
    } else {
        iters = 500;  // For very large arrays (5M+)
    }

    float *a   = aligned_alloc(64, bytes);
    float *b   = aligned_alloc(64, bytes);
    float *out = aligned_alloc(64, bytes);

    if (!a || !b || !out) {
        fprintf(stderr, "alloc failed for n=%zu\n", n);
        return;
    }

    // Initialize arrays
    for (size_t i = 0; i < n; i++) {
        a[i] = (float)i * 0.5f;
        b[i] = (float)i * 0.25f + 1.0f;
    }

    // Warmup
    for (int w = 0; w < 10; w++)
        func(a, b, out, n);

    // Run multiple trials
    double times_ms[num_trials];
    for (int trial = 0; trial < num_trials; trial++) {
        double t0 = now_sec();
        for (int i = 0; i < iters; i++) {
            func(a, b, out, n);
        }
        double t1 = now_sec();
        
        double elapsed = t1 - t0;
        double elapsed_ms = elapsed * 1000.0;
        times_ms[trial] = elapsed_ms / (double)iters;
    }

    // Calculate statistics
    double median_ms = calculate_median(times_ms, num_trials);
    double mean_ms = calculate_mean(times_ms, num_trials);
    double stddev_ms = calculate_stddev(times_ms, num_trials, mean_ms);
    double min_ms = find_min(times_ms, num_trials);
    double max_ms = find_max(times_ms, num_trials);
    double p99_ms = calculate_percentile(times_ms, num_trials, 0.99);
    
    // Calculate GFLOP/s using median (more robust)
    double ops = (double)iters * (double)n;
    double gflops = ops / (median_ms / 1000.0) / 1e9;

    // Correctness check (only for non-CSV mode)
    if (!csv_mode) {
        int bad = 0;
        for (size_t i = 0; i < 16 && i < n; i++) {
            float expect = a[i] * b[i];
            if (out[i] != expect) {
                printf("mismatch at %zu: got %f, expect %f\n", i, out[i], expect);
                bad = 1;
            }
        }
        if (!bad && n >= 16)
            printf("  correctness: OK\n");
    }

    // Output results
    if (csv_mode) {
        // CSV format: array_size,median_ms,mean_ms,stddev_ms,min_ms,max_ms,p99_ms,gflops,simd_level
        printf("%zu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.4f,%s\n", 
               n, median_ms, mean_ms, stddev_ms, min_ms, max_ms, p99_ms, gflops, simd_level_name(lvl));
    } else {
        printf("  n = %zu, iters = %d, trials = %d\n", n, iters, num_trials);
        printf("  median = %.6f ms, mean = %.6f ms\n", median_ms, mean_ms);
        printf("  stddev = %.6f ms, min = %.6f ms, max = %.6f ms\n", stddev_ms, min_ms, max_ms);
        printf("  p99 = %.6f ms\n", p99_ms);
        printf("  GFLOP/s = %.4f (based on median)\n", gflops);
    }

    free(a);
    free(b);
    free(out);
}

int
main(int argc, char **argv)
{
    // Parse command line arguments
    int csv_mode = 0;
    simd_level_t forced_level = SIMD_SCALAR;
    int use_forced_level = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--csv") == 0) {
            csv_mode = 1;
        } else if (strcmp(argv[i], "--force-level") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --force-level requires an argument\n");
                return 1;
            }
            i++;
            const char *level_str = argv[i];
            
            if (strcmp(level_str, "scalar") == 0) {
                forced_level = SIMD_SCALAR;
            } else if (strcmp(level_str, "sse2") == 0) {
                forced_level = SIMD_SSE2;
            } else if (strcmp(level_str, "sse4.2") == 0) {
                forced_level = SIMD_SSE4_2;
            } else if (strcmp(level_str, "avx") == 0) {
                forced_level = SIMD_AVX;
            } else if (strcmp(level_str, "avx2") == 0) {
                forced_level = SIMD_AVX2;
            } else if (strcmp(level_str, "avx512f") == 0) {
                forced_level = SIMD_AVX512F;
            } else {
                fprintf(stderr, "Error: Unknown SIMD level '%s'\n", level_str);
                fprintf(stderr, "Valid levels: scalar, sse2, sse4.2, avx, avx2, avx512f\n");
                return 1;
            }
            use_forced_level = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("\nOptions:\n");
            printf("  --csv              Output results in CSV format to stdout\n");
            printf("                     Format: array_size,median_ms,mean_ms,stddev_ms,min_ms,max_ms,p99_ms,gflops,simd_level\n");
            printf("  --force-level LVL  Force specific SIMD level instead of auto-detection\n");
            printf("                     Valid levels: scalar, sse2, sse4.2, avx, avx2, avx512f\n");
            printf("  --help, -h         Show this help message\n");
            printf("\nExamples:\n");
            printf("  %s                              # Human-readable output with auto-detect\n", argv[0]);
            printf("  %s --csv --force-level avx2     # CSV output using AVX2\n", argv[0]);
            printf("  %s --force-level scalar         # Test scalar implementation\n", argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            fprintf(stderr, "Use --help for usage information\n");
            return 1;
        }
    }

    simd_level_t lvl;
    if (use_forced_level) {
        lvl = forced_level;
    } else {
        lvl = detect_simd_level();
    }
    
    vector_mul_func_t func = get_function_for_level(lvl);
    
    if (!csv_mode) {
        printf("===========================================\n");
        printf("Vector Multiply Benchmark (Feature Compare)\n");
        printf("===========================================\n");
        if (use_forced_level) {
            printf("Forced SIMD level: %s\n", simd_level_name(lvl));
        } else {
            printf("Detected SIMD level: %s\n", simd_level_name(lvl));
        }
        printf("\n");
    } else {
        // CSV header
        printf("array_size,median_ms,mean_ms,stddev_ms,min_ms,max_ms,p99_ms,gflops,simd_level\n");
    }

    // Array sizes to test: comprehensive range from 512 to 4M elements
    const size_t sizes[] = {
        512,       // 512
        1024,      // 1K
        2048,      // 2K
        4096,      // 4K
        8192,      // 8K
        12288,     // 12K
        16384,     // 16K
        20480,     // 20K
        24576,     // 24K
        28672,     // 28K
        32768,     // 32K
        40960,     // 40K
        49152,     // 48K
        57344,     // 56K
        65536,     // 64K
        81920,     // 80K
        98304,     // 96K
        114688,    // 112K
        131072,    // 128K
        163840,    // 160K
        196608,    // 192K
        229376,    // 224K
        262144,    // 256K
        327680,    // 320K
        393216,    // 384K
        458752,    // 448K
        524288,    // 512K
        655360,    // 640K
        786432,    // 768K
        917504,    // 896K
        1048576,   // 1M
        1310720,   // 1.25M
        1572864,   // 1.5M
        1835008,   // 1.75M
        2097152,   // 2M
        2621440,   // 2.5M
        3145728,   // 3M
        3670016,   // 3.5M
        4194304,   // 4M
    };
    const int num_sizes = sizeof(sizes) / sizeof(sizes[0]);

    // Benchmark each size
    for (int i = 0; i < num_sizes; i++) {
        if (!csv_mode) {
            printf("\n--- Benchmarking size: %zu elements ---\n", sizes[i]);
        }
        benchmark_size(sizes[i], csv_mode, lvl, func);
    }

    if (!csv_mode) {
        printf("\n===========================================\n");
        printf("Benchmark complete!\n");
        printf("===========================================\n");
    }

    return 0;
}
