# B1 I-Cache Oracle Performance

This experiment evaluates performance of B1 on affected microarchitectures using an I-cache covert channel transferring 1 Mibit (Section 5.1, Table 4).

## Files

- [`b1_icache_oracle_performance.c`](./b1_icache_oracle_performance.c) - main experiment
- [`b1_icache_oracle_performance_results/`](./b1_icache_oracle_performance_results/) - collected per-microarchitecture results
- [`table.py`](./table.py) - generates Table 4

## Usage

Build and run:

```sh
make
./b1_icache_oracle_performance [cpu]
```

This writes `b1_icache_oracle_performance_results_<cpu>.csv` in the current directory.

To render Table 4 as a standalone PDF:

```sh
make b1_icache_oracle_performance_table.pdf
```
