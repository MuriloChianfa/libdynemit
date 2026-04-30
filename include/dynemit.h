/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_H
#define DYNEMIT_H

/**
 * Dynemit - Dynamic SIMD Dispatch Library
 * 
 * This is the main umbrella header for the Dynemit library.
 * Include this header to get access to all features and CPU detection APIs.
 */

#ifdef __cplusplus
extern "C" {
#endif

// Core CPU detection and SIMD level detection
#include <dynemit/core.h>

// Error handling utilities for IFUNC resolvers
#include <dynemit/err.h>

// Element-wise vector operations
#include <dynemit/add.h>
#include <dynemit/mul.h>
#include <dynemit/sub.h>

// Statistical reduction and moment primitives
#include <dynemit/stats.h>

// Shannon entropy
#include <dynemit/entropy.h>

// Diversity and concentration indices
#include <dynemit/simpson.h>
#include <dynemit/hhi.h>
#include <dynemit/gini.h>

// Histogram-based range counting
#include <dynemit/histogram.h>

// Concentration analysis (top-K ratios, Hill estimator, composite)
#include <dynemit/topk.h>
#include <dynemit/hill.h>
#include <dynemit/concentration.h>

// Cardinality estimation
#include <dynemit/hll.h>

#ifdef __cplusplus
}
#endif

#endif // DYNEMIT_H
