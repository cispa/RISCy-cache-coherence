# B2 CTG Performance

This experiment evaluates performance of the Cache-State Transfer Gadget (CTG) (B2) using an I-cache covert channel transferring 1 Mibit via D-cache state transfer (Section 5.2, Table 5).

## Files

- [`b2_ctg_performance.c`](./b2_ctg_performance.c) - main experiment
- [`b2_ctg_performance_results/`](./b2_ctg_performance_results/) - collected per-microarchitecture results
- [`table.py`](./table.py) - generates Table 5
- [`table_standalone.tex`](./table_standalone.tex) - standalone wrapper for rendering the table as a PDF

## Usage

Build and run:

```sh
make
./b2_ctg_performance [cpu]
```

This writes `b2_ctg_performance_results_<cpu>.csv` in the current directory.

To render Table 5 as a standalone PDF:

```sh
make b2_ctg_performance_table.pdf
```
