# RISC-V ICSC Library

This directory provides the RISC-V implementation of the architectural cache-state leakage primitive.

This implementation is only guaranteed to work as-is on C910. Other RISC-V microarchitectures may require adaptation.

## Quick Test

```sh
make run
```

The test pins to a core, repeatedly measures a flushed line and a cached line, and prints `should be low` / `should be high` percentages.

Expected output on C910 (example):

```text
Pinning to core 0...
should be low:    0.22
should be high:  99.63
should be low:    0.30
should be high:  99.26
---------------
```

## Using the Library

Include `lib_arch_sc_riscv64.h`, initialize once, then call `measure_arch_sc(victim)`:

```c
int cpu = -1;
init_arch_sc(&cpu);
int cached = measure_arch_sc(victim);
```

- `1` means victim is cached
- `0` means victim is not cached

For a complete usage example, see `test_sc.c`.
