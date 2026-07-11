/* SPDX-License-Identifier: BSL-1.0 */
#include <dynemit.h>
#include <stdio.h>

int main(void)
{
    simd_level_t level = detect_simd_level();
    printf("libdynemit consumer smoke (C): simd_level=%d\n", (int)level);
    return 0;
}
