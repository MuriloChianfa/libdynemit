#include <bench_utils.h>
#include <dynemit/skewness.h>

static void
bench_size(size_t n, int csv, simd_level_t lvl, skewness_f64_fn_t func)
{
    const int trials = 10;
    int iters = bench_iters_for_size(n);
    double *data = aligned_alloc(64, n * sizeof(double));
    if (!data) return;

    for (size_t i = 0; i < n; i++)
        data[i] = (double)i * 0.5 + 1.0;
    for (int w = 0; w < 10; w++) func(data, n);

    double times[10];
    for (int t = 0; t < trials; t++) {
        double t0 = bench_now_sec();
        for (int i = 0; i < iters; i++) func(data, n);
        times[t] = (bench_now_sec() - t0) * 1000.0 / iters;
    }

    bench_stats_t s;
    bench_compute_stats(times, trials, &s);
    double gflops = (double)n / (s.median_ms / 1000.0) / 1e9;

    if (csv) bench_csv_row(n, &s, gflops, lvl);
    else     bench_human_row(n, iters, trials, &s, gflops);

    free(data);
}

int main(int argc, char **argv)
{
    bench_opts_t opts;
    int rc = bench_parse_opts(argc, argv, &opts, "Skewness (f64)");
    if (rc != 0) return rc < 0 ? 1 : 0;

    simd_level_t lvl = opts.force_level_set ? opts.force_level : detect_simd_level();
    skewness_f64_fn_t func = skewness_f64_select(lvl);

    if (opts.auto_detect && bench_auto_detect_open("skewness", lvl) != 0) return 1;
    if (opts.csv) bench_csv_header();
    else          bench_print_header("Skewness (f64)", lvl, opts.force_level_set);

    for (int i = 0; i < BENCH_NUM_SIZES; i++) {
        if (!opts.csv)
            printf("\n--- size: %zu ---\n", BENCH_SIZES[i]);
        bench_size(BENCH_SIZES[i], opts.csv, lvl, func);
    }

    if (!opts.csv) bench_print_footer();
    if (opts.auto_detect) bench_auto_detect_close();
    return 0;
}
