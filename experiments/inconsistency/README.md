# Inconsistency

This experiment checks whether a CPU can be driven into an instruction/data-cache inconsistency state (Section 4.2).

## Files

- [`inconsistency.c`](./inconsistency.c) - main inconsistency check for RISC targets
- [`inconsistency_results/`](./inconsistency_results/) - collected per-microarchitecture results
- [`inconsistency_x86.c`](./inconsistency_x86.c) - reference check for x86

## Usage

Build and run:

```sh
make
./inconsistency [cpu]
```

This writes `inconsistency_results_<cpu>.csv` in the current directory.

Exit status:

- `0`: inconsistency observed
- `1`: no inconsistency observed in this run

## x86 Experiment

Build and run:

```sh
make inconsistency_x86
./inconsistency_x86
```

Expected output:

```text
no instruction/data-cache inconsistency observed
```
