/* SPDX-License-Identifier: BSL-1.0 */
#ifndef BENCH_UTILS_H
#define BENCH_UTILS_H

/*
 * Shared benchmark infrastructure for dynemit feature benchmarks.
 * Header-only: include in each feature benchmark .c file.
 */

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
#include <sys/stat.h>
#include <errno.h>
#include <dynemit/core.h>

/* ---- Architecture string ---- */

static inline const char *
bench_get_arch(void)
{
#if defined(__x86_64__) || defined(__i386__)
    return "x86_64";
#elif defined(__aarch64__)
    return "aarch64";
#else
    return "unknown";
#endif
}

/* ---- Recursive directory creation ---- */

static int
bench_mkdir_p(const char *path)
{
    char tmp[512];
    size_t len = strlen(path);
    if (len >= sizeof(tmp)) return -1;
    memcpy(tmp, path, len + 1);

    for (size_t i = 1; i < len; i++) {
        if (tmp[i] == '/') {
            tmp[i] = '\0';
            if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
                return -1;
            tmp[i] = '/';
        }
    }
    if (mkdir(tmp, 0755) != 0 && errno != EEXIST)
        return -1;
    return 0;
}

/* ---- Timing ---- */

static inline double
bench_now_sec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec * 1e-9;
}

/* ---- Statistics ---- */

static int
bench_compare_double(const void *a, const void *b)
{
    double da = *(const double *)a;
    double db = *(const double *)b;
    if (da < db) return -1;
    if (da > db) return 1;
    return 0;
}

static double
bench_median(double *values, int n)
{
    double sorted[n];
    memcpy(sorted, values, (size_t)n * sizeof(double));
    qsort(sorted, (size_t)n, sizeof(double), bench_compare_double);
    if (n % 2 == 0)
        return (sorted[n/2 - 1] + sorted[n/2]) / 2.0;
    return sorted[n/2];
}

static double
bench_mean(double *values, int n)
{
    double sum = 0.0;
    for (int i = 0; i < n; i++)
        sum += values[i];
    return sum / (double)n;
}

static double
bench_stddev(double *values, int n, double mean)
{
    double ssq = 0.0;
    for (int i = 0; i < n; i++) {
        double d = values[i] - mean;
        ssq += d * d;
    }
    return sqrt(ssq / (double)n);
}

static double
bench_percentile(double *values, int n, double pct)
{
    double sorted[n];
    memcpy(sorted, values, (size_t)n * sizeof(double));
    qsort(sorted, (size_t)n, sizeof(double), bench_compare_double);
    double idx = pct * (n - 1);
    int lo = (int)floor(idx);
    int hi = (int)ceil(idx);
    if (lo == hi) return sorted[lo];
    double w = idx - lo;
    return sorted[lo] * (1.0 - w) + sorted[hi] * w;
}

static double
bench_min(double *values, int n)
{
    double m = values[0];
    for (int i = 1; i < n; i++)
        if (values[i] < m) m = values[i];
    return m;
}

static double
bench_max(double *values, int n)
{
    double m = values[0];
    for (int i = 1; i < n; i++)
        if (values[i] > m) m = values[i];
    return m;
}

/* ---- Statistics result ---- */

typedef struct {
    double median_ms;
    double mean_ms;
    double stddev_ms;
    double min_ms;
    double max_ms;
    double p99_ms;
} bench_stats_t;

static void
bench_compute_stats(double *times_ms, int n, bench_stats_t *out)
{
    out->median_ms = bench_median(times_ms, n);
    out->mean_ms   = bench_mean(times_ms, n);
    out->stddev_ms = bench_stddev(times_ms, n, out->mean_ms);
    out->min_ms    = bench_min(times_ms, n);
    out->max_ms    = bench_max(times_ms, n);
    out->p99_ms    = bench_percentile(times_ms, n, 0.99);
}

/* ---- Iteration count heuristic ---- */

static int
bench_iters_for_size(size_t n)
{
    if (n < 100000)  return 5000;
    if (n < 2000000) return 2000;
    if (n < 5000000) return 1000;
    return 500;
}

/* ---- Trials per data point ---- */

#define BENCH_TRIALS 3

/* ---- Standard array sizes ---- */

static const size_t BENCH_SIZES[] = {
    512, 1024, 2048, 4096, 8192, 12288, 16384, 20480, 24576, 28672,
    32768, 40960, 49152, 57344, 65536,
};
#define BENCH_NUM_SIZES ((int)(sizeof(BENCH_SIZES) / sizeof(BENCH_SIZES[0])))

/* ---- CSV output ---- */

static void
bench_csv_header(void)
{
    printf("array_size,median_ms,mean_ms,stddev_ms,min_ms,max_ms,"
           "p99_ms,gflops,simd_level\n");
}

static void
bench_csv_row(size_t n, const bench_stats_t *s, double gflops,
              simd_level_t lvl)
{
    printf("%zu,%.6f,%.6f,%.6f,%.6f,%.6f,%.6f,%.4f,%s\n",
           n, s->median_ms, s->mean_ms, s->stddev_ms,
           s->min_ms, s->max_ms, s->p99_ms, gflops,
           simd_level_name(lvl));
}

static void
bench_human_row(size_t n, int iters, int trials,
                const bench_stats_t *s, double gflops)
{
    printf("  n = %zu, iters = %d, trials = %d\n", n, iters, trials);
    printf("  median = %.6f ms, mean = %.6f ms\n",
           s->median_ms, s->mean_ms);
    printf("  stddev = %.6f ms, min = %.6f ms, max = %.6f ms\n",
           s->stddev_ms, s->min_ms, s->max_ms);
    printf("  p99 = %.6f ms\n", s->p99_ms);
    printf("  GFLOP/s = %.4f (based on median)\n", gflops);
}

/* ---- CPU model detection ---- */

typedef struct {
    unsigned impl;
    unsigned part;
    const char *name;
} bench_arm_cpu_t;

static const bench_arm_cpu_t bench_arm_cpus[] = {
    /* ARM Ltd (0x41) */
    { 0x41, 0xd4f, "arm_neoverse_v2" },
    { 0x41, 0xd49, "arm_neoverse_n2" },
    { 0x41, 0xd40, "arm_neoverse_v1" },
    { 0x41, 0xd0c, "arm_neoverse_n1" },
    { 0x41, 0xd0b, "arm_cortex_a76"  },
    { 0x41, 0xd05, "arm_cortex_a55"  },
    { 0x41, 0xd07, "arm_cortex_a57"  },
    { 0x41, 0xd08, "arm_cortex_a72"  },
    { 0x41, 0xd09, "arm_cortex_a73"  },
    { 0x41, 0xd0a, "arm_cortex_a75"  },
    { 0x41, 0xd0d, "arm_cortex_a77"  },
    { 0x41, 0xd41, "arm_cortex_a78"  },
    { 0x41, 0xd44, "arm_cortex_x1"   },
    /* Apple (0x61) */
    { 0x61, 0x022, "apple_m1_icestorm"  },
    { 0x61, 0x023, "apple_m1_firestorm" },
    { 0x61, 0x032, "apple_m2_blizzard"  },
    { 0x61, 0x033, "apple_m2_avalanche" },
    { 0, 0, NULL }
};

#if defined(__aarch64__)
static void
bench_get_cpu_model_arm(char *buffer, size_t bufsize)
{
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) { snprintf(buffer, bufsize, "unknown_arm_cpu"); return; }

    char line[256];
    unsigned impl = 0, part = 0;
    int have_impl = 0, have_part = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (!have_impl && strncmp(line, "CPU implementer", 15) == 0) {
            char *colon = strchr(line, ':');
            if (colon) { impl = (unsigned)strtoul(colon + 1, NULL, 0); have_impl = 1; }
        }
        if (!have_part && strncmp(line, "CPU part", 8) == 0) {
            char *colon = strchr(line, ':');
            if (colon) { part = (unsigned)strtoul(colon + 1, NULL, 0); have_part = 1; }
        }
        if (have_impl && have_part) break;
    }
    fclose(fp);

    if (have_impl && have_part) {
        for (int i = 0; bench_arm_cpus[i].name; i++) {
            if (bench_arm_cpus[i].impl == impl && bench_arm_cpus[i].part == part) {
                snprintf(buffer, bufsize, "%s", bench_arm_cpus[i].name);
                return;
            }
        }
        snprintf(buffer, bufsize, "arm_%02x_%03x", impl, part);
        return;
    }
    snprintf(buffer, bufsize, "unknown_arm_cpu");
}
#endif

static void
bench_get_cpu_model(char *buffer, size_t bufsize)
{
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) { snprintf(buffer, bufsize, "unknown_cpu"); return; }

    char line[256];
    int found = 0;
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "model name", 10) != 0) continue;
        char *colon = strchr(line, ':');
        if (!colon) continue;
        colon++;
        while (*colon == ' ' || *colon == '\t') colon++;
        size_t len = strlen(colon);
        if (len > 0 && colon[len-1] == '\n') colon[len-1] = '\0';

        char temp[256];
        strncpy(temp, colon, sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';

        char *patterns[] = {"-core", " core", "processor", NULL};
        for (int p = 0; patterns[p]; p++) {
            char *pos = strcasestr(temp, patterns[p]);
            if (pos) {
                char *ns = pos;
                while (ns > temp && (isdigit(*(ns-1)) || *(ns-1)==' ' || *(ns-1)=='-'))
                    ns--;
                char *pe = pos + strlen(patterns[p]);
                while (*pe == ' ') pe++;
                memmove(ns, pe, strlen(pe) + 1);
            }
        }

        size_t j = 0;
        for (size_t i = 0; temp[i] && j < bufsize - 1; i++) {
            char c = temp[i];
            if (isalnum(c))
                buffer[j++] = (char)tolower(c);
            else if (c==' '||c=='-'||c=='('||c==')'||c=='@')
                if (j > 0 && buffer[j-1] != '_') buffer[j++] = '_';
        }
        while (j > 0 && buffer[j-1] == '_') j--;
        buffer[j] = '\0';
        found = 1;
        break;
    }
    fclose(fp);

    if (!found) {
#if defined(__aarch64__)
        bench_get_cpu_model_arm(buffer, bufsize);
#else
        snprintf(buffer, bufsize, "unknown_cpu");
#endif
    }
}

/* ---- CLI argument parsing ---- */

typedef struct {
    int csv;
    int auto_detect;
    int force_level_set;
    simd_level_t force_level;
} bench_opts_t;

static int
bench_parse_level(const char *s, simd_level_t *out)
{
    if (strcmp(s, "scalar")  == 0) { *out = SIMD_SCALAR;  return 0; }
    if (strcmp(s, "sse2")    == 0) { *out = SIMD_SSE2;    return 0; }
    if (strcmp(s, "sse4.2")  == 0) { *out = SIMD_SSE4_2;  return 0; }
    if (strcmp(s, "avx")     == 0) { *out = SIMD_AVX;     return 0; }
    if (strcmp(s, "avx2")    == 0) { *out = SIMD_AVX2;    return 0; }
    if (strcmp(s, "avx512f") == 0) { *out = SIMD_AVX512F; return 0; }
    if (strcmp(s, "neon")    == 0) { *out = SIMD_NEON;    return 0; }
    if (strcmp(s, "sve")     == 0) { *out = SIMD_SVE;     return 0; }
    if (strcmp(s, "sve2")    == 0) { *out = SIMD_SVE2;    return 0; }
    return -1;
}

static int
bench_parse_opts(int argc, char **argv, bench_opts_t *opts,
                 const char *bench_name)
{
    memset(opts, 0, sizeof(*opts));
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--csv") == 0) {
            opts->csv = 1;
        } else if (strcmp(argv[i], "--auto-detect") == 0) {
            opts->csv = 1;
            opts->auto_detect = 1;
        } else if (strcmp(argv[i], "--force-level") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --force-level requires an argument\n");
                return -1;
            }
            i++;
            if (bench_parse_level(argv[i], &opts->force_level) != 0) {
                fprintf(stderr, "Error: Unknown SIMD level '%s'\n", argv[i]);
                fprintf(stderr, "Valid: scalar, sse2, sse4.2, avx, avx2, avx512f, neon, sve, sve2\n");
                return -1;
            }
            opts->force_level_set = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [OPTIONS]\n", argv[0]);
            printf("\n%s Benchmark\n\n", bench_name);
            printf("Options:\n");
            printf("  --csv              CSV output to stdout\n");
            printf("  --force-level LVL  Force SIMD level\n"
                   "                     x86: scalar, sse2, sse4.2, avx, avx2, avx512f\n"
                   "                     ARM: scalar, neon, sve, sve2\n");
            printf("  --auto-detect      Auto-detect CPU, write CSV to file\n");
            printf("  --help, -h         Show this help\n");
            return 1;
        } else {
            fprintf(stderr, "Unknown option: %s (use --help)\n", argv[i]);
            return -1;
        }
    }
    return 0;
}

/* ---- Auto-detect file output ---- */

static FILE *bench_saved_stdout;
static char  bench_auto_filename[512];

static int
bench_auto_detect_open(const char *feature, simd_level_t lvl)
{
    const char *arch = bench_get_arch();
    char cpu[256];
    bench_get_cpu_model(cpu, sizeof(cpu));
    if (strlen(cpu) > 80) cpu[80] = '\0';

    const char *ss = simd_level_name(lvl);
    char sl[32];
    size_t j = 0;
    for (size_t i = 0; ss[i] && j < sizeof(sl)-1; i++) {
        char c = ss[i];
        if (isalnum(c)) sl[j++] = (char)tolower(c);
        else if ((c=='-'||c=='.') && j>0 && sl[j-1]!='_') sl[j++] = '_';
    }
    sl[j] = '\0';

    char dir[512];
    snprintf(dir, sizeof(dir), "bench/cpus/%s/%s/data", arch, cpu);
    if (bench_mkdir_p(dir) != 0) {
        fprintf(stderr, "Error: cannot create directory '%s'\n", dir);
        return -1;
    }

    snprintf(bench_auto_filename, sizeof(bench_auto_filename),
             "%s/%s_%s.csv", dir, feature, sl);

    bench_saved_stdout = stdout;
    stdout = fopen(bench_auto_filename, "w");
    if (!stdout) {
        fprintf(stderr, "Error: cannot create '%s'\n", bench_auto_filename);
        stdout = bench_saved_stdout;
        return -1;
    }
    fprintf(stderr, "SIMD level: %s\n", simd_level_name(lvl));
    fprintf(stderr, "Writing results to: %s\n", bench_auto_filename);
    return 0;
}

static void
bench_auto_detect_close(void)
{
    if (bench_saved_stdout) {
        fclose(stdout);
        stdout = bench_saved_stdout;
        bench_saved_stdout = NULL;
        fprintf(stderr, "Results saved to: %s\n", bench_auto_filename);
    }
}

/* ---- Benchmark header/footer ---- */

static void
bench_print_header(const char *name, simd_level_t lvl, int forced)
{
    printf("===========================================\n");
    printf("%s Benchmark\n", name);
    printf("===========================================\n");
    if (forced)
        printf("Forced SIMD level: %s\n", simd_level_name(lvl));
    else
        printf("Detected SIMD level: %s\n", simd_level_name(lvl));
    printf("\n");
}

static void
bench_print_footer(void)
{
    printf("\n===========================================\n");
    printf("Benchmark complete!\n");
    printf("===========================================\n");
}

#endif /* BENCH_UTILS_H */
