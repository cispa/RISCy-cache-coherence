#!/usr/bin/env python3
import argparse
import pandas as pd
import matplotlib.pyplot as plt
from pathlib import Path

# CLI arguments
parser = argparse.ArgumentParser()
parser.add_argument(
    "--svg",
    metavar="OUTFILE",
    help="If set, save plot as an SVG to OUTFILE instead of (or in addition to) showing."
)
args = parser.parse_args()

# Directory containing CSVs
data_dir = Path("b2_ctg_prevalence_results")

# Mapping from (trans, jump, load, dep, victim) → label
scenario_map = {
    (0, 0, 0, 0, 0): "baseline",
    (0, 0, 1, 0, 0): "load",
    (0, 1, 0, 0, 0): "jump",
    (1, 0, 1, 0, 0): "tran load",
    (1, 1, 0, 0, 0): "tran jump",
    (1, 0, 1, 1, 0): "tran dep load",
    (1, 1, 0, 1, 0): "tran dep jump",
    (1, 0, 1, 1, 1): "tran dep cached load",
    (1, 1, 0, 1, 1): "tran dep cached jump",
}

# desired plotting order
scenario_order = [
    "baseline",
    "load",
    "jump",
    "tran load",
    "tran jump",
    "tran dep load",
    "tran dep cached load",
    "tran dep jump",
    "tran dep cached jump",
]

records = []

# Read each file
for csv_file in data_dir.glob("*.csv"):
    uarch = csv_file.stem
    df = pd.read_csv(csv_file)

    # Require victim column
    required_cols = {"transient", "jump", "load", "dependent", "access_victim", "time"}
    if not required_cols.issubset(df.columns):
        raise SystemExit(
            f"error: '{csv_file}' missing required columns {required_cols - set(df.columns)}"
        )

    for _, row in df.iterrows():
        key = (
            int(row["transient"]),
            int(row["jump"]),
            int(row["load"]),
            int(row["dependent"]),
            int(row["access_victim"]),
        )
        label = scenario_map.get(key, f"unknown_{key}")
        records.append({
            "µarch": uarch,
            "scenario": label,
            "time": row["time"]
        })

# Create dataframe and pivot
df_all = pd.DataFrame(records)
df_pivot = df_all.pivot(index="µarch", columns="scenario", values="time")

# Reorder columns to match scenario_order
df_pivot = df_pivot.reindex(columns=scenario_order)

# Normalize so baseline = 1.0
df_norm = df_pivot.div(df_pivot["baseline"], axis=0)

# Plot
ax = df_norm.plot(kind="bar", figsize=(14, 6))
plt.ylabel("Relative Latency")
plt.axhline(1.0, color="black", linestyle="--", linewidth=0.8)
plt.xticks(rotation=45, ha="right")
plt.legend(title="Scenario")
plt.tight_layout()

# Save SVG if requested
if args.svg:
    plt.savefig(args.svg, format="svg")
else:
    plt.show()
