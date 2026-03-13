# Shared Library User Input Attack

This experiment evaluates the shared-library user input leakage case study on Android (Section 6.3, Figure 8).

## Files

- [`probe_addr.c`](./probe_addr.c) - probes the selected `libinput.so` cache line (`0x1639d0`) over time
- [`scan.c`](./scan.c) - scans mapped libraries/cache lines to identify candidate hot offsets
- [`analysis.py`](./analysis.py) - summarizes repeated scan outputs (`prime*.csv` and `noprime.csv`)
- [`plot.py`](./plot.py) - plots the leakage trace used for visualization
- [`scan_results/`](./scan_results/) - example scan outputs
- [`Pixel_9_libinput.so`](./Pixel_9_libinput.so) and [`Alcatel_One_Touch_Pop_2_libinput.so`](./Alcatel_One_Touch_Pop_2_libinput.so) - reference binaries used during offset matching

## Usage

Build tools:

```sh
make scan probe_addr
```

Scan for candidate cache lines (reads file paths from `libs`):

```sh
printf '/system/lib64/libinput.so\n' > libs
./scan <cpu>
```

Run the final leakage probe on the selected line (`0x1639d0`):

```sh
./probe_addr <cpu>
```

This writes `histogram_<cpu>.csv` in the current directory.

Generate the Figure 8-style leakage plot:

```sh
python plot.py
```
