/* SPDX-License-Identifier: BSL-1.0 */
#include <stddef.h>

// This file is only compiled for the all-in-one dynemit library
// It provides the complete feature list and overrides the weak default

const char **
dynemit_features(void)
{
    static const char *features[] = {
        "core",
        "vector_add",
        "vector_mul",
        "vector_sub",
        nullptr  // nullptr-terminated
    };
    return features;
}
