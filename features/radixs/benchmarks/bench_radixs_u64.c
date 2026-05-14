#include <bench_utils.h>
#include <dynemit/radixs.h>

static void
bench_size(size_t n, int csv, simd_level_t lvl, radixs_u64_fn_t func)
{
    const int trials = BENCH_TRIALS;
    int iters = bench_iters_for_size(n);
    uint64_t *in  = aligned_alloc(64, n * sizeof(uint64_t));
    uint64_t *out = aligned_alloc(64, n * sizeof(uint64_t));
    if (!in || !out) { free(in); free(out); return; }

    uint64_t s = 0xDEADBEEFULL ^ (uint64_t)n;
    for (size_t i = 0; i < n; i++) {
        s ^= s << 13; s ^= s >> 7; s ^= s << 17;
        in[i] = s;
    }
    for (int w = 0; w < 5; w++) func(in, out, n);

    double times[BENCH_TRIALS];
    for (int t = 0; t < trials; t++) {
        double t0 = bench_now_sec();
        for (int i = 0; i < iters; i++) func(in, out, n);
        times[t] = (bench_now_sec() - t0) * 1000.0 / iters;
    }

    bench_stats_t st;
    bench_compute_stats(times, trials, &st);
    double gflops = (double)n / (st.median_ms / 1000.0) / 1e9;

    if (csv) bench_csv_row(n, &st, gflops, lvl);
    else     bench_human_row(n, iters, trials, &st, gflops);

    free(in); free(out);
}

int main(int argc, char **argv)
{
    bench_opts_t opts;
    int rc = bench_parse_opts(argc, argv, &opts, "RadixSort (u64)");
    if (rc != 0) return rc < 0 ? 1 : 0;

    simd_level_t lvl = opts.force_level_set ? opts.force_level : detect_simd_level();
    radixs_u64_fn_t func = radixs_u64_select(lvl);

    if (opts.auto_detect && bench_auto_detect_open("radixs_u64", lvl) != 0) return 1;
    if (opts.csv) bench_csv_header();
    else          bench_print_header("RadixSort (u64)", lvl, opts.force_level_set);

    for (int i = 0; i < BENCH_NUM_SIZES; i++) {
        if (!opts.csv)
            printf("\n--- size: %zu ---\n", BENCH_SIZES[i]);
        bench_size(BENCH_SIZES[i], opts.csv, lvl, func);
    }

    if (!opts.csv) bench_print_footer();
    if (opts.auto_detect) bench_auto_detect_close();
    return 0;
}
