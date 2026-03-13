# AES T-Tables

This experiment evaluates the AES T-table case study (Section 6.2, Table 8; Appendix C, Figure 10).

## Files

- [`aes_t_table.c`](./aes_t_table.c) - main attack implementation (ICSC and `Flush+Reload` variants)
- [`aes_t_table_results/`](./aes_t_table_results/) - collected result files
- [`table.py`](./table.py) - generates the AES result table
- [`table_standalone.tex`](./table_standalone.tex) - standalone wrapper for rendering the table as a PDF
- [`t_table_cache_access_patterns_standalone.tex`](./t_table_cache_access_patterns_standalone.tex) - standalone wrapper for rendering the T-table cache-access patterns figure as a PDF
- [`eval.sh`](./eval.sh) - repeated-run evaluator for `archsc` and `fr`
- [`make_matrix.py`](./make_matrix.py) - helper to format histogram matrices for plotting

## Usage

Build `ICSC` and `Flush+Reload` binaries:

```sh
make && cp aes_t_table aes_t_table_archsc
FR=1 make && cp aes_t_table aes_t_table_fr
```

Run repeated evaluation on a core:

```sh
bash eval.sh <runs> <core>
```

Example:

```sh
bash eval.sh 100 0
```

To render the performance table (Table 8) as a standalone PDF:

```sh
make aes_t_table_table.pdf
```

To render Appendix C, Figure 10 as a standalone PDF:

```sh
make t_table_cache_access_patterns.pdf
```
