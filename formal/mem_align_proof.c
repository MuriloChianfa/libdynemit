/* SPDX-License-Identifier: BSL-1.0 */
/*
 * Bounded proofs for mem_align_up / mem_aligned_bytes.
 *
 * Overflow of size+mask and count*elem_size is in-scope: this proof
 * does not enable --unsigned-overflow-check so wrap-around can be
 * characterized rather than treated as an automatic failure.
 */

#include "cprover.h"
#include "mem.h"

static void
proof_align_no_wrap(void)
{
    size_t size = nondet_size_t();
    const size_t align = DYNEMIT_MEM_ALIGN;
    const size_t mask = align - 1;

    __CPROVER_assume(size <= SIZE_MAX - mask);

    size_t r = mem_align_up(size, align);
    __CPROVER_assert(r >= size, "align_up >= size when no wrap");
    __CPROVER_assert((r & mask) == 0, "align_up is a multiple of align");
    __CPROVER_assert(r - size < align, "align_up slack is in [0, align)");
}

static void
proof_align_wrap(void)
{
    size_t size = nondet_size_t();
    const size_t align = DYNEMIT_MEM_ALIGN;
    const size_t mask = align - 1;

    __CPROVER_assume(size > SIZE_MAX - mask);

    size_t r = mem_align_up(size, align);
    /* Current implementation wraps modulo 2^word. */
    __CPROVER_assert(r < size, "align_up wrap yields r < size");
    __CPROVER_assert((r & mask) == 0, "align_up remains aligned after wrap");
}

static void
proof_aligned_bytes_no_wrap(void)
{
    size_t count = nondet_size_t();
    size_t elem_size = nondet_size_t();

    __CPROVER_assume(elem_size == sizeof(uint32_t) || elem_size == sizeof(uint64_t));
    __CPROVER_assume(count <= SIZE_MAX / elem_size);

    size_t raw = count * elem_size;
    __CPROVER_assume(raw <= SIZE_MAX - (DYNEMIT_MEM_ALIGN - 1));

    size_t a = mem_aligned_bytes(count, elem_size);
    __CPROVER_assert(a >= raw, "aligned_bytes >= raw when no wrap");
    __CPROVER_assert((a % DYNEMIT_MEM_ALIGN) == 0, "aligned_bytes is 64-aligned");
    __CPROVER_assert(a - raw < DYNEMIT_MEM_ALIGN, "aligned_bytes slack < 64");
}

static void
proof_aligned_bytes_mul_wrap(void)
{
    size_t count = nondet_size_t();
    size_t elem_size = nondet_size_t();

    __CPROVER_assume(elem_size == sizeof(uint32_t) || elem_size == sizeof(uint64_t));
    __CPROVER_assume(count > SIZE_MAX / elem_size);

    size_t raw_wrap = count * elem_size;
    size_t a = mem_aligned_bytes(count, elem_size);
    __CPROVER_assert(a == mem_align_up(raw_wrap, DYNEMIT_MEM_ALIGN),
                     "mul overflow uses the wrapped product");
}

static void
proof_aligned_count_u32_u64(void)
{
    size_t n = nondet_size_t();

    __CPROVER_assume(n <= SIZE_MAX / sizeof(uint32_t));
    __CPROVER_assume(n * sizeof(uint32_t) <= SIZE_MAX - (DYNEMIT_MEM_ALIGN - 1));
    __CPROVER_assert(mem_aligned_count(n, uint32_t)
                         == mem_aligned_bytes(n, sizeof(uint32_t)),
                     "mem_aligned_count(uint32_t) matches aligned_bytes");

    __CPROVER_assume(n <= SIZE_MAX / sizeof(uint64_t));
    __CPROVER_assume(n * sizeof(uint64_t) <= SIZE_MAX - (DYNEMIT_MEM_ALIGN - 1));
    __CPROVER_assert(mem_aligned_count(n, uint64_t)
                         == mem_aligned_bytes(n, sizeof(uint64_t)),
                     "mem_aligned_count(uint64_t) matches aligned_bytes");
}

int
main(void)
{
    proof_align_no_wrap();
    proof_align_wrap();
    proof_aligned_bytes_no_wrap();
    proof_aligned_bytes_mul_wrap();
    proof_aligned_count_u32_u64();
    return 0;
}
