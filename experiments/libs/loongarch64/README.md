# LoongArch ICSC Library

This directory provides the LoongArch implementation of the architectural cache-state leakage primitive.

This implementation is only guaranteed to work as-is on Loongson 3A5000-HV. Other LoongArch microarchitectures may require adaptation.

## Quick Test

```sh
make run
```

The test pins to a core, repeatedly measures a flushed line and a cached line, and prints `should be low` / `should be high` percentages.

Expected output on Loongson 3A5000-HV (example):

```text
Pinning to core 3...
should be low:    0.01
should be high: 100.00
should be low:    0.00
should be high: 100.03
---------------
```

## Using the Library

Include `lib_arch_sc_loongarch64.h`, initialize once, then call `measure_arch_sc(victim)`:

```c
int cpu = -1;
init_arch_sc(&cpu);
int cached = measure_arch_sc(victim);
```

- `1` means victim is cached
- `0` means victim is not cached

For a complete usage example, see `test_sc.c`.

If you run configurations that require privileged flush/iflush operations, see [../../../kernel_modules/loongarch64_privileged_module/README.md](../../../kernel_modules/loongarch64_privileged_module/README.md).
