# Spectral

This experiment evaluates the Architectural Spectre ("Spectral") case study (Section 6.1).

## Files

- [`main.c`](./main.c) - base Spectral PoC (`ARCHSC` and `Flush+Reload` variants via build flags)
- [`main_oryon.c`](./main_oryon.c) - Oryon-tuned variant
- [`main_loongson.c`](./main_loongson.c) - Loongson-tuned variant
- [`benchmark.sh`](./benchmark.sh) - runs repeated trials and reports median/mean/standard error and success rate
- [`spectral_results/`](./spectral_results/) - collected example logs

## Usage

Build and run one trial:

```sh
make
./poc [cpu]
```

Build architecture-tuned variants:

```sh
make poc_oryon
./poc_oryon [cpu]

make poc_loongson
./poc_loongson [cpu]
```

Run repeated trials (default `ARCHSC` mode):

```sh
make -B && bash benchmark.sh <runs>
```

Run repeated trials with `Flush+Reload` baseline:

```sh
FR=1 make -B && bash benchmark.sh <runs>
```

For T-Head C910 configuration:

```sh
THEAD=1 make -B && bash benchmark.sh <runs>
```
