#ifndef DYNEMIT_H
#define DYNEMIT_H

/**
 * Dynemit - Dynamic SIMD Dispatch Library
 * 
 * This is the main umbrella header for the Dynemit library.
 * Include this header to get access to all features and CPU detection APIs.
 */

// Core CPU detection and SIMD level detection
#include <dynemit/core.h>

// Features - automatically included when using the all-in-one library
#ifdef DYNEMIT_ALL_FEATURES
#include <dynemit/vector_add.h>
#include <dynemit/vector_mul.h>
#include <dynemit/vector_sub.h>
#endif

// Alternatively, users can include individual feature headers:
// #include <dynemit/vector_add.h>
// #include <dynemit/vector_mul.h>
// #include <dynemit/vector_sub.h>
// etc.

#endif // DYNEMIT_H

