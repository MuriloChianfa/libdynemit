/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_HLL_BIAS_H
#define DYNEMIT_HLL_BIAS_H

/*
 * HLL estimator constants.
 *
 * The original HLL++ paper (Heule et al. 2013) fixes classic-HLL mid-
 * range bias with two lookup tables (rawEstimateData[p] / biasData[p])
 * and a LinearCounting threshold[p].  Otmar Ertl's "New cardinality
 * estimation algorithms for HyperLogLog sketches" (2017) replaces all
 * of that with a tableless closed-form estimator based on the sigma/tau
 * functions.  That is what we use (via hll_estimate_from_histogram() in
 * hll.h) - it is strictly more accurate than HLL++ bias tables and has
 * no per-precision data to embed.
 *
 * The only constant the estimator still needs is alpha_infinity - the
 * limit of Flajolet's alpha_m as m -> infinity.  Ertl showed this single
 * value works for all m once the tau/sigma terms are included, so no
 * per-m switch is needed.
 */

/* alpha_infinity = 1 / (2 * ln 2) */
#define HLL_ALPHA_INF 0.72134752044448170367

#endif /* DYNEMIT_HLL_BIAS_H */
