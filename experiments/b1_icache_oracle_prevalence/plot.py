import os
import argparse
import pandas as pd
import matplotlib.pyplot as plt
from glob import glob
from matplotlib.ticker import FuncFormatter
import numpy as np
from sklearn.metrics.pairwise import cosine_similarity

# SIMILARITY_THRESHOLD = 0.99
SIMILARITY_THRESHOLD = 1

seed = 1

RESTRICT_X = 128

RESULTS_DIR = "b1_icache_oracle_prevalence_results"
csv_files = glob(os.path.join(RESULTS_DIR, "*.csv"))

# Distinct markers used to differentiate CLUSTER lines (colors unchanged)
MARKERS = ["o", "s", "D", "^", "v", "<", ">", "P", "X", "*", "h", "H", "4", "8"]

# --------------------------------------------------------------------
# Hardcoded clusters: customize this to your data & desired groupings.
# Keyed by (jump, mfence) -> list of clusters; each cluster = list[str]
#
# Notes:
# - Only CPUs that actually appear in the data for the selected panel
#   will be plotted, even if listed here.
# - Any CPU present in the data but not listed here will become a
#   singleton cluster automatically.
# - If a (jump, mfence) key is missing, the script falls back to
#   similarity-based clustering (no error).
# --------------------------------------------------------------------
HARDCODED_CLUSTERS = {
    (1,1): [
        ["Cortex-A72", "C906"],
        ["Cortex-A720", "Cortex-A520", "Cortex-X4", "Cortex-A76", "Cortex-A73", "Icestorm", "Firestorm", "Cortex-A725", "Cortex-A55", "Carmel", "Cortex-A53", "C910", "Oryon"]
        # ["Cortex-A53", "Cortex-A55"],
        # ["C908", "X60"],
    ]
    # Example (adjust or replace with your real CPU names):
    # (1, 1): [
    #     ["Zen2", "Zen3"],
    #     ["Skylake", "Haswell"],
    #     ["Cortex-A76", "Cortex-A77"],
    # ],
    # (1, 0): [
    #     ["Cortex-A53", "Cortex-A55"],
    # ],
}

# Helper: cosine similarity that groups all-zero curves
def safe_similarity(vec1, vec2):
    v1_zero = not np.any(vec1)
    v2_zero = not np.any(vec2)
    if v1_zero and v2_zero:
        return 1.0  # treat both zero as identical
    if v1_zero or v2_zero:
        return -1.0  # don't match zero with non-zero
    return cosine_similarity([vec1], [vec2])[0, 0]

def compute_mcc(df):
    """Compute MCC per row from tp, fp, fn, tn columns."""
    tp = df["tp"].astype(float).to_numpy()
    fp = df["fp"].astype(float).to_numpy()
    fn = df["fn"].astype(float).to_numpy()
    tn = df["tn"].astype(float).to_numpy()
    num = tp * tn - fp * fn
    den = (tp + fp) * (tp + fn) * (tn + fp) * (tn + fn)
    # Avoid divide-by-zero; where den==0, set MCC=0
    mcc = np.zeros_like(num, dtype=float)
    mask = den > 0
    mcc[mask] = num[mask] / np.sqrt(den[mask])
    return mcc

def distinct_line_colors(n, background="light"):
    """
    Returns n visually distinct RGBA colors optimized for line plots.
    Uses tab20 (dark tones first), then tab20b, then tab20c.
    """
    def ordered_pairs(cmap_name):
        cols = list(plt.get_cmap(cmap_name).colors)  # length 20
        dark, light = cols[0::2], cols[1::2]
        return (dark + light) if background == "light" else (light + dark)

    bank = ordered_pairs("tab20")
    if n > 20:
        bank += ordered_pairs("tab20b")
    if n > 40:
        bank += ordered_pairs("tab20c")
    if n > len(bank):
        deficit = n - len(bank)
        bank += [(*bank[i % len(bank)][:3], 0.9) for i in range(deficit)]
    return bank[:n]

def build_hardcoded_clusters(cpu_names, jump_val, mfence):
    """
    Build clusters from HARDCODED_CLUSTERS for (jump_val, mfence).
    - Keeps only CPUs that are present in cpu_names.
    - Adds singleton clusters for any remaining CPUs not listed.
    Returns: List[List[str]]
    """
    key = (jump_val, mfence)
    clusters = []
    seen = set()

    if key in HARDCODED_CLUSTERS:
        for cluster in HARDCODED_CLUSTERS[key]:
            # Filter to CPUs that actually exist in this plot
            c = [cpu for cpu in cluster if cpu in cpu_names and cpu not in seen]
            if c:
                clusters.append(c)
                seen.update(c)

    # Add singletons for any CPUs not covered above
    for cpu in cpu_names:
        if cpu not in seen:
            clusters.append([cpu])

    return clusters

# Nested structure: jump -> mfence -> cpu_name -> DataFrame
grouped_data = {
    0: {0: {}, 1: {}},
    1: {0: {}, 1: {}},
}

# Read and group data
for file_path in csv_files:
    cpu_name = os.path.basename(file_path).replace(".csv", "")
    df = pd.read_csv(file_path)

    for (jump, mfence), group in df.groupby(["jump", "mfence"]):
        if jump in grouped_data and mfence in grouped_data[jump]:
            group = group.copy()
            group["mcc"] = compute_mcc(group)
            grouped_data[jump][mfence][cpu_name] = group

# Titles for 2x2 mode
titles = {
    0: "none",
    1: "mfence",
}

# ----------------------------
# CLI arguments / layout mode
# ----------------------------
parser = argparse.ArgumentParser(description="Plot MCC clusters and optionally save as SVG.")
parser.add_argument("--svg", nargs="?", const="mcc_plot.svg",
                    help="Save figure as SVG. Optional path, default: mcc_plot.svg")

# Single-plot mode: exactly one (jump, mfence) panel
parser.add_argument("--single-plot", action="store_true",
                    help="Plot exactly one panel specified by --jump/--mfence (defaults: jump=1, mfence=1).")
parser.add_argument("--jump", type=int, choices=[0, 1], default=1, help="Jump value for --single-plot mode.")
parser.add_argument("--mfence", type=int, choices=[0, 1], default=1, help="Mfence value for --single-plot mode.")

# Export TSV for single-plot case
parser.add_argument(
    "--export-tsv",
    nargs="?",
    const="single_plot.tsv",
    help=(
        "In --single-plot mode, export interpolated representative curves per cluster as TSV "
        "aligned by nops for tikz plots. Optional path (default: single_plot.tsv). "
        "Each column is a cluster labeled as ','.join(CPUs)."
    ),
)

# New: use hardcoded clustering instead of similarity-based clustering
parser.add_argument(
    "--use-hardcoded-clusters",
    action="store_true",
    help="Use HARDCODED_CLUSTERS[(jump,mfence)] to define clusters. "
         "CPUs not listed become singleton clusters. Missing keys fall back to similarity-based clustering."
)

args = parser.parse_args()

SKIP_CPUS = []
# if args.single_plot:
#     SKIP_CPUS += ["Cortex-A72", "C906"]

# Guard: refuse export when not in single-plot mode
if args.export_tsv and not args.single_plot:
    raise SystemExit("--export-tsv can only be used together with --single-plot")

# Decide which rows/columns to render
if args.single_plot:
    row_vals = [args.jump]
    fence_vals = [args.mfence]
    figsize = (9, 6)
else:
    row_vals = [0, 1]
    fence_vals = [0, 1]
    figsize = (15, 10)

if args.single_plot:
    plt.rcParams.update({
        'axes.titlesize': 18,
        'axes.labelsize': 16,
        'legend.fontsize': 14,
        'xtick.labelsize': 14,
        'ytick.labelsize': 14,
    })

# Create figure/axes
nrows, ncols = len(row_vals), len(fence_vals)
fig, axes = plt.subplots(nrows, ncols, figsize=figsize, sharex=True, sharey=True)

# Make axes indexable as 2D array
if nrows == 1 and ncols == 1:
    axes = np.array([[axes]])
elif nrows == 1 and ncols > 1:
    axes = np.array([axes])
elif nrows > 1 and ncols == 1:
    axes = np.array([[ax] for ax in axes])

# Build color palette only from CPUs that actually appear in the selected panels
used_cpus = sorted({
    cpu
    for j in row_vals
    for mf in fence_vals
    for cpu in grouped_data[j][mf]
})
palette = distinct_line_colors(len(used_cpus), background="light")
if len(used_cpus) > len(palette):
    print("Warning: fewer unique colors than CPUs; lines may reuse colors.")
elif len(used_cpus) > 20:
    print(f"Info: using {len(palette)} colors from tab20 + extensions (tab20b/c).")
cpu_color_map = {cpu: color for cpu, color in zip(used_cpus, palette)}

# For single-plot export: prepare holder
export_df = None

# Plot
for row, jump_val in enumerate(row_vals):
    for col, mfence in enumerate(fence_vals):
        cpu_data = grouped_data[jump_val][mfence]
        cpu_data = {k: v for k, v in cpu_data.items() if k not in SKIP_CPUS}
        ax = axes[row, col]
        # Set per-panel title
        if args.single_plot:
            title_text = f"jump={jump_val}, mfence={mfence}"
        else:
            title_text = f"{titles[mfence]}" + ("\nwith jump" if jump_val else "")
        ax.set_title(title_text)

        if not cpu_data:
            ax.text(0.5, 0.5, "No data for selected combination",
                    ha="center", va="center", transform=ax.transAxes)
            ax.set_axis_off()
            if args.export_tsv:
                raise SystemExit("No data available for the selected combination; cannot export TSV.")
            continue

        # Interpolation step
        all_nops = sorted(set().union(*[df["nops"].values for df in cpu_data.values()]))
        all_nops = [n for n in all_nops if n <= RESTRICT_X]   # 👈 restrict
        common_x = np.array(sorted(all_nops))
        interpolated = {}

        for cpu, df in cpu_data.items():
            df = df.sort_values("nops")
            interp_y = np.interp(common_x, df["nops"], df["mcc"])
            norm_val = np.linalg.norm(interp_y)
            norm = interp_y / norm_val if norm_val != 0 and not np.isnan(norm_val) else np.zeros_like(interp_y)
            interpolated[cpu] = norm

        # -----------------
        # Clustering step
        # -----------------
        if args.use_hardcoded_clusters and (jump_val, mfence) in HARDCODED_CLUSTERS:
            clustered = build_hardcoded_clusters(list(cpu_data.keys()), jump_val, mfence)
            print(f"Using hardcoded clusters for (jump={jump_val}, mfence={mfence}).")
        elif args.use_hardcoded_clusters and (jump_val, mfence) not in HARDCODED_CLUSTERS:
            print(f"No hardcoded clusters for (jump={jump_val}, mfence={mfence}); falling back to similarity clustering.")
            # fall through to similarity clustering below
            clustered = None
        else:
            clustered = None

        if clustered is None:
            clustered = []
            used = set()
            for cpu1, vec1 in interpolated.items():
                if cpu1 in used:
                    continue
                cluster = [cpu1]
                used.add(cpu1)
                for cpu2, vec2 in interpolated.items():
                    if cpu2 in used:
                        continue
                    sim = safe_similarity(vec1, vec2)
                    if sim >= SIMILARITY_THRESHOLD:
                        cluster.append(cpu2)
                        used.add(cpu2)
                clustered.append(cluster)

        if args.single_plot and args.export_tsv:
            export_df = pd.DataFrame({"nops": common_x})

        # Plot clusters: one representative line per cluster,
        # each with a DISTINCT MARKER (same color mapping as before).
        for idx, cluster in enumerate(clustered):
            repr_cpu = cluster[0]
            color = cpu_color_map.get(repr_cpu, None)

            # Compute best MCC across CPUs in this cluster (for legend)
            best_val = max(cpu_data[c]["mcc"].max() for c in cluster if c in cpu_data)

            # Legend label
            label_names_short = ", ".join(map(lambda s: s.replace("Cortex-", "")[:4], cluster))
            label = f"{label_names_short} [{best_val:.3f}]"

            df_rep = cpu_data[repr_cpu].sort_values("nops")
            df_rep = df_rep[df_rep["nops"] <= RESTRICT_X]  # 👈 restrict

            # Assign a distinct marker PER CLUSTER line
            marker = MARKERS[idx % len(MARKERS)]

            x = np.asarray(df_rep["nops"])
            y = np.asarray(df_rep["mcc"])

            # 1) Line + markers at EVERY data point
            ax.plot(
                x, y,
                label=label,
                color=color,
                linestyle="--",
                marker=marker,
                markersize=6,
                markeredgewidth=0.8,
                markevery=None  # markers at all data points
            )

            # Skip segments touching NaNs
            valid = ~(np.isnan(x[:-1]) | np.isnan(x[1:]) | np.isnan(y[:-1]) | np.isnan(y[1:]))

            rng = np.random.default_rng(
                seed if seed is not None else (hash(label) & 0xFFFFFFFF) if label else None
            )

            n_per_segment = 3
            xr_list, yr_list = [], []
            for i in np.where(valid)[0]:
                t = rng.random(n_per_segment)  # n random fractions along this segment
                xr_list.extend((1 - t) * x[i] + t * x[i + 1])
                yr_list.extend((1 - t) * y[i] + t * y[i + 1])

            # 3) Overlay the random markers
            ax.scatter(
                xr_list, yr_list,
                marker=marker,     # same shape as point markers
                s=36,              # ~ markersize=6
                # edgecolors=color,
                color=color,
                # facecolors='none', # hollow to avoid hiding the line
                linewidths=0.8,
                zorder=3
            )

            if args.single_plot and args.export_tsv:
                col_name = ",".join(cluster)
                rep_interp = np.interp(common_x, df_rep["nops"].to_numpy(), df_rep["mcc"].to_numpy())
                export_df[col_name] = rep_interp

        ax.set_xscale("log", base=2)
        ax.xaxis.set_major_formatter(FuncFormatter(lambda x, _: f"{int(x)}"))
        ax.set_xlabel("nops")
        ax.set_ylabel("MCC")
        ax.grid(True)

        if args.single_plot:
            ax.legend(fontsize=12, loc='right')
        else:
            if row == 0:
                ax.legend(fontsize="small", loc='lower center', bbox_to_anchor=(0.5, 1.02))
            else:
                ax.legend(fontsize="small", loc='upper center', bbox_to_anchor=(0.5, -0.25))

plt.tight_layout(rect=[0, 0, 1, 0.95])

if args.svg:
    fig.savefig(args.svg, format="svg")
else:
    plt.show()

if args.single_plot and args.export_tsv:
    if export_df is None or export_df.shape[1] == 1:
        raise SystemExit("No clusters produced; TSV not written.")
    export_path = args.export_tsv if isinstance(args.export_tsv, str) else "single_plot.tsv"
    export_df.to_csv(export_path, sep="\t", index=False)
    print(f"Exported single-plot TSV to: {export_path}")
