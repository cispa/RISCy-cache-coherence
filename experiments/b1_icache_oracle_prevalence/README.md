# B1 I-Cache Oracle Prevalence

This experiment evaluates prevalence and parameter sensitivity of the B1 I-cache oracle (Section 4.2, Figure 5/6).

## Files

- [`b1_icache_oracle_prevalence.c`](./b1_icache_oracle_prevalence.c) - main experiment
- [`b1_icache_oracle_prevalence_results/`](./b1_icache_oracle_prevalence_results/) - collected per-microarchitecture results
- [`plot.py`](./plot.py) - plotting script for the main results

## Usage

Build and run:

```sh
make
./b1_icache_oracle_prevalence [cpu]
```

This writes `b1_icache_oracle_prevalence_results_<cpu>.csv` in the current directory.

Plotting:

```sh
make plot
```

The panel used in the paper is the bottom-right subplot (`jump=1`, `mfence=1`).
