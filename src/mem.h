/* SPDX-License-Identifier: BSL-1.0 */
#ifndef DYNEMIT_MEM_H
#define DYNEMIT_MEM_H

#include <stddef.h>
#include <stdint.h>

#ifndef errno_t
typedef int errno_t;
#endif

#ifndef EINVAL
#define EINVAL 22
#endif

#ifndef EOVERFLOW
#define EOVERFLOW 75
#endif

#ifndef RSIZE_MAX
#define RSIZE_MAX (SIZE_MAX >> 1)
#endif

/*
 * Default alignment for aligned_alloc() scratch buffers (AVX-512 friendly).
 * ISO C11 requires the allocation size to be a multiple of alignment; glibc
 * and ASan treat violations as hard failures. Use mem_aligned_count() to
 * round element-array byte counts up before calling aligned_alloc().
 */
#ifndef DYNEMIT_MEM_ALIGN
#define DYNEMIT_MEM_ALIGN 64u
#endif

static inline size_t
mem_align_up(size_t size, size_t align)
{
    size_t mask = align - 1;
    return (size + mask) & ~mask;
}

static inline size_t
mem_aligned_bytes(size_t count, size_t elem_size)
{
    return mem_align_up(count * elem_size, DYNEMIT_MEM_ALIGN);
}

#define mem_aligned_count(n, type) \
    mem_aligned_bytes((size_t)(n), sizeof(type))

static inline errno_t
memsets(void *dest, size_t destsz, int value, size_t count)
{
    if (dest == NULL)
        return count > 0 ? EINVAL : 0;
    if (destsz > RSIZE_MAX || count > RSIZE_MAX)
        return EOVERFLOW;
    if (count > destsz) {
        __builtin_memset(dest, 0, destsz);
        return EOVERFLOW;
    }
    __builtin_memset(dest, value, count);
    return 0;
}

static inline errno_t
memcpys(void *restrict dest, size_t destsz,
        const void *restrict src, size_t count)
{
    if (dest == NULL || src == NULL) {
        if (dest != NULL && destsz > 0)
            __builtin_memset(dest, 0, destsz);
        return (dest == NULL || src == NULL) && count > 0 ? EINVAL : 0;
    }
    if (destsz > RSIZE_MAX || count > RSIZE_MAX)
        return EOVERFLOW;
    if (count > destsz) {
        __builtin_memset(dest, 0, destsz);
        return EOVERFLOW;
    }
    __builtin_memcpy(dest, src, count);
    return 0;
}

#endif /* DYNEMIT_MEM_H */
