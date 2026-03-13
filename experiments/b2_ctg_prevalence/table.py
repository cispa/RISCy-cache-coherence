#!/usr/bin/env python3

import argparse
import csv
import glob
import os
import sys
from typing import Dict, List, Tuple, Optional, Iterable

RESET = "\x1b[0m"

Key4 = Tuple[int, int, int, int]
Key5 = Tuple[int, int, int, int, int]  # (transient, jump, load, dependent, access_victim)


def rgb_ansi(r: int, g: int, b: int, s: str) -> str:
    return f"\x1b[38;2;{r};{g};{b}m{s}{RESET}"


def lerp(a: float, b: float, t: float) -> float:
    return a + (b - a) * t


def green_to_red(t: float) -> Tuple[int, int, int]:
    t = max(0.0, min(1.0, t))
    r = int(lerp(0, 255, t))
    g = int(lerp(255, 0, t))
    return r, g, 0


def purple_to_red(t: float) -> Tuple[int, int, int]:
    """
    Eye-friendly scale for white backgrounds:
    deep purple (102, 0, 153) → deep blue (0, 0, 200) → strong red (200, 0, 0).
    """
    t = max(0.0, min(1.0, t))
    if t < 0.5:
        # 0.0 → 0.5 : purple to blue
        frac = t / 0.5
        r = int(102 + (0   - 102) * frac)
        g = int(0   + (0   - 0)   * frac)
        b = int(153 + (200 - 153) * frac)
    else:
        # 0.5 → 1.0 : blue to red
        frac = (t - 0.5) / 0.5
        r = int(0   + (200 - 0)   * frac)
        g = 0
        b = int(200 + (0   - 200) * frac)
    return r, g, b


def nonlinearize(t: float, gamma: float) -> float:
    if gamma <= 0:
        gamma = 1.0
    return t ** gamma


def _detect_header_aliases(fieldnames: Iterable[str]) -> Dict[str, Optional[str]]:
    fields = {f.strip(): f.strip() for f in (fieldnames or [])}

    def first_present(*candidates: str) -> Optional[str]:
        for c in candidates:
            if c in fields:
                return fields[c]
        return None

    aliases = {
        "trans": first_present("transient", "trans"),
        "jump": first_present("jump"),
        "load": first_present("load"),
        "dep": first_present("dependent", "dep"),
        "victim": first_present("access_victim", "victim"),  # optional
        "time": first_present("time"),
    }
    return aliases


def parse_dir(path: str) -> Tuple[List[Key5], Dict[str, Dict[Key5, float]]]:
    files = sorted(glob.glob(os.path.join(path, "*.csv")))
    if not files:
        print(f"error: no CSV files found in '{path}'", file=sys.stderr)
        sys.exit(1)

    data: Dict[str, Dict[Key5, float]] = {}
    all_cases: set[Key5] = set()

    for f in files:
        micro = os.path.splitext(os.path.basename(f))[0]
        rows: Dict[Key5, float] = {}

        with open(f, newline="") as fh:
            reader = csv.DictReader(fh)
            if not reader.fieldnames:
                print(f"error: '{f}' has no header row", file=sys.stderr)
                sys.exit(1)

            aliases = _detect_header_aliases(reader.fieldnames)
            missing_required = [k for k in ("trans", "jump", "load", "dep", "time") if aliases.get(k) is None]
            if missing_required:
                expected = "transient|trans, jump, load, dependent|dep, time (+ optional access_victim)"
                print(
                    f"error: '{f}' missing required columns: {', '.join(missing_required)}\n"
                    f"       header had: {', '.join(reader.fieldnames)}\n"
                    f"       expected at least: {expected}",
                    file=sys.stderr,
                )
                sys.exit(1)

            for r in reader:
                try:
                    t = int(r[aliases["trans"]])
                    j = int(r[aliases["jump"]])
                    l = int(r[aliases["load"]])
                    d = int(r[aliases["dep"]])
                    v = int(r[aliases["victim"]])
                    val = float(r[aliases["time"]])
                except Exception as e:
                    print(f"error parsing row in '{f}': {e}\nrow={r}", file=sys.stderr)
                    sys.exit(1)

                key: Key5 = (t, j, l, d, v)
                rows[key] = val
                all_cases.add(key)

        data[micro] = rows

    ordering_map: Dict[Key5, int] = {}
    order_list: List[Key5] = [
        (0, 0, 0, 0, 0),
        (0, 0, 1, 0, 0),
        (0, 1, 0, 0, 0),
        (1, 0, 1, 0, 0),
        (1, 1, 0, 0, 0),
        (1, 0, 1, 1, 0),
        (1, 0, 1, 1, 1),
        (1, 1, 0, 1, 0),
        (1, 1, 0, 1, 1),
    ]
    for idx, pat in enumerate(order_list):
        ordering_map[pat] = idx

    def case_rank(c: Key5) -> Tuple[int, Key5]:
        return (ordering_map.get(c, 999), c)

    testcases: List[Key5] = sorted(all_cases, key=case_rank)
    return testcases, data


def case_label(case: Key5) -> str:
    t, j, l, d, v = case
    base_map: Dict[Key5, str] = {
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
    return base_map.get(case, f"T{t}J{j}L{l}D{d}V{v}")


def compute_col_widths(headers: List[str], rows: List[List[str]]) -> List[int]:
    widths = [len(h) for h in headers]
    for r in rows:
        for i, cell in enumerate(r):
            widths[i] = max(widths[i], len(cell))
    return widths


def pad_and_color(value_str: str, width: int, color: Optional[Tuple[int, int, int]]) -> str:
    pad = " " * (width - len(value_str))
    if color is None:
        return value_str + pad
    r, g, b = color
    return rgb_ansi(r, g, b, value_str) + pad


def format_value(v: float) -> str:
    return f"{v:.3f}"


def format_pct(delta: float) -> str:
    sign = "+" if delta >= 0 else ""
    return f"{sign}{delta:.1f}%"


def latex_escape(text: str) -> str:
    repl = {
        "\\": r"\textbackslash{}",
        "&": r"\&",
        "%": r"\%",
        "$": r"\$",
        "#": r"\#",
        "_": r"\_",
        "{": r"\{",
        "}": r"\}",
        "~": r"\textasciitilde{}",
        "^": r"\textasciicircum{}",
        "µ": r"\textmu{}",
    }
    out = []
    for ch in text:
        out.append(repl.get(ch, ch))
    return "".join(out)


def latex_rgb01_from_rgb255(rgb: Tuple[int, int, int]) -> Tuple[float, float, float]:
    r, g, b = rgb
    return (r / 255.0, g / 255.0, b / 255.0)


def latex_makecell_label(label: str) -> str:
    parts = label.split()
    if len(parts) <= 1:
        return latex_escape(label)
    return r"\makecell{" + " \\\\ ".join(latex_escape(p) for p in parts) + "}"


def latex_format_value(v: float, pct_mode: bool) -> str:
    if pct_mode:
        # return r"\SI{" + f"{v:+.1f}" + r"}{\percent}"
        return "\\dotscale{0.5}{" + f"{-v/100}" + "}"
    else:
        return f"{v:.3f}"


def latex_color_text(s: str, rgb01: Optional[Tuple[float, float, float]]) -> str:
    if rgb01 is None:
        return s
    r, g, b = rgb01
    return f"\\textcolor[rgb]{{{r:.3f},{g:.3f},{b:.3f}}}{{{s}}}"


def emit_latex_table(headers: List[str],
                     rows_text: List[List[Optional[str]]],
                     rows_colors_rgb255: List[List[Optional[Tuple[int, int, int]]]],
                     caption: Optional[str],
                     label: Optional[str]) -> None:
    colspec = "l" + ("r" * (len(headers) - 1))
    print("% LaTeX table generated by b2_ctg_prevalence/table.py")
    print("\\begin{tabular}{" + colspec + "}")
    print("\\toprule")
    header_line = " & ".join(f"\\textbf{{{h}}}" for h in headers) + r" \\"
    print(header_line)
    print("\\midrule")
    for text_row, color_row in zip(rows_text, rows_colors_rgb255):
        cells = []
        for s, c in zip(text_row, color_row):
            s = "-" if s is None else s
            rgb01 = latex_rgb01_from_rgb255(c) if c else None
            cells.append(latex_color_text(s, rgb01))
        print(" & ".join(cells) + r" \\")
    print("\\bottomrule")
    print("\\end{tabular}")
    if caption:
        print(f"% caption: {caption}")
    if label:
        print(f"% label: {label}")

def main():
    ap = argparse.ArgumentParser(add_help=False, description=None)
    ap.add_argument("dir", nargs="?", default="b2_ctg_prevalence_results",
                    help="directory containing per-µarch CSV files")
    ap.add_argument("--pct-baseline", action="store_true",
                    help="show values as percent change vs the baseline (0,0,0,0,0) for each row; hides the baseline column")
    ap.add_argument("--gamma", type=float, default=0.8,
                    help="gamma for color mapping (lower <1 => more sensitivity at low end)")
    ap.add_argument("--no-color", action="store_true", help="disable ANSI colors (terminal mode only)")
    ap.add_argument("--latex", action="store_true",
                    help="emit a LaTeX tabular instead of ANSI table (uses colored *text* and SI for percents)")
    ap.add_argument("--full-columns", action="store_true",
                    help="show all diagnostic columns (old format) instead of the compact paper layout")
    ap.add_argument("--latex-caption", default=None, help="(LaTeX) optional caption comment")
    ap.add_argument("--latex-label", default=None, help="(LaTeX) optional label comment")
    ap.add_argument("-h", "--help", action="help", help="show this help message and exit")
    args = ap.parse_args()

    testcases, data = parse_dir(args.dir)

    BASEKEY: Key5 = (0, 0, 0, 0, 0)

    CASE_TD_LOAD: Key5 = (1, 0, 1, 1, 0)    # transient dependent load, victim uncached
    CASE_TD_CLOAD: Key5 = (1, 0, 1, 1, 1)   # transient dependent load, victim cached
    CASE_TD_JUMP: Key5 = (1, 1, 0, 1, 0)    # transient dependent jump, victim uncached
    CASE_TD_CJUMP: Key5 = (1, 1, 0, 1, 1)   # transient dependent jump, victim cached

    if args.full_columns:
        display_cases: List[Key5] = [c for c in testcases if not (args.pct_baseline and c == BASEKEY)]
    else:
        display_cases = [CASE_TD_LOAD, CASE_TD_CLOAD, CASE_TD_JUMP, CASE_TD_CJUMP]

    # In-order cores (no \cmark). Everything else → treated as out-of-order.
    IN_ORDER_SET = {
        "C906",
        "C908",
        "X60",
        "Carmel",
        "Cortex-A520",
        "Cortex-A53",
        "Cortex-A55",
    }

    # Headers for terminal output. LaTeX compact mode uses a custom multi-row header.
    headers: List[str] = ["µarch"]
    if args.full_columns:
        for c in display_cases:
            headers.append(case_label(c))
            if c == (1, 1, 0, 0, 0):
                headers.append("tran jump works")
    else:
        headers += ["load uncached", "load cached", "fetch uncached", "fetch cached"]
    headers.append(r"\bbtwo")

    plain_rows: List[List[str]] = []
    color_rows: List[List[Optional[Tuple[int, int, int]]]] = []

    latex_rows_text: List[List[Optional[str]]] = []
    latex_rows_colors: List[List[Optional[Tuple[int, int, int]]]] = []

    for micro, rows in sorted(data.items()):
        base_val = rows.get(BASEKEY)

        if micro in IN_ORDER_SET:
            continue

        # Prepare display values (raw or % vs baseline)
        disp_vals_by_case: Dict[Key5, Optional[float]] = {}
        for c in display_cases:
            raw_v = rows.get(c)
            if raw_v is None:
                disp_vals_by_case[c] = None
            elif args.pct_baseline:
                if base_val is None or base_val == 0:
                    disp_vals_by_case[c] = None
                else:
                    disp_vals_by_case[c] = (raw_v / base_val - 1.0) * 100.0
            else:
                disp_vals_by_case[c] = raw_v

        present = [v for v in disp_vals_by_case.values() if v is not None]
        vmin, vmax = (min(present), max(present)) if present else (0.0, 0.0)

        # Start row with µarch and out-of-order flag
        plain_cells = [micro]
        color_cells: List[Optional[Tuple[int, int, int]]] = [None]

        latex_cells: List[Optional[str]] = [latex_escape(micro)]
        latex_colors: List[Optional[Tuple[int, int, int]]] = [None]

        # Out-of-order column: checkmark if NOT in known in-order list.
        # is_ooo = micro not in IN_ORDER_SET
        # if is_ooo:
        #     plain_cells.append("✓")
        #     color_cells.append(None)
        #     latex_cells.append(r"\cmark")
        #     latex_colors.append(None)
        # else:
        #     plain_cells.append("")
        #     color_cells.append(None)
        #     latex_cells.append("")
        #     latex_colors.append(None)

        # Emit per-case cells (full mode inserts "tran jump works" after tran jump)
        for c in display_cases:
            v = disp_vals_by_case[c]
            if v is None:
                plain_cells.append("-")
                color_cells.append((120, 120, 120) if not args.no_color else None)
                latex_cells.append("-")
                latex_colors.append((180, 180, 180))
            else:
                s_term = format_pct(v) if args.pct_baseline else format_value(v)
                plain_cells.append(s_term)
                if args.no_color or args.latex:
                    color_cells.append(None)
                else:
                    t = 0.5 if vmax == vmin else (v - vmin) / (vmax - vmin)
                    t = nonlinearize(t, args.gamma)
                    color_cells.append(green_to_red(t))

                s_tex = latex_format_value(v, args.pct_baseline)
                latex_cells.append(s_tex)
                t = 0.5 if vmax == vmin else (v - vmin) / (vmax - vmin)
                t = nonlinearize(t, args.gamma)
                latex_colors.append(purple_to_red(t))

            if args.full_columns and c == (1, 1, 0, 0, 0):
                works_tjump: Optional[bool]
                disp_jump = disp_vals_by_case.get((0, 1, 0, 0, 0))
                disp_tjump = disp_vals_by_case.get((1, 1, 0, 0, 0))
                if disp_jump is None or disp_tjump is None:
                    works_tjump = None
                else:
                    if args.pct_baseline:
                        works_tjump = abs(disp_tjump - disp_jump) <= 15.0
                    else:
                        works_tjump = abs(rows[(1, 1, 0, 0, 0)] - rows[(0, 1, 0, 0, 0)]) <= 10.0

                if works_tjump is None:
                    plain_cells.append("-")
                    color_cells.append((120, 120, 120) if not args.no_color else None)
                    latex_cells.append("-")
                    latex_colors.append((180, 180, 180))
                elif works_tjump:
                    plain_cells.append("✓")
                    color_cells.append(None)
                    latex_cells.append(r"\cmark")
                    latex_colors.append(None)
                else:
                    plain_cells.append("")
                    color_cells.append(None)
                    latex_cells.append("")
                    latex_colors.append(None)

        # --- \bbtwo check (raw or % depending on --pct-baseline) ---
        raw_tdj = rows.get(CASE_TD_JUMP)
        raw_tdcj = rows.get(CASE_TD_CJUMP)
        if args.pct_baseline:
            disp_tdj = disp_vals_by_case.get(CASE_TD_JUMP)
            disp_tdcj = disp_vals_by_case.get(CASE_TD_CJUMP)
            if disp_tdj is None or disp_tdcj is None:
                works_bbtwo = None
            else:
                works_bbtwo = (disp_tdj - 10.0) > disp_tdcj
        else:
            if raw_tdj is None or raw_tdcj is None:
                works_bbtwo = None
            else:
                works_bbtwo = (raw_tdj - 10.0) > raw_tdcj

        if works_bbtwo is None:
            plain_cells.append("-")
            color_cells.append((120, 120, 120) if not args.no_color else None)
            latex_cells.append("-")
            latex_colors.append((180, 180, 180))
        elif works_bbtwo:
            plain_cells.append("✓")
            color_cells.append(None)
            latex_cells.append(r"\cmark")
            latex_colors.append(None)
        else:
            plain_cells.append("")
            color_cells.append(None)
            latex_cells.append("")
            latex_colors.append(None)

        plain_rows.append(plain_cells)
        color_rows.append(color_cells)

        latex_rows_text.append(latex_cells)
        latex_rows_colors.append(latex_colors)

    if args.latex:
        if not args.full_columns:
            print("% LaTeX table generated by b2_ctg_prevalence/table.py")
            print(r"\begin{tabular}{lccccc}")
            print(r"\toprule")
            print(r" & \multicolumn{2}{c}{\small\textbf{Data Load}}")
            print(r" & \multicolumn{2}{c}{\small\textbf{Instr. Fetch}}")
            print(r" & \multirow{2}{*}{\small\textbf{\bbtwo}} \\")
            print(r"\cmidrule(lr){2-3}\cmidrule(lr){4-5}")
            print(r"\textbf{Microarch.} & \small\textbf{uncached} & \small\textbf{cached}")
            print(r" & \small\textbf{uncached} & \small\textbf{cached}")
            print(r" & \\")
            print(r"\midrule")
            for text_row, color_row in zip(latex_rows_text, latex_rows_colors):
                cells = []
                for s, c in zip(text_row, color_row):
                    s = "-" if s is None else s
                    rgb01 = latex_rgb01_from_rgb255(c) if c else None
                    cells.append(latex_color_text(s, rgb01))
                print(" & ".join(cells) + r" \\")
            print(r"\bottomrule")
            print(r"\end{tabular}")
        else:
            latex_headers: List[str] = [latex_makecell_label("µarch")]
            for c in display_cases:
                latex_headers.append(latex_makecell_label(case_label(c)))
                if c == (1, 1, 0, 0, 0):
                    latex_headers.append(latex_makecell_label("tran jump works"))
            latex_headers.append(r"\bbtwo")

            emit_latex_table(latex_headers, latex_rows_text, latex_rows_colors,
                             caption=args.latex_caption, label=args.latex_label)

        lo_r, lo_g, lo_b = latex_rgb01_from_rgb255(purple_to_red(nonlinearize(0.0, args.gamma)))
        mid_r, mid_g, mid_b = latex_rgb01_from_rgb255(purple_to_red(nonlinearize(0.5, args.gamma)))
        hi_r, hi_g, hi_b = latex_rgb01_from_rgb255(purple_to_red(nonlinearize(1.0, args.gamma)))
        mode = "%Δ vs baseline (baseline column hidden)" if args.pct_baseline else "raw"
        print(f"% Per-row text color scale (gamma={args.gamma:.2f}, mode={mode}): "
              f"low=({lo_r:.2f},{lo_g:.2f},{lo_b:.2f}) -> mid=({mid_r:.2f},{mid_g:.2f},{mid_b:.2f}) -> "
              f"high=({hi_r:.2f},{hi_g:.2f},{hi_b:.2f}).")
        print("% NOTE: Percent cells use \\SI{...}{\\percent} (siunitx).")
        if args.full_columns:
            print("% NOTE: 'tran jump works' is |tran jump - jump| <= 10 (raw) or <= 15 (%% mode).")
        print("% NOTE: \\bbtwo is '(tran dep jump - 10) > tran dep cached jump' "
              "(applied on raw or %% values matching the table mode).")
        print("% NOTE: \\cmark should be defined in LaTeX or replace with \\checkmark (amssymb).")
        return

    widths = compute_col_widths(headers, plain_rows)
    print("  ".join(h + " " * (widths[i] - len(h)) for i, h in enumerate(headers)))
    print("  ".join("-" * w for w in widths))

    for r_plain, r_colors in zip(plain_rows, color_rows):
        print("  ".join(pad_and_color(cell, widths[i], col) for i, (cell, col) in enumerate(zip(r_plain, r_colors))))

    if not args.no_color:
        lo = rgb_ansi(*green_to_red(nonlinearize(0.0, args.gamma)), "low")
        mid = rgb_ansi(*green_to_red(nonlinearize(0.5, args.gamma)), "mid")
        hi = rgb_ansi(*green_to_red(nonlinearize(1.0, args.gamma)), "high")
        mode = "%Δ vs baseline (baseline column hidden)" if args.pct_baseline else "raw"
        print(f"\nPer-row color scale (gamma={args.gamma:.2f}, mode={mode}): {lo} \u2192 {mid} \u2192 {hi} (min \u2192 mid \u2192 max).")
    else:
        print("\nPer-row scale: min → max (colors disabled).")


if __name__ == "__main__":
    main()
