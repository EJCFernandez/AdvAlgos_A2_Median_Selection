#!/usr/bin/env python3
"""Generate every figure and summary table in the report from the CSVs in results/.

Written by Claude (AI) as infrastructure; reviewed by Ethan Fernandez. Disclosed in ai-log.md.

Usage:  python3 scripts/plot.py [results_dir]

Reads   results/<mode>_time.csv   (timing build: use its times)
        results/<mode>_count.csv  (counted build: use its comparison counts)
for mode in sweep, groups, fix, tail, dups.

Writes  results/figures/fig1_sweep_time.png        time per element vs n, per input family
        results/figures/fig2_sweep_comparisons.png comparisons per element vs n
        results/figures/fig3_time_ratio.png        MoM / quickselect and nth_element / quickselect
        results/figures/fig4_tail.png              quickselect over many seeds vs deterministic baselines
        results/figures/fig5_group_size.png        g = 3, 5, 7
        results/figures/fig6_cost_of_fix.png       three-way / Lomuto, time and comparisons
        results/figures/fig7_duplicates.png        Lomuto vs three-way on duplicate-heavy input (log-log)
        results/summary.csv       median / quartiles of time and median comparisons per configuration
        results/tail_summary.txt  percentiles and ratios for the tail experiment
        results/dups_slopes.csv   local log-log slopes for the duplicates experiment

Every plotted point is the MEDIAN over repetitions; error bars span the 25th-75th
percentiles (interquartile range). Rows with status other than "ok" are excluded.
"""
import csv
import math
import os
import sys
from collections import defaultdict

import numpy as np
import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

RES = sys.argv[1] if len(sys.argv) > 1 else os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "results")
FIG = os.path.join(RES, "figures")
os.makedirs(FIG, exist_ok=True)

ALGO_NAME = {"quickselect": "Quickselect", "mom": "Median-of-medians", "nth_element": "std::nth_element"}
ALGO_COLOUR = {"quickselect": "tab:blue", "mom": "tab:orange", "nth_element": "tab:green"}
G_COLOUR = {"3": "tab:red", "5": "tab:orange", "7": "tab:purple"}
FAMILY_NAME = {"random": "Random", "few_distinct": "Few distinct (0-3)", "all_equal": "All equal",
               "sorted": "Sorted", "reverse": "Reverse sorted"}

QS3 = ("quickselect", "threeway", "")
QSL = ("quickselect", "lomuto", "")
MOM3 = ("mom", "threeway", "5")
MOML = ("mom", "lomuto", "5")
NTH = ("nth_element", "na", "")


# ---------------------------------------------------------------- loading
def load(mode, kind):
    path = os.path.join(RES, f"{mode}_{kind}.csv")
    if not os.path.exists(path):
        print(f"  missing {path}: skipping what depends on it")
        return []
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def key_of(r):
    return (r["algorithm"], r["scheme"], r["g"])


def collect(rows, field):
    """{(series key, family, n): [values]} over rows with status ok and a value in `field`."""
    d = defaultdict(list)
    for r in rows:
        if r["status"] == "ok" and r[field] != "":
            d[(key_of(r), r["family"], int(r["n"]))].append(float(r[field]))
    return d


def capped_sizes(rows):
    d = defaultdict(list)
    for r in rows:
        if r["status"] == "capped":
            d[(key_of(r), r["family"])].append(int(r["n"]))
    return d


def med_iqr(vals):
    a = np.asarray(vals, dtype=float)
    return float(np.median(a)), float(np.percentile(a, 25)), float(np.percentile(a, 75))


def label(key, with_scheme=True):
    a, s, g = key
    text = ALGO_NAME[a]
    if a == "mom" and g:
        text += f" (g={g})"
    if with_scheme and s in ("lomuto", "threeway"):
        text += ", Lomuto" if s == "lomuto" else ", three-way"
    return text


def style(key):
    a, s, g = key
    return dict(color=ALGO_COLOUR[a], linestyle="--" if s == "lomuto" else "-")


def sizes_for(d, key, family):
    return sorted(n for (k, f, n) in d if k == key and f == family)


def families_in(d):
    order = ["random", "few_distinct", "sorted", "all_equal", "reverse"]
    present = {f for (_, f, _) in d}
    return [f for f in order if f in present]


def save(fig, name):
    path = os.path.join(FIG, name)
    fig.tight_layout()
    fig.savefig(path, dpi=200)
    plt.close(fig)
    print(f"  wrote {path}")


def series_panel(ax, d, family, keys, per_element, colour_by_g=False, with_scheme=True):
    for key in keys:
        ns = sizes_for(d, key, family)
        if not ns:
            continue
        med, lo, hi = [], [], []
        for n in ns:
            m, p25, p75 = med_iqr(d[(key, family, n)])
            s = n if per_element else 1.0
            med.append(m / s)
            lo.append((m - p25) / s)
            hi.append((p75 - m) / s)
        st = style(key)
        if colour_by_g:
            st["color"] = G_COLOUR.get(key[2], "black")
        ax.errorbar(ns, med, yerr=[lo, hi], marker="o", ms=3, capsize=2, lw=1.4,
                    label=label(key, with_scheme), **st)
    ax.set_xscale("log")
    ax.grid(True, which="both", alpha=0.3)


# ---------------------------------------------------------------- summary.csv
summary_rows = []


def add_summary(experiment, t, c):
    keys = set(k[:3] for k in t) | set(k[:3] for k in c)
    for (key, fam, n) in sorted(keys, key=lambda x: (x[0], x[1], x[2])):
        row = {"experiment": experiment, "algorithm": key[0], "scheme": key[1], "g": key[2],
               "family": fam, "n": n}
        if (key, fam, n) in t:
            m, p25, p75 = med_iqr(t[(key, fam, n)])
            row.update(reps=len(t[(key, fam, n)]), median_time_ns=f"{m:.1f}", p25_time_ns=f"{p25:.1f}",
                       p75_time_ns=f"{p75:.1f}", median_ns_per_element=f"{m / n:.4f}")
        if (key, fam, n) in c:
            mc = float(np.median(c[(key, fam, n)]))
            row.update(median_comparisons=f"{mc:.1f}", comparisons_per_element=f"{mc / n:.4f}")
        summary_rows.append(row)


# ---------------------------------------------------------------- figures 1-3: sweep
def sweep():
    t = collect(load("sweep", "time"), "time_ns")
    c = collect(load("sweep", "count"), "comparisons")
    add_summary("sweep", t, c)
    keys = [QS3, MOM3, NTH]

    for data, name, ylabel in ((t, "fig1_sweep_time.png", "Median time per element (ns)"),
                               (c, "fig2_sweep_comparisons.png", "Comparisons per element")):
        fams = families_in(data)
        if not fams:
            continue
        fig, axes = plt.subplots(1, len(fams), figsize=(4.6 * len(fams), 4), sharey=True, squeeze=False)
        for ax, fam in zip(axes[0], fams):
            series_panel(ax, data, fam, keys, per_element=True, with_scheme=False)
            ax.set_title(FAMILY_NAME.get(fam, fam))
            ax.set_xlabel("n")
        axes[0][0].set_ylabel(ylabel)
        axes[0][0].legend(fontsize=8)
        fig.suptitle("Size sweep, k = n/2, three-way partitioning" if "time" in name
                     else "Size sweep: comparisons (counted build), k = n/2")
        save(fig, name)

    # Figure 3: time ratios relative to quickselect, per family.
    fams = families_in(t)
    if fams:
        fig, axes = plt.subplots(1, 2, figsize=(10, 4), sharey=False)
        for ax, other, title in ((axes[0], MOM3, "Median-of-medians / quickselect"),
                                 (axes[1], NTH, "std::nth_element / quickselect")):
            for fam in fams:
                ns = [n for n in sizes_for(t, QS3, fam) if (other, fam, n) in t]
                if not ns:
                    continue
                r = [med_iqr(t[(other, fam, n)])[0] / med_iqr(t[(QS3, fam, n)])[0] for n in ns]
                ax.plot(ns, r, marker="o", ms=3, label=FAMILY_NAME.get(fam, fam))
            ax.axhline(1.0, color="black", lw=0.8)
            if other == MOM3:
                ax.axhline(2.0, color="grey", lw=1, ls=":", label="Prediction 1 (2x)")
            ax.set_xscale("log")
            ax.set_xlabel("n")
            ax.set_ylabel("Ratio of median times")
            ax.set_title(title)
            ax.grid(True, which="both", alpha=0.3)
            ax.legend(fontsize=8)
        save(fig, "fig3_time_ratio.png")


# ---------------------------------------------------------------- figure 4: tail
def pct(a, q):
    return float(np.percentile(a, q))


def tail():
    tr = load("tail", "time")
    cr = load("tail", "count")
    if not tr:
        return
    by = defaultdict(list)
    for r in tr:
        if r["status"] == "ok":
            by[key_of(r)].append(float(r["time_ns"]))
    byc = defaultdict(list)
    for r in cr:
        if r["status"] == "ok" and r["comparisons"] != "":
            byc[key_of(r)].append(float(r["comparisons"]))
    n = int(tr[0]["n"])

    lines = [f"Tail experiment: one fixed random input, n = {n}, k = n/2", ""]

    def describe(name, a, unit_scale, unit):
        a = np.asarray(a, dtype=float)
        med = np.median(a)
        constant = float(np.ptp(a)) == 0.0
        sk = float(((a - a.mean()) ** 3).mean() / (a.std() ** 3)) if not constant else float("nan")
        lines.append(f"{name}: runs={len(a)}")
        lines.append(f"  median={med / unit_scale:.4f}{unit} mean={a.mean() / unit_scale:.4f}{unit} "
                     f"min={a.min() / unit_scale:.4f}{unit} max={a.max() / unit_scale:.4f}{unit}")
        lines.append(f"  p90/median={pct(a, 90) / med:.3f} p99/median={pct(a, 99) / med:.3f} "
                     f"p99.9/median={pct(a, 99.9) / med:.3f} max/median={a.max() / med:.3f} "
                     + (f"skewness={sk:.3f}" if not math.isnan(sk) else "skewness=n/a (every run identical)"))
        if len(a) < 5000:
            lines.append(f"  note: with {len(a)} runs, p99.9 rests on about {max(1, len(a) // 1000)} run(s) "
                         f"above it, so treat it as indicative only")

    for key in (QS3, MOM3, NTH):
        if key in by:
            describe(f"time, {label(key)}", by[key], 1e6, " ms")
    lines.append("")
    for key in (QS3, MOM3, NTH):
        if key in byc:
            describe(f"comparisons per element, {label(key)}", np.asarray(byc[key]) / n, 1.0, "")
    with open(os.path.join(RES, "tail_summary.txt"), "w") as f:
        f.write("\n".join(lines) + "\n")
    print(f"  wrote {os.path.join(RES, 'tail_summary.txt')}")

    fig, axes = plt.subplots(1, 2, figsize=(11, 4))
    ax = axes[0]
    if QS3 in by:
        q = np.asarray(by[QS3]) / np.median(by[QS3])
        ax.hist(q, bins=60, color=ALGO_COLOUR["quickselect"], alpha=0.75,
                label=f"Quickselect ({len(q)} pivot seeds)")
        for p, ls in ((99, "--"), (99.9, ":")):
            ax.axvline(pct(q, p), color="black", ls=ls, lw=1, label=f"p{p:g} = {pct(q, p):.2f}x median")
        ax.axvline(q.max(), color="red", lw=1, label=f"max = {q.max():.2f}x median")
    for key in (MOM3, NTH):
        if key in by:
            b = np.asarray(by[key]) / np.median(by[key])
            ax.hist(b, bins=30, histtype="step", lw=1.5, color=ALGO_COLOUR[key[0]],
                    label=f"{label(key, False)} ({len(b)} runs, same input)")
    ax.set_xlabel("Time / that algorithm's median time")
    ax.set_ylabel("Runs")
    ax.set_title("Run-to-run variation in time")
    ax.legend(fontsize=7)
    ax.grid(True, alpha=0.3)

    ax = axes[1]
    if QS3 in byc:
        qc = np.asarray(byc[QS3]) / n
        ax.hist(qc, bins=60, color=ALGO_COLOUR["quickselect"], alpha=0.75, label="Quickselect")
        ax.axvline(np.median(qc), color="black", lw=1, label=f"median = {np.median(qc):.2f}")
    for key in (MOM3, NTH):
        if key in byc:
            v = float(np.median(byc[key])) / n
            ax.axvline(v, color=ALGO_COLOUR[key[0]], lw=2, label=f"{label(key, False)} = {v:.2f} (fixed)")
    ax.set_xlabel("Comparisons per element (counted build)")
    ax.set_ylabel("Runs")
    ax.set_title("Run-to-run variation in comparisons")
    ax.legend(fontsize=7)
    ax.grid(True, alpha=0.3)
    fig.suptitle(f"Tail experiment: one fixed random input, n = {n}, three-way partitioning")
    save(fig, "fig4_tail.png")


# ---------------------------------------------------------------- figure 5: group size
def groups():
    t = collect(load("groups", "time"), "time_ns")
    c = collect(load("groups", "count"), "comparisons")
    add_summary("groups", t, c)
    keys = [("mom", "threeway", g) for g in ("3", "5", "7")]
    fig, axes = plt.subplots(1, 2, figsize=(10, 4))
    for ax, data, ylabel in ((axes[0], t, "Median time per element (ns)"),
                             (axes[1], c, "Comparisons per element")):
        series_panel(ax, data, "random", keys, per_element=True, colour_by_g=True, with_scheme=False)
        ax.set_xlabel("n")
        ax.set_ylabel(ylabel)
        ax.legend(fontsize=8)
    fig.suptitle("Median-of-medians group size, random input, three-way partitioning")
    save(fig, "fig5_group_size.png")


# ---------------------------------------------------------------- figure 6: cost of the fix
def fix():
    t = collect(load("fix", "time"), "time_ns")
    c = collect(load("fix", "count"), "comparisons")
    add_summary("fix", t, c)
    fig, axes = plt.subplots(1, 2, figsize=(10, 4), sharey=True)
    for ax, data, title in ((axes[0], t, "Time"), (axes[1], c, "Comparisons")):
        for three, lom in ((QS3, QSL), (MOM3, MOML)):
            ns = [n for n in sizes_for(data, three, "random") if (lom, "random", n) in data]
            if not ns:
                continue
            r = [med_iqr(data[(three, "random", n)])[0] / med_iqr(data[(lom, "random", n)])[0] for n in ns]
            ax.plot(ns, r, marker="o", ms=3, color=ALGO_COLOUR[three[0]], label=label(three, False))
        ax.axhline(1.0, color="black", lw=0.8)
        ax.set_xscale("log")
        ax.set_xlabel("n")
        ax.set_title(f"{title}: three-way / Lomuto")
        ax.grid(True, which="both", alpha=0.3)
        ax.legend(fontsize=8)
    axes[0].set_ylabel("Ratio of medians")
    fig.suptitle("Cost of the fix on random input (values above 1 mean three-way costs more)")
    save(fig, "fig6_cost_of_fix.png")


# ---------------------------------------------------------------- figure 7: duplicates
def dups():
    tr = load("dups", "time")
    cr = load("dups", "count")
    t = collect(tr, "time_ns")
    c = collect(cr, "comparisons")
    add_summary("dups", t, c)
    capped_t = capped_sizes(tr)
    keys = [QSL, QS3, MOML, MOM3]
    fams = [f for f in ("few_distinct", "all_equal") if f in families_in(t) or f in families_in(c)]
    if not fams:
        return

    slope_rows = []
    fig, axes = plt.subplots(len(fams), 2, figsize=(11, 4.2 * len(fams)), squeeze=False)
    for i, fam in enumerate(fams):
        for j, (data, ylabel, metric) in enumerate(((t, "Median time (ns)", "time"),
                                                    (c, "Comparisons", "comparisons"))):
            ax = axes[i][j]
            for key in keys:
                ns = sizes_for(data, key, fam)
                if not ns:
                    continue
                med = [med_iqr(data[(key, fam, n)])[0] for n in ns]
                lab = label(key)
                if metric == "time" and capped_t.get((key, fam)):
                    lab += f" (capped above n={max(ns)})"
                ax.plot(ns, med, marker="o", ms=3, label=lab, **style(key))
                for (n1, v1), (n2, v2) in zip(zip(ns, med), zip(ns[1:], med[1:])):
                    if v1 > 0 and v2 > 0:
                        slope_rows.append({"family": fam, "metric": metric, "series": label(key),
                                           "n_from": n1, "n_to": n2,
                                           "local_slope": f"{math.log(v2 / v1) / math.log(n2 / n1):.3f}"})
            # Reference slopes, anchored below the lowest series at the smallest size so
            # they never sit on top of a data line.
            firsts = [(sizes_for(data, k, fam)[0], med_iqr(data[(k, fam, sizes_for(data, k, fam)[0])])[0])
                      for k in keys if sizes_for(data, k, fam)]
            if firsts:
                n0 = min(n for n, _ in firsts)
                v0 = min(v for n, v in firsts if n == n0) / 3.0
                xs = np.array([n0, max(n for (_, f, n) in data if f == fam)], dtype=float)
                ax.plot(xs, v0 * (xs / n0), color="grey", ls=":", lw=1, label="slope 1 (linear)")
                ax.plot(xs, v0 * (xs / n0) ** 2, color="black", ls=":", lw=1, label="slope 2 (quadratic)")
            ax.set_xscale("log")
            ax.set_yscale("log")
            ax.set_xlabel("n")
            ax.set_ylabel(ylabel)
            ax.set_title(f"{FAMILY_NAME.get(fam, fam)}: {metric}")
            ax.grid(True, which="both", alpha=0.3)
            ax.legend(fontsize=7)
    fig.suptitle("Duplicate-heavy input: Lomuto (dashed) vs three-way (solid), k = n/2, log-log")
    save(fig, "fig7_duplicates.png")

    path = os.path.join(RES, "dups_slopes.csv")
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=["family", "metric", "series", "n_from", "n_to", "local_slope"],
                           lineterminator="\n")
        w.writeheader()
        w.writerows(slope_rows)
    print(f"  wrote {path}  (a local slope that keeps rising with n means faster than any fixed polynomial)")


def main():
    print(f"Reading CSVs from {os.path.abspath(RES)}")
    for step in (sweep, tail, groups, fix, dups):
        step()
    path = os.path.join(RES, "summary.csv")
    fields = ["experiment", "algorithm", "scheme", "g", "family", "n", "reps", "median_time_ns", "p25_time_ns",
              "p75_time_ns", "median_ns_per_element", "median_comparisons", "comparisons_per_element"]
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields, lineterminator="\n")
        w.writeheader()
        w.writerows(summary_rows)
    print(f"  wrote {path}")


if __name__ == "__main__":
    main()
