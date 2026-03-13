#!/usr/bin/env python3
import sys
import numpy as np

def read_file(fname):
    """Read CSV-style numbers from file -> list of list[int]."""
    rows = []
    with open(fname) as f:
        for line in f:
            if not line.strip():
                continue
            nums = [int(x) for x in line.strip().split(",")]
            rows.append(nums)
    return rows

def normalize_rows(rows):
    """Normalize across all values in one dataset (all rows), then invert."""
    all_vals = [v for row in rows for v in row]
    lo, hi = min(all_vals), max(all_vals)

    if hi == lo:  # avoid division by zero
        return [[0 for _ in row] for row in rows]

    normed = []
    for row in rows:
        norm_row = [100 - int(round((x - lo) / (hi - lo) * 100)) for x in row]
        normed.append(norm_row)
    return normed

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} file1 file2", file=sys.stderr)
        sys.exit(1)

    file1, file2 = sys.argv[1], sys.argv[2]

    rows1 = read_file(file1)
    rows2 = read_file(file2)

    n = min(len(rows1), len(rows2))
    norm1 = normalize_rows(rows1[:n])
    norm2 = normalize_rows(rows2[:n])

    for r1, r2 in zip(norm1, norm2):
        middle = "0," * 5 + "0"   # 6 zeros
        line = "{" + ",".join(f"{v:3d}" for v in r1) \
               + ",  " + middle + ",  " \
               + ",".join(f"{v:3d}" for v in r2) + "},"
        print(line)

if __name__ == "__main__":
    main()
