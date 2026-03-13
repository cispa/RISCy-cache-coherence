#!/usr/bin/env python3
import sys, os, glob
import pandas as pd

def load_hits(path):
    df = pd.read_csv(path, usecols=["hits","rel-addr"])
    return df.groupby("rel-addr")["hits"].sum()

def main():
    if len(sys.argv) < 2:
        sys.exit("Usage: report_primes.py <dir>")
    d = sys.argv[1]
    prime_paths = sorted(glob.glob(os.path.join(d, "prime*.csv")))
    if not prime_paths:
        sys.exit("no prime*.csv found")
    noprime_path = os.path.join(d, "noprime.csv")
    noprime = load_hits(noprime_path)

    prime_dfs = [load_hits(p).rename(os.path.basename(p)) for p in prime_paths]
    primes = pd.concat(prime_dfs, axis=1).fillna(0).astype(int)
    primes["median"] = primes.median(axis=1)
    primes["base"] = noprime.reindex(primes.index).fillna(0).astype(int)

    top = primes.sort_values("median", ascending=False).head(10)

    for addr, row in top.iterrows():
        vals = [str(row[os.path.basename(p)]) for p in prime_paths]
        print(f"{addr}\t{row['base']}\t({','.join(vals)})\tmedian={row['median']}")

if __name__ == "__main__":
    main()
