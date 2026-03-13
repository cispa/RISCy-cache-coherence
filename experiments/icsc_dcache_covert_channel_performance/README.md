# ICSC D-Cache Covert Channel Performance

This experiment evaluates an ICSC-based D-cache covert channel transferring 1 Mibit (Section 5.3, Table 6) and compares ICSC against `Flush+Reload` (Section 5.4, Table 7).

## Files

- [`icsc_dcache_covert_channel_performance.c`](./icsc_dcache_covert_channel_performance.c) - main experiment
- [`icsc_dcache_covert_channel_performance_results/`](./icsc_dcache_covert_channel_performance_results/) - collected per-microarchitecture results
- [`table.py`](./table.py) - generates Table 6 (default) and Table 7 (`--compare-fr`) summaries
- [`table_standalone.tex`](./table_standalone.tex) - standalone wrapper for rendering the table as a PDF

## Usage

Build and run:

```sh
make
./icsc_dcache_covert_channel_performance [cpu]
```

This writes `icsc_dcache_covert_channel_performance_results_<cpu>.csv` in the current directory.

To render Table 6 as a standalone PDF:

```sh
make icsc_dcache_covert_channel_performance_table.pdf
```

To render Table 7 as a standalone PDF:

```sh
make icsc_dcache_covert_channel_performance_vs_flush_reload_table.pdf
```
