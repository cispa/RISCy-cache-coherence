# B2 CTG Prevalence

This experiment evaluates the prevalence study of the Cache-State Transfer Gadget (CTG) (B2) (Section 4.3, Figure 4, Table 3).

## Files

- [`b2_ctg_prevalence.c`](./b2_ctg_prevalence.c) - main experiment
- [`b2_ctg_prevalence_results/`](./b2_ctg_prevalence_results/) - collected per-microarchitecture results
- [`table.py`](./table.py) - generates Table 3
- [`table_standalone.tex`](./table_standalone.tex) - standalone wrapper for rendering the table as a PDF

## Usage

Build and run:

```sh
make
./b2_ctg_prevalence [cpu]
```

This writes `b2_ctg_prevalence_results_<cpu>.csv` in the current directory.

To render Table 3 as a standalone PDF:

```sh
make b2_ctg_prevalence_table.pdf
```
