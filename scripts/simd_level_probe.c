/* SPDX-License-Identifier: BSL-1.0 */
/*
 * Print detect_simd_level() and exit non-zero when below a required floor.
 * Used in CI to verify SDE/QEMU exposes the expected ISA before coverage runs.
 *
 * Usage: simd_level_probe MIN_LEVEL
 *   MIN_LEVEL is the integer simd_level_t value (see dynemit/core.h).
 */
#include <stdio.h>
#include <stdlib.h>

#include <dynemit/core.h>

int
main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "usage: %s MIN_SIMD_LEVEL\n", argv[0]);
        return 2;
    }

    char *end = nullptr;
    long required = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end != '\0' || required < 0) {
        fprintf(stderr, "invalid MIN_SIMD_LEVEL: %s\n", argv[1]);
        return 2;
    }

    simd_level_t level = detect_simd_level();
    printf("detect_simd_level=%d (%s)\n", (int)level, simd_level_name(level));

    if (level < (simd_level_t)required) {
        fprintf(stderr,
                "ERROR: detected level %d (%s) is below required %ld\n",
                (int)level, simd_level_name(level), required);
        return 1;
    }

    return 0;
}
