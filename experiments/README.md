# Experiments

This directory contains all paper experiments and case studies.

## Building Blocks and Prevalence (Section 4)

- [libs](./libs/): Shared ICSC core libraries (cross-ISA `cacheutils`, I-cache probing helpers, and per-architecture implementations for AArch64/Oryon, RISC-V, and LoongArch).
- [inconsistency](./inconsistency/): Instruction/data-cache inconsistency check (Section 4.2).
- [b1_icache_oracle_prevalence](./b1_icache_oracle_prevalence/): B1 I-cache oracle prevalence experiment (Section 4.2, Figure 5/6).
- [b1_icache_oracle_performance](./b1_icache_oracle_performance/): B1 I-cache oracle performance experiment (Section 5.1, Table 4).
- [b2_ctg_prevalence](./b2_ctg_prevalence/): B2 CTG prevalence experiment (Section 4.3, Figure 4, Table 3).

## Covert-Channel Performance (Section 5)

- [b2_ctg_performance](./b2_ctg_performance/): B2 CTG performance experiment (Section 5.2, Table 5).
- [icsc_dcache_covert_channel_performance](./icsc_dcache_covert_channel_performance/): ICSC D-cache covert-channel performance (Section 5.3, Table 6) and Flush+Reload comparison (Section 5.4, Table 7).

## Case Studies (Section 6)

- [spectral](./spectral/): Architectural Spectre (Spectral) case study (Section 6.1).
- [aes_t_table](./aes_t_table/): AES T-table case study (Section 6.2, Figure 7, Table 8; Appendix C, Figure 10).
- [libinput_user_input_leak](./libinput_user_input_leak/): Shared-library user input attack on Android (Section 6.3, Figure 8).

## Mitigation and Appendix Experiments (Section 7.1, Appendix B)

- [software_mitigation](./software_mitigation/): Software mitigation based on blocking W+X mappings (Section 7.1).
- [timer_resolution](./timer_resolution/): Timer resolution experiment (Appendix B, Table 10).

## Running Experiments

> [!IMPORTANT]
> Some experiments require kernel modules on specific platforms:
> - **Loongson (LoongArch):** follow [../kernel_modules/loongarch64_privileged_module/README.md](../kernel_modules/loongarch64_privileged_module/README.md) before running experiments.
> - **Apple (Asahi Linux):** follow [../kernel_modules/apple-uarch-module/README.md](../kernel_modules/apple-uarch-module/README.md) before running experiments.

We provide [run-and-gather](../bin/run-and-gather), a shared helper for multi-machine experiments. It cross-compiles the current experiment for the required architectures/variants, copies binaries to selected machines, runs them remotely, stores logs, and gathers result files.

Before using `run-and-gather`, adapt the machine list in the script to your setup. In particular, add your own hosts and configure their architecture, core IDs, microarchitecture names, and any experiment-specific variants/build flags.

Example usage:

```sh
run-and-gather
run-and-gather lab88 lab53 lab67
run-and-gather --make-flag THEAD=1 lab53
run-and-gather --build-only lab88 lab53
```

Run it from inside an experiment directory with a matching `Makefile`. Logs are written to `logs/`, and collected outputs to `<experiment>_results/`.
Built helper binaries are stored in `out/`.
