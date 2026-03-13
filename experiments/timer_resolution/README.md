# Timer Resolution

This artifact measures the effective timer resolution on each tested microarchitecture and compares it to the latency gap between cache hits and misses for both D-cache and I-cache `Flush+Reload` (Appendix B, Table 10).

## Files

- [`timer_resolution.c`](./timer_resolution.c) - runs one measurement and writes a CSV result
- [`timer_resolution_results/`](./timer_resolution_results/) - collected per-microarchitecture results
- [`table.py`](./table.py) - converts all CSVs into a LaTeX table
- [`table.tex`](./table.tex) - generated table
- [`table_standalone.tex`](./table_standalone.tex) - standalone wrapper for rendering the table as a PDF

## Usage

Build and run:

```sh
make
./timer_resolution [cpu]
```

This writes `timer_resolution_results_<cpu>.csv` in the current directory.

To render the table as a standalone PDF:

```sh
make timer_resolution_table.pdf
```
