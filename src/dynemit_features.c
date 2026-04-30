/* SPDX-License-Identifier: BSL-1.0 */
#include <stddef.h>

const char **
dynemit_features(void)
{
    static const char *features[] = {
        "core",
        "add",
        "mul",
        "sub",
        "sum",
        "mean",
        "min",
        "max",
        "variance",
        "skewness",
        "kurtosis",
        "entropy",
        "simpson",
        "hhi",
        "gini",
        "histogram",
        "topk",
        "hill",
        "concentration",
        "hll",
        nullptr
    };
    return features;
}
