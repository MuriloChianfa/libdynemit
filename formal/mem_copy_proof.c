/* SPDX-License-Identifier: BSL-1.0 */
/*
 * Contract proofs for memsets / memcpys. Huge destsz is only used on
 * paths that return before touching a buffer.
 */

#include "cprover.h"
#include "mem.h"

enum { MEM_COPY_BOUND = 16 };

static void
fill_nondet(unsigned char *buf, size_t n)
{
    for (size_t i = 0; i < n; i++)
        buf[i] = nondet_uchar();
}

static void
proof_memsets_null(void)
{
    size_t destsz = nondet_size_t();
    size_t count = nondet_size_t();
    int value = nondet_int();

    __CPROVER_assert(memsets(NULL, destsz, value, 0) == 0,
                     "memsets(NULL, *, *, 0) succeeds");
    __CPROVER_assume(count > 0);
    __CPROVER_assert(memsets(NULL, destsz, value, count) == EINVAL,
                     "memsets(NULL, *, *, count>0) is EINVAL");
}

static void
proof_memsets_rsize(void)
{
    unsigned char dummy = nondet_uchar();
    size_t destsz = nondet_size_t();
    size_t count = nondet_size_t();
    int value = nondet_int();
    unsigned char before = dummy;

    __CPROVER_assume(destsz > RSIZE_MAX || count > RSIZE_MAX);
    __CPROVER_assume(destsz != 0); /* dest is a real 1-byte object */
    errno_t err = memsets(&dummy, destsz, value, count);
    __CPROVER_assert(err == EOVERFLOW, "memsets RSIZE overflow is EOVERFLOW");
    __CPROVER_assert(dummy == before, "memsets RSIZE overflow does not write");
}

static void
proof_memsets_bounded(void)
{
    unsigned char dest[MEM_COPY_BOUND];
    unsigned char orig[MEM_COPY_BOUND];
    size_t destsz = nondet_size_t();
    size_t count = nondet_size_t();
    unsigned char value = nondet_uchar();

    fill_nondet(dest, MEM_COPY_BOUND);

    __CPROVER_assume(destsz <= MEM_COPY_BOUND);
    __CPROVER_assume(count <= RSIZE_MAX);
    __CPROVER_assume(destsz <= RSIZE_MAX);

    for (int i = 0; i < MEM_COPY_BOUND; i++)
        orig[i] = dest[i];

    errno_t err = memsets(dest, destsz, (int)value, count);

    if (count > destsz) {
        __CPROVER_assert(err == EOVERFLOW, "memsets count>destsz is EOVERFLOW");
        for (size_t i = 0; i < destsz; i++)
            __CPROVER_assert(dest[i] == 0, "memsets overflow zeroes dest");
        for (size_t i = destsz; i < MEM_COPY_BOUND; i++)
            __CPROVER_assert(dest[i] == orig[i], "memsets overflow leaves tail");
    } else {
        __CPROVER_assert(err == 0, "memsets success");
        for (size_t i = 0; i < count; i++)
            __CPROVER_assert(dest[i] == value, "memsets writes value");
        for (size_t i = count; i < MEM_COPY_BOUND; i++)
            __CPROVER_assert(dest[i] == orig[i], "memsets leaves tail");
    }
}

static void
proof_memcpys_null_dest(void)
{
    size_t destsz = nondet_size_t();
    size_t count = nondet_size_t();
    unsigned char src[MEM_COPY_BOUND];

    fill_nondet(src, MEM_COPY_BOUND);

    __CPROVER_assert(memcpys(NULL, destsz, src, 0) == 0,
                     "memcpys(NULL, *, src, 0) succeeds");
    __CPROVER_assume(count > 0);
    __CPROVER_assert(memcpys(NULL, destsz, src, count) == EINVAL,
                     "memcpys(NULL, *, src, count>0) is EINVAL");
}

static void
proof_memcpys_null_src(void)
{
    unsigned char dest[MEM_COPY_BOUND];
    unsigned char orig[MEM_COPY_BOUND];
    size_t destsz = nondet_size_t();
    size_t count = nondet_size_t();

    fill_nondet(dest, MEM_COPY_BOUND);

    __CPROVER_assume(destsz <= MEM_COPY_BOUND);

    for (int i = 0; i < MEM_COPY_BOUND; i++)
        orig[i] = dest[i];

    errno_t err = memcpys(dest, destsz, NULL, count);

    if (destsz > 0) {
        for (size_t i = 0; i < destsz; i++)
            __CPROVER_assert(dest[i] == 0, "memcpys NULL src zeroes dest");
        for (size_t i = destsz; i < MEM_COPY_BOUND; i++)
            __CPROVER_assert(dest[i] == orig[i], "memcpys NULL src leaves tail");
    } else {
        for (int i = 0; i < MEM_COPY_BOUND; i++)
            __CPROVER_assert(dest[i] == orig[i], "memcpys NULL src destsz=0 no write");
    }

    if (count > 0)
        __CPROVER_assert(err == EINVAL, "memcpys NULL src count>0 is EINVAL");
    else
        __CPROVER_assert(err == 0, "memcpys NULL src count=0 succeeds");
}

static void
proof_memcpys_rsize(void)
{
    unsigned char dummy = nondet_uchar();
    unsigned char src[1];
    size_t destsz = nondet_size_t();
    size_t count = nondet_size_t();
    unsigned char before = dummy;

    src[0] = nondet_uchar();

    __CPROVER_assume(destsz > RSIZE_MAX || count > RSIZE_MAX);
    __CPROVER_assume(destsz != 0);
    errno_t err = memcpys(&dummy, destsz, src, count);
    __CPROVER_assert(err == EOVERFLOW, "memcpys RSIZE overflow is EOVERFLOW");
    __CPROVER_assert(dummy == before, "memcpys RSIZE overflow does not write");
}

static void
proof_memcpys_bounded(void)
{
    unsigned char dest[MEM_COPY_BOUND];
    unsigned char src[MEM_COPY_BOUND];
    unsigned char orig[MEM_COPY_BOUND];
    size_t destsz = nondet_size_t();
    size_t count = nondet_size_t();

    fill_nondet(dest, MEM_COPY_BOUND);
    fill_nondet(src, MEM_COPY_BOUND);

    __CPROVER_assume(destsz <= MEM_COPY_BOUND);
    __CPROVER_assume(count <= MEM_COPY_BOUND);
    __CPROVER_assume(destsz <= RSIZE_MAX);
    __CPROVER_assume(count <= RSIZE_MAX);

    for (int i = 0; i < MEM_COPY_BOUND; i++)
        orig[i] = dest[i];

    errno_t err = memcpys(dest, destsz, src, count);

    if (count > destsz) {
        __CPROVER_assert(err == EOVERFLOW, "memcpys count>destsz is EOVERFLOW");
        for (size_t i = 0; i < destsz; i++)
            __CPROVER_assert(dest[i] == 0, "memcpys overflow zeroes dest");
        for (size_t i = destsz; i < MEM_COPY_BOUND; i++)
            __CPROVER_assert(dest[i] == orig[i], "memcpys overflow leaves tail");
    } else {
        __CPROVER_assert(err == 0, "memcpys success");
        for (size_t i = 0; i < count; i++)
            __CPROVER_assert(dest[i] == src[i], "memcpys copies bytes");
        for (size_t i = count; i < MEM_COPY_BOUND; i++)
            __CPROVER_assert(dest[i] == orig[i], "memcpys leaves tail");
    }
}

int
main(void)
{
    proof_memsets_null();
    proof_memsets_rsize();
    proof_memsets_bounded();
    proof_memcpys_null_dest();
    proof_memcpys_null_src();
    proof_memcpys_rsize();
    proof_memcpys_bounded();
    return 0;
}
