# AArch64 ICSC Library

This directory provides the ARMv8 implementation of the architectural cache-state leakage primitive.

This implementation is only guaranteed to work as-is on Cortex-A76. Other AArch64 microarchitectures may require adaptation.

## Quick Test

```sh
make run
```

The test pins to a core, repeatedly measures a flushed line and a cached line, and prints `should be low` / `should be high` percentages.

Expected output on Cortex-A76 (example):

```text
Pinning to core 5...
should be low:    0.01
should be high: 100.00
should be low:    0.02
should be high:  99.80
---------------
```

## Using the Library

Include `lib_arch_sc_aarch64.h`, initialize once, then call `measure_arch_sc(victim)`:

```c
int cpu = -1;
init_arch_sc(&cpu);
int cached = measure_arch_sc(victim);
```

- `1` means victim is cached
- `0` means victim is not cached

For a complete usage example, see `test_sc.c`.
