# libs

This directory contains the core cross-ISA primitives used by most experiments.

## Structure

- [`cacheutils.h`](./cacheutils.h) - shared cache/timer helpers (`flush`, fences, timestamping, calibration)
- [`lib_probe_icache.h`](./lib_probe_icache.h) - common I-cache probing helpers used by experiment code
- [`measure_helpers.h`](./measure_helpers.h) - shared measurement helpers for timing and ICSC-gadget readout
- [`b1_config.h`](./b1_config.h) - shared B1 parameter selection (`nops`, `mfence`, `jump`) from build flags
- [`aarch64/`](./aarch64/) - ARM-specific ICSC implementation and test programs ([README](./aarch64/README.md))
- [`aarch64/oryon/`](./aarch64/oryon/) - Oryon-tuned ARM implementation ([README](./aarch64/oryon/README.md))
- [`riscv64/`](./riscv64/) - RISC-V-specific ICSC implementation and test programs ([README](./riscv64/README.md))
- [`loongarch64/`](./loongarch64/) - LoongArch-specific ICSC implementation and test programs ([README](./loongarch64/README.md))

## Notes

- B1 (I-cache oracle) and B2 (CTG) behavior is implemented in experiment code, but both rely on these shared primitives.
- For Loongson and Apple systems, follow [../../kernel_modules/loongarch64_privileged_module/README.md](../../kernel_modules/loongarch64_privileged_module/README.md) and [../../kernel_modules/apple-uarch-module/README.md](../../kernel_modules/apple-uarch-module/README.md) when privileged cache operations require helper modules.
