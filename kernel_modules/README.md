# Kernel Modules

This directory contains optional platform-specific kernel helpers used by some experiments.

- [apple-uarch-module](./apple-uarch-module/): Apple Silicon (Asahi Linux) module for userspace cache flush and cycle-counter access.
- [loongarch64_privileged_module](./loongarch64_privileged_module/): LoongArch module exposing privileged cache-maintenance operations.

Refer to each subdirectory README for build/insert instructions.
