#!/usr/bin/env python3
"""Export manuscript-ready PNG charts from the generated benchmark CSV files."""
from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data"
OUT = ROOT / "figures"
OUT.mkdir(exist_ok=True)

def save(fig, name):
    fig.tight_layout()
    fig.savefig(OUT / name, dpi=300, bbox_inches="tight")
    plt.close(fig)

r = pd.read_csv(DATA / "results.csv")
# Collapse across hash functions only for presentation; preserve raw stratified CSV.
probe = r.groupby(["LoadFactor", "CollisionMethod"], as_index=False)[["UnsuccessfulMeanProbes", "UnsuccessfulP50Probes", "UnsuccessfulP95Probes", "UnsuccessfulMaxProbes"]].mean()
fig, ax = plt.subplots(figsize=(10, 6))
for method, g in probe.groupby("CollisionMethod"):
    ax.plot(g["LoadFactor"], g["UnsuccessfulP95Probes"], marker="o", label=method)
ax.set(title="Unsuccessful-search P95 probe count", xlabel="Load factor", ylabel="P95 probes")
ax.legend(fontsize=7, ncol=2)
ax.grid(True, alpha=.25)
save(fig, "fig01_unsuccessful_probe_p95.png")

fig, ax = plt.subplots(figsize=(11, 6))
for method, g in r.groupby("CollisionMethod"):
    agg = g.groupby("LoadFactor", as_index=False).agg(median=("InsertionTimeMedianMs", "median"), ci=("InsertionTimeCI95Ms", "median"))
    ax.errorbar(agg["LoadFactor"], agg["median"], yerr=agg["ci"], marker="o", capsize=2, label=method)
ax.set(title="Insertion batch median time with bootstrap interval", xlabel="Load factor", ylabel="Milliseconds")
ax.legend(fontsize=7, ncol=2)
ax.grid(True, alpha=.25)
save(fig, "fig02_insertion_timing.png")

ab = pd.read_csv(DATA / "ablation_results.csv")
fig, ax = plt.subplots(figsize=(11, 6))
ax.bar(ab["Variant"], ab["IdentifierOnlyMedianMs"], yerr=ab["IdentifierOnlyCI95Ms"], capsize=3)
ax.set(title="Hybrid component ablation: identifier-only insertion", xlabel="Variant", ylabel="Milliseconds per insertion batch")
ax.tick_params(axis="x", labelrotation=25)
ax.grid(True, axis="y", alpha=.25)
save(fig, "fig03_ablation_identifier_insert.png")

fig, ax = plt.subplots(figsize=(9, 5))
h = pd.read_csv(DATA / "hash_timing.csv")
ax.bar(h["HashFunction"], h["MedianNanosecondsPerHash"], yerr=h["CI95Ms"] * 1e6 / h["KeysPerRun"], capsize=3)
ax.set(title="Standalone hash computation cost", xlabel="Hash function", ylabel="Nanoseconds per key")
ax.tick_params(axis="x", labelrotation=15)
ax.grid(True, axis="y", alpha=.25)
save(fig, "fig04_hash_timing.png")

q = pd.read_csv(DATA / "hash_quality.csv")
fig, ax = plt.subplots(figsize=(9, 5))
ax.bar(q["HashFunction"], q["ChiSquareStatistic"])
ax.set(title="Bucket-count chi-square diagnostic", xlabel="Hash function", ylabel="Chi-square statistic")
ax.tick_params(axis="x", labelrotation=15)
ax.grid(True, axis="y", alpha=.25)
save(fig, "fig05_hash_quality_chisquare.png")

b = pd.read_csv(DATA / "baseline_results.csv")
mb = pd.read_csv(DATA / "modern_baseline_results.csv")
custom = r.groupby("LoadFactor", as_index=False).agg(CustomMedianMs=("InsertionTimeMedianMs", "median"))
fig, ax = plt.subplots(figsize=(9, 5))
ax.plot(custom["LoadFactor"], custom["CustomMedianMs"], marker="o", label="Custom open-addressing median across configurations")
ax.plot(b["LoadFactor"], b["InsertionMedianMs"], marker="o", label="std::unordered_map")
valid = mb[mb["Baseline"] != "UNAVAILABLE"]
if not valid.empty:
    ax.plot(valid["LoadFactor"], valid["InsertionMedianMs"], marker="o", label="Boost.UnorderedFlatMap")
ax.set(title="Insertion timing: custom tables and library baselines", xlabel="Target load factor", ylabel="Milliseconds per batch")
ax.legend(fontsize=8)
ax.grid(True, alpha=.25)
save(fig, "fig06_modern_baselines.png")

fig, ax = plt.subplots(figsize=(10, 6))
for method, g in r.groupby("CollisionMethod"):
    agg = g.groupby("LoadFactor", as_index=False).agg(peak=("PeakTableArrayBytes", "median"))
    ax.plot(agg["LoadFactor"], agg["peak"], marker="o", label=method)
ax.set(title="Estimated peak table-array memory during rehash", xlabel="Load factor", ylabel="Bytes (old + new slot arrays)")
ax.legend(fontsize=7, ncol=2)
ax.grid(True, alpha=.25)
save(fig, "fig07_peak_table_array_memory.png")

st = pd.read_csv(DATA / "source_trace_benchmark.csv")
fig, ax = plt.subplots(figsize=(10, 5))
ax.bar(st["CollisionMethod"], st["MedianTraceMs"], yerr=st["CI95Ms"], capsize=3)
ax.set(title="Repository-derived scoped identifier trace replay", xlabel="Collision strategy", ylabel="Milliseconds per trace replay")
ax.tick_params(axis="x", labelrotation=25)
ax.grid(True, axis="y", alpha=.25)
save(fig, "fig08_source_trace_replay.png")

loads = sorted(r["LoadFactor"].unique())
model = pd.DataFrame({"LoadFactor": loads})
model["Uniform hashing miss reference"] = 1 / (1 - model["LoadFactor"])
hybrid = r[r["CollisionMethod"] == "Adaptive Quadratic-to-Robin-Hood"].groupby("LoadFactor", as_index=False).agg(ObservedMean=("UnsuccessfulMeanProbes", "mean"), ObservedP95=("UnsuccessfulP95Probes", "mean"))
fig, ax = plt.subplots(figsize=(9, 5))
ax.plot(model["LoadFactor"], model["Uniform hashing miss reference"], marker="o", label="Uniform-hashing reference E[miss probes]")
ax.plot(hybrid["LoadFactor"], hybrid["ObservedMean"], marker="o", label="Hybrid observed mean")
ax.plot(hybrid["LoadFactor"], hybrid["ObservedP95"], marker="o", label="Hybrid observed P95")
ax.set(title="Adaptive hybrid vs uniform-hashing reference", xlabel="Load factor", ylabel="Expected / observed unsuccessful-search probes")
ax.legend(fontsize=8)
ax.grid(True, alpha=.25)
save(fig, "fig09_hybrid_uniform_reference.png")

print(f"Wrote {len(list(OUT.glob('*.png')))} PNG figures to {OUT}")
