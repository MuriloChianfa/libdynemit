# CBMC proofs

Bounded model-checking of [`src/mem.h`](../src/mem.h).

## Install

Ubuntu package (version varies by distro):

```bash
sudo apt install cbmc
```

Or a current [CBMC release `.deb`](https://github.com/diffblue/cbmc/releases).

Confirm the bin:

```bash
cbmc --version
```

## Run

From the repository root:

```bash
./formal/run.sh
```

Override the binary with `CBMC=/path/to/cbmc`. Logs land in `formal/out/`.

## What is proved

| Proof | Bound | Properties |
|---|---|---|
| `mem_align_proof.c` | no loops | `mem_align_up` / `mem_aligned_bytes` no-wrap: result `>=` raw size, 64-aligned, slack `< 64`. Wrap of `size+mask` and `count*elem_size` is characterized (result is `mem_align_up` of the wrapped product). |
| `mem_copy_proof.c` | `destsz <= 16` (unwind 17) | `memsets` / `memcpys` NULL, `RSIZE_MAX`, success, and `count > destsz` contracts. Huge sizes are only used on paths that return before writing. |
