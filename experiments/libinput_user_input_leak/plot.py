#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt

# Read the CSV file into a DataFrame
# df = pd.read_csv("histnew4.csv")
# Captured for 12s
TOTAL_DURATION_S = 12.0
df = pd.read_csv("pixel9_touch_input_histogram.csv")
full_sample_count = len(df)

# Keep legacy filtering/downsampling (optional)
# df = df[df["time"] <= 300].reset_index(drop=True)
# df = df.iloc[::10].reset_index(drop=True)

df = df[:375*15000]
df = df[15000*235:]

# Good parameters, 9ms measurement window
# WINDOW = 5
# limit = 2

# Cleaned up trace
WINDOW = 15000
limit = 8

# Group into windows of size WINDOW, vote by at least limit
groups = df.groupby(df.index // WINDOW)
per_window = groups["time"].sum().ge(limit).astype(int)

per_window = 1 - per_window

seconds_per_window = TOTAL_DURATION_S * WINDOW / full_sample_count
x_seconds = (per_window.index.to_numpy() - per_window.index.min()) * seconds_per_window

print(per_window)

# Plot: one point per window, 0 or 1
plt.figure(figsize=(8, 4))
plt.plot(x_seconds, per_window.values, marker="o", linestyle="-")
plt.xlabel("Time (s)")
plt.ylabel("Cache Hit (0/1)")
plt.title("Cache Hit Detection (≥3 ones per window)")
plt.grid(True)
plt.tight_layout()
plt.show()

per_window.to_csv("windowed.csv", index=True, header=["reading"])
