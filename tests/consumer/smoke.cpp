/* SPDX-License-Identifier: BSL-1.0 */
#include <dynemit.h>
#include <cstdio>

int main()
{
    simd_level_t level = detect_simd_level();
    std::printf("libdynemit consumer smoke (C++): simd_level=%d\n",
                static_cast<int>(level));
    return 0;
}
