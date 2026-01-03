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
#include <dynemit/core.h>
#include <dynemit/vector_mul.h>

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

/* ---------- generate auto filename ---------- */
static void
generate_auto_filename(char *buffer, size_t bufsize, simd_level_t level)
{
    char cpu_model[256];
    get_cpu_model_name(cpu_model, sizeof(cpu_model));
    
    // Shorten very long CPU names (keep first 80 chars)
    if (strlen(cpu_model) > 80) {
        cpu_model[80] = '\0';
    }
    
    const char *simd_str = simd_level_name(level);
    char simd_lower[32];
    
    // Convert SIMD level to lowercase and remove special chars
    size_t j = 0;
    for (size_t i = 0; simd_str[i] && j < sizeof(simd_lower) - 1; i++) {
        char c = simd_str[i];
        if (isalnum(c)) {
            simd_lower[j++] = tolower(c);
        } else if (c == '-' || c == '.') {
            if (j > 0 && simd_lower[j-1] != '_') {
                simd_lower[j++] = '_';
            }
        }
    }
    simd_lower[j] = '\0';
    
    snprintf(buffer, bufsize, "bench/data/results_%s_%s.csv", cpu_model, simd_lower);
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

/* ---------- benchmark a single array size ---------- */
static void
benchmark_size(size_t n, int csv_mode, simd_level_t lvl)
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
        vector_mul_f32(a, b, out, n);

    // Run multiple trials
    double times_ms[num_trials];
    for (int trial = 0; trial < num_trials; trial++) {
        double t0 = now_sec();
        for (int i = 0; i < iters; i++) {
            vector_mul_f32(a, b, out, n);
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
    int auto_detect = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--csv") == 0) {
            csv_mode = 1;
        } else if (strcmp(argv[i], "--auto-detect") == 0) {
            csv_mode = 1;
            auto_detect = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("\nOptions:\n");
            printf("  --csv          Output results in CSV format to stdout\n");
            printf("                 Format: array_size,time_ms,gflops,simd_level\n");
            printf("  --auto-detect  Auto-detect CPU and SIMD level, write CSV to file\n");
            printf("                 Filename format: results_<cpu_model>_<simd_level>.csv\n");
            printf("  --help, -h     Show this help message\n");
            printf("\nExamples:\n");
            printf("  %s                    # Human-readable output\n", argv[0]);
            printf("  %s --csv > out.csv    # CSV to stdout\n", argv[0]);
            printf("  %s --auto-detect      # Auto-generate filename\n", argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            fprintf(stderr, "Use --help for usage information\n");
            return 1;
        }
    }

    simd_level_t lvl = detect_simd_level();
    
    // Handle auto-detect mode: redirect stdout to file
    FILE *original_stdout = NULL;
    char auto_filename[512];
    
    if (auto_detect) {
        generate_auto_filename(auto_filename, sizeof(auto_filename), lvl);
        
        // Save original stdout
        original_stdout = stdout;
        
        // Open file for writing
        stdout = fopen(auto_filename, "w");
        if (!stdout) {
            fprintf(stderr, "Error: Could not create file '%s'\n", auto_filename);
            stdout = original_stdout;
            return 1;
        }
        
        // Print to stderr so user sees what's happening
        fprintf(stderr, "Auto-detected CPU and SIMD level\n");
        fprintf(stderr, "SIMD level: %s\n", simd_level_name(lvl));
        fprintf(stderr, "Writing results to: %s\n", auto_filename);
    }
    
    if (!csv_mode) {
        printf("===========================================\n");
        printf("Vector Multiply Benchmark\n");
        printf("===========================================\n");
        printf("Detected SIMD level: %s\n", simd_level_name(lvl));
        printf("(this is the version the ifunc dispatcher will pick)\n");
        printf("\n");
    } else {
        // CSV header
        printf("array_size,median_ms,mean_ms,stddev_ms,min_ms,max_ms,p99_ms,gflops,simd_level\n");
    }

    // Array sizes to test: comprehensive range from 512 to 16M elements
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
        benchmark_size(sizes[i], csv_mode, lvl);
    }

    if (!csv_mode) {
        printf("\n===========================================\n");
        printf("Benchmark complete!\n");
        printf("To generate CSV output: %s --csv\n", argv[0]);
        printf("To auto-detect and save: %s --auto-detect\n", argv[0]);
        printf("===========================================\n");
    }
    
    // Close file and restore stdout if in auto-detect mode
    if (auto_detect && original_stdout) {
        fclose(stdout);
        stdout = original_stdout;
        fprintf(stderr, "Benchmark complete! Results saved to: %s\n", auto_filename);
    }

    return 0;
}


