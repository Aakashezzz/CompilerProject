"""Research evidence dashboard: visualizes benchmark CSVs produced by the C++ backend."""
import os
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
import streamlit as st
from utils import inject_css

st.set_page_config(page_title="Research Evidence", page_icon="🧪", layout="wide")
inject_css()
BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DATA_DIR = os.path.abspath(os.path.join(BASE_DIR, "..", "data"))

def read_csv(name):
    path = os.path.join(DATA_DIR, name)
    if not os.path.exists(path):
        return pd.DataFrame()
    try:
        return pd.read_csv(path)
    except (OSError, pd.errors.ParserError) as exc:
        st.warning(f"Could not read {name}: {exc}")
        return pd.DataFrame()

results = read_csv("results.csv")
baseline = read_csv("baseline_results.csv")
modern_baseline = read_csv("modern_baseline_results.csv")
quality = read_csv("hash_quality.csv")

st.title("Research Evidence & Statistical Results")
st.caption("Charts below are generated from the CSV output of the C++ benchmark. Re-run the backend on the machine used for your paper before citing timings.")
if results.empty:
    st.error("Benchmark results are missing. Compile and run the C++ backend from the repository root first.")
    st.stop()

# Keep filters available across all sections.
with st.expander("Filter benchmark configurations", expanded=True):
    c1, c2, c3 = st.columns(3)
    hashes = c1.multiselect("Hash function", sorted(results["HashFunction"].dropna().unique()), default=sorted(results["HashFunction"].dropna().unique()))
    methods = c2.multiselect("Collision strategy", sorted(results["CollisionMethod"].dropna().unique()), default=sorted(results["CollisionMethod"].dropna().unique()))
    loads = c3.multiselect("Target load factor", sorted(results["LoadFactor"].dropna().unique()), default=sorted(results["LoadFactor"].dropna().unique()))
filtered = results[results.HashFunction.isin(hashes) & results.CollisionMethod.isin(methods) & results.LoadFactor.isin(loads)].copy()
if filtered.empty:
    st.info("Choose at least one hash function, collision strategy, and load factor.")
    st.stop()

c1, c2, c3, c4 = st.columns(4)
c1.metric("Configurations shown", f"{len(filtered):,}")
reps = int(filtered["Repetitions"].min()) if "Repetitions" in filtered else 0
c2.metric("Repetitions per configuration", str(reps) if reps else "Not recorded")
c3.metric("Failed insertions", f"{int(filtered['FailedInserts'].sum()):,}" if "FailedInserts" in filtered else "N/A")
c4.metric("Failed successful-lookups", f"{int(filtered['FailedSearches'].sum()):,}" if "FailedSearches" in filtered else "N/A")

st.header("1. Unsuccessful-search probe distributions")
st.write("Each missing-key lookup records its probe count. P50 is the median, P95 is the 95th percentile, and maximum is the longest observed miss in the benchmark workload.")
probe_cols = [c for c in ["UnsuccessfulMeanProbes", "UnsuccessfulP50Probes", "UnsuccessfulP95Probes", "UnsuccessfulMaxProbes"] if c in filtered]
if probe_cols:
    p = filtered.groupby(["LoadFactor", "CollisionMethod"], as_index=False)[probe_cols].mean()
    measure = st.selectbox("Probe statistic", probe_cols, index=min(2, len(probe_cols)-1), key="probe_stat")
    fig = px.line(p, x="LoadFactor", y=measure, color="CollisionMethod", markers=True, title=f"{measure} by load factor")
    fig.update_layout(xaxis_title="Target load factor", yaxis_title="Probes per unsuccessful search")
    st.plotly_chart(fig, use_container_width=True)
    st.dataframe(p, use_container_width=True, hide_index=True)
else:
    st.warning("Unsuccessful-search distribution columns are absent from results.csv. Re-run the updated backend.")

st.header("2. Timing medians and approximate 95% confidence intervals")
st.write("The point estimate is the sample median runtime. The reported interval is a deterministic percentile-bootstrap 95% confidence interval for the median, stored as a conservative symmetric half-width for chart error bars.")
time_specs = [
    ("InsertionTimeMedianMs", "InsertionTimeCI95Ms", "Insertion batch time (ms)"),
    ("SuccessfulSearchMedianMs", "SuccessfulSearchCI95Ms", "Successful-search batch time (ms)"),
    ("UnsuccessfulSearchMedianMs", "UnsuccessfulSearchCI95Ms", "Unsuccessful-search batch time (ms)"),
]
available_specs = [x for x in time_specs if x[0] in filtered.columns]
if available_specs:
    labels = [x[2] for x in available_specs]
    chosen_label = st.selectbox("Timing measure", labels, key="timing_measure")
    med_col, ci_col, _ = next(x for x in available_specs if x[2] == chosen_label)
    t = filtered.copy()
    t["Configuration"] = t["HashFunction"].astype(str) + " | " + t["CollisionMethod"].astype(str)
    fig = px.bar(t, x="Configuration", y=med_col, color="LoadFactor", barmode="group", title=f"{chosen_label} with approximate 95% CI")
    if ci_col in t.columns:
        fig.update_traces(error_y=dict(type="data", array=t[ci_col].fillna(0), visible=True))
    fig.update_layout(xaxis_tickangle=-45, yaxis_title="Milliseconds per batch", xaxis_title="Hash function | collision strategy")
    st.plotly_chart(fig, use_container_width=True)
else:
    st.warning("Timing median/CI columns are missing. Re-run the updated backend.")

st.header("3. Modern standard-library baseline")
if not baseline.empty and "Baseline" in baseline.columns:
    custom = filtered.groupby("LoadFactor", as_index=False).agg(
        CustomInsertionMedianMs=("InsertionTimeMedianMs", "median"),
        CustomSuccessfulSearchMedianMs=("SuccessfulSearchMedianMs", "median"),
        CustomUnsuccessfulSearchMedianMs=("UnsuccessfulSearchMedianMs", "median"),
    )
    b = baseline.copy()
    b = b.rename(columns={"InsertionMedianMs":"InsertionTimeMedianMs", "SuccessfulSearchMedianMs":"SuccessfulSearchMedianMs", "UnsuccessfulSearchMedianMs":"UnsuccessfulSearchMedianMs"})
    metric = st.selectbox("Baseline timing metric", ["InsertionTimeMedianMs", "SuccessfulSearchMedianMs", "UnsuccessfulSearchMedianMs"], format_func=lambda x: {"InsertionTimeMedianMs":"Insertion batch", "SuccessfulSearchMedianMs":"Successful-search batch", "UnsuccessfulSearchMedianMs":"Unsuccessful-search batch"}[x], key="baseline_metric")
    c = filtered.groupby("LoadFactor", as_index=False)[metric].median().rename(columns={metric:"Custom open-addressing median (ms)"})
    if metric in b.columns:
        c = c.merge(b[["LoadFactor", metric]].rename(columns={metric:"std::unordered_map median (ms)"}), on="LoadFactor", how="left")
        melted = c.melt(id_vars="LoadFactor", var_name="Implementation", value_name="Batch time (ms)")
        fig = px.line(melted, x="LoadFactor", y="Batch time (ms)", color="Implementation", markers=True, title="Custom table vs std::unordered_map")
        st.plotly_chart(fig, use_container_width=True)
        st.caption("Custom result is the median across selected custom configurations at each load factor; the standard-library baseline is measured separately. This is a timing comparison, not a comparable probe-count comparison.")
    else:
        st.warning("Expected timing columns are missing from baseline_results.csv.")
    st.dataframe(baseline, use_container_width=True, hide_index=True)
else:
    st.warning("baseline_results.csv is missing or has an unsupported schema. Re-run the updated backend.")

st.header("4. Hash-function bucket distribution")
if not quality.empty and {"HashFunction", "OccupiedBuckets", "CollisionPairs", "MaxBucketOccupancy", "Keys", "Buckets"}.issubset(quality.columns):
    q1, q2 = st.columns(2)
    with q1:
        st.plotly_chart(px.bar(quality, x="HashFunction", y="CollisionPairs", title="Pairwise collisions for fixed key set"), use_container_width=True)
    with q2:
        st.plotly_chart(px.bar(quality, x="HashFunction", y="MaxBucketOccupancy", title="Maximum bucket occupancy"), use_container_width=True)
    st.dataframe(quality, use_container_width=True, hide_index=True)
    st.caption("This is one deterministic identifier set and bucket count; it is a distribution diagnostic, not a proof of general hash quality or cryptographic security.")
else:
    st.warning("hash_quality.csv is missing or has an unsupported schema. Re-run the updated backend.")

st.header("5. Memory estimate and adaptive rehashing")
if "TableArrayBytes" in filtered.columns:
    m = filtered.groupby(["LoadFactor", "CollisionMethod"], as_index=False).agg(TableArrayBytes=("TableArrayBytes", "median"), RehashCount=("RehashCount", "mean") if "RehashCount" in filtered.columns else ("TableArrayBytes", "count"))
    a, b = st.columns(2)
    with a:
        fig = px.line(m, x="LoadFactor", y="TableArrayBytes", color="CollisionMethod", markers=True, title="Estimated contiguous table-array bytes")
        fig.update_yaxes(title="Bytes (TableEntry vector only)")
        st.plotly_chart(fig, use_container_width=True)
    with b:
        fig = px.bar(m, x="LoadFactor", y="RehashCount", color="CollisionMethod", barmode="group", title="Rehash count reported by benchmark")
        st.plotly_chart(fig, use_container_width=True)
    st.caption("TableArrayBytes is the contiguous slot-array estimate. PeakTableArrayBytes estimates simultaneous old/new slot arrays during rehash; it excludes string heap allocations, allocator overhead, and process RSS. BytesPerSlot is sizeof(TableEntry) in this build, not total process memory.")
    if "BytesPerSlot" in filtered.columns:
        st.write("Slot-layout size (C++ sizeof(TableEntry)):", int(filtered["BytesPerSlot"].median()), "bytes per slot")
    if {"PeakTableArrayBytes", "RehashDurationMs"}.issubset(filtered.columns):
        mem = filtered.groupby(["LoadFactor", "CollisionMethod"], as_index=False).agg(
            PeakTableArrayBytes=("PeakTableArrayBytes", "median"),
            RehashDurationMs=("RehashDurationMs", "median"))
        a, b = st.columns(2)
        with a:
            st.plotly_chart(px.line(mem, x="LoadFactor", y="PeakTableArrayBytes", color="CollisionMethod",
                                    markers=True, title="Estimated peak slot-array bytes during rehash"), use_container_width=True)
        with b:
            st.plotly_chart(px.bar(mem, x="LoadFactor", y="RehashDurationMs", color="CollisionMethod",
                                   barmode="group", title="Measured rehash duration (per final repetition)"), use_container_width=True)
else:
    st.info("Table-array memory estimates are not available in this CSV.")

st.header("6. Correctness counters and raw data")
check_cols = [c for c in ["HashFunction", "CollisionMethod", "LoadFactor", "Inserted", "FailedInserts", "FailedSearches", "RehashCount"] if c in filtered.columns]
if check_cols:
    st.dataframe(filtered[check_cols], use_container_width=True, hide_index=True)
for label, frame, filename in [("Download filtered benchmark results", filtered, "filtered_results.csv"), ("Download full baseline data", baseline, "baseline_results.csv"), ("Download hash-quality data", quality, "hash_quality.csv")]:
    if not frame.empty:
        st.download_button(label, frame.to_csv(index=False).encode("utf-8"), file_name=filename, mime="text/csv")

st.header("7. Analytical reference model (linear probing only)")
st.write("For context, the classical uniform-hashing approximation for linear probing predicts expected successful-search probes of ½(1 + 1/(1−α)) and unsuccessful-search probes of ½(1 + 1/(1−α)²), where α is load factor. These are reference curves under simplifying assumptions, not predictions for every hash function or collision strategy.")
model_loads = sorted([float(x) for x in filtered["LoadFactor"].dropna().unique() if 0 < float(x) < 1])
if model_loads:
    model = pd.DataFrame({"LoadFactor": model_loads})
    model["Linear-probing theory: successful"] = 0.5 * (1 + 1 / (1 - model["LoadFactor"]))
    model["Linear-probing theory: unsuccessful"] = 0.5 * (1 + 1 / ((1 - model["LoadFactor"]) ** 2))
    observed = filtered[filtered["CollisionMethod"] == "Linear Probing"].groupby("LoadFactor", as_index=False).agg(
        ObservedUnsuccessfulP50=("UnsuccessfulP50Probes", "mean"),
        ObservedUnsuccessfulP95=("UnsuccessfulP95Probes", "mean")
    ) if {"UnsuccessfulP50Probes", "UnsuccessfulP95Probes"}.issubset(filtered.columns) else pd.DataFrame()
    fig = go.Figure()
    fig.add_trace(go.Scatter(x=model["LoadFactor"], y=model["Linear-probing theory: successful"], mode="lines+markers", name="Theory: successful search"))
    fig.add_trace(go.Scatter(x=model["LoadFactor"], y=model["Linear-probing theory: unsuccessful"], mode="lines+markers", name="Theory: unsuccessful search"))
    if not observed.empty:
        fig.add_trace(go.Scatter(x=observed["LoadFactor"], y=observed["ObservedUnsuccessfulP50"], mode="markers", name="Observed linear-probing miss P50"))
        fig.add_trace(go.Scatter(x=observed["LoadFactor"], y=observed["ObservedUnsuccessfulP95"], mode="markers", name="Observed linear-probing miss P95"))
    fig.update_layout(title="Classical linear-probing reference curves vs observed miss probes", xaxis_title="Load factor α", yaxis_title="Expected / observed probes")
    st.plotly_chart(fig, use_container_width=True)
    # Uniform-hashing reference requested for the adaptive hybrid. The formula
    # is E[unsuccessful probes] = 1/(1-alpha), under the uniform-probing model.
    hybrid_observed = filtered[filtered["CollisionMethod"] == "Adaptive Quadratic-to-Robin-Hood"].groupby("LoadFactor", as_index=False).agg(
        HybridObservedMissMean=("UnsuccessfulMeanProbes", "mean"),
        HybridObservedMissP95=("UnsuccessfulP95Probes", "mean")
    ) if {"UnsuccessfulMeanProbes", "UnsuccessfulP95Probes"}.issubset(filtered.columns) else pd.DataFrame()
    uniform = pd.DataFrame({"LoadFactor": model_loads})
    uniform["Uniform-hashing expected unsuccessful probes"] = 1 / (1 - uniform["LoadFactor"])
    fig2 = go.Figure()
    fig2.add_trace(go.Scatter(x=uniform["LoadFactor"], y=uniform["Uniform-hashing expected unsuccessful probes"], mode="lines+markers", name="Uniform-hashing reference"))
    if not hybrid_observed.empty:
        fig2.add_trace(go.Scatter(x=hybrid_observed["LoadFactor"], y=hybrid_observed["HybridObservedMissMean"], mode="markers+lines", name="Hybrid observed mean miss probes"))
        fig2.add_trace(go.Scatter(x=hybrid_observed["LoadFactor"], y=hybrid_observed["HybridObservedMissP95"], mode="markers", name="Hybrid observed P95 miss probes"))
    fig2.update_layout(title="Adaptive hybrid vs uniform-hashing unsuccessful-search reference", xaxis_title="Load factor α", yaxis_title="Expected / observed unsuccessful-search probes")
    st.plotly_chart(fig2, use_container_width=True)
    st.caption("The uniform-hashing curve is a reference expectation, not an exact derivation for the hybrid. Observed mean/P95 statistics should not be treated as identical to the theoretical expectation. Discuss the measured quadratic-probing gap only after checking the regenerated CSVs.")

st.header("8. Evidence limits and interpretation")
st.info("The dashboard includes controlled ablations, a source-derived lexical trace, scope/deletion/growth tests, hash timing and distribution diagnostics, and standard-library/optional Boost baselines. The source trace remains a lexical approximation rather than a production compiler trace, and charts do not replace a formal correctness proof or manuscript discussion.")
st.markdown("""
- **Ablation:** keyword filtering, the duplicate-check, and the adaptive switch are varied independently; the duplicate-check-off ablation uses a unique-key-only workload and a benchmark-only unsafe option; never enable that option for general symbol-table use.
- **Compiler workload:** the checked-in trace is extracted from this repository. For stronger evidence, rerun the extractor against a named external C/C++ codebase and disclose its commit/version and license.
- **Scope/deletion/growth:** tests exercise scope shadowing, erase/rebuild and optional growth. The trace's brace depth is approximate, not compiler-grade semantic scope.
- **Hash functions:** MurmurHash3 x64/128 (low 64 bits, seed 0), FNV-1a + avalanche (custom), and XXH64 (seed 0) are compared. Only the canonical algorithms should be described as standard implementations.
- **Modern baseline:** Boost.UnorderedFlatMap is conditional on Boost headers being present at build time; std::unordered_map is always available. No instrumented Clang/GCC internal symbol-table baseline is included.
- **Theory and literature:** the dashboard plots classical reference curves, not an exact derived model for the hybrid. Formal argument, related work, full manuscript references and author metadata must still be completed in the paper.
""")


st.header("8. Ablation study")
ablation = read_csv("ablation_results.csv")
if not ablation.empty:
    st.caption("Identifier-only timing uses the same unique identifiers for every variant. Keyword-attempt timing is isolated on a fresh table, so accepted keyword entries do not contaminate the identifier timing. The duplicate-check-off ablation uses only unique identifiers and a benchmark-only unsafe option; normal symbol-table behavior still rejects duplicates.")
    fig = px.bar(ablation, x="Variant", y="IdentifierOnlyMedianMs", error_y="IdentifierOnlyCI95Ms",
                 title="Ablation: identifier-only insertion time (95% CI)")
    fig.update_layout(xaxis_tickangle=-25, xaxis_title="", yaxis_title="Milliseconds")
    st.plotly_chart(fig, use_container_width=True)
    fig2 = px.bar(ablation, x="Variant", y="KeywordAttemptsMedianMs", error_y="KeywordAttemptsCI95Ms",
                  title="Ablation: keyword-attempt handling time (95% CI)")
    fig2.update_layout(xaxis_tickangle=-25, xaxis_title="", yaxis_title="Milliseconds")
    st.plotly_chart(fig2, use_container_width=True)
    st.dataframe(ablation, use_container_width=True, hide_index=True)
    st.download_button("Download ablation results", ablation.to_csv(index=False).encode("utf-8"),
                       file_name="ablation_results.csv", mime="text/csv")
else:
    st.warning("Ablation data missing. Re-run the C++ backend.")

st.header("10. Hash computation cost and distribution test")
hash_timing = read_csv("hash_timing.csv")
if not hash_timing.empty:
    a, b = st.columns(2)
    with a:
        st.plotly_chart(px.bar(hash_timing, x="HashFunction", y="MedianNanosecondsPerHash",
                               error_y=hash_timing["CI95Ms"] * 1e6 / hash_timing["KeysPerRun"],
                               title="Hash computation time per key"), use_container_width=True)
    with b:
        if "ChiSquareStatistic" in quality.columns:
            st.plotly_chart(px.bar(quality, x="HashFunction", y="ChiSquareStatistic",
                                   title="Bucket-count chi-square statistic (descriptive diagnostic)"),
                            use_container_width=True)
        else:
            st.info("Re-run the backend to regenerate chi-square distribution metrics.")
    st.dataframe(hash_timing, use_container_width=True, hide_index=True)
    st.caption("Hash timing is measured on a fixed generated key set. Chi-square is a descriptive statistic for the tested bucket distribution; do not interpret it as a universal hash-quality verdict without the test assumptions and p-value.")
    st.download_button("Download hash timing results", hash_timing.to_csv(index=False).encode("utf-8"),
                       file_name="hash_timing.csv", mime="text/csv")
else:
    st.warning("Hash timing data missing. Re-run the C++ backend.")

st.header("12. Source-code identifier trace")
trace = read_csv("source_identifier_trace.csv")
if not trace.empty and {"Identifier", "IsKeyword", "ScopeDepthApprox"}.issubset(trace.columns):
    keyword_counts = trace.groupby("IsKeyword", as_index=False).size()
    keyword_counts["TokenClass"] = keyword_counts["IsKeyword"].map({0: "Non-keyword identifier", 1: "C++ keyword"})
    st.plotly_chart(px.bar(keyword_counts, x="TokenClass", y="size",
                           title="Lexical trace: identifiers vs reserved keywords"), use_container_width=True)
    scope_counts = trace.groupby("ScopeDepthApprox", as_index=False).size()
    st.plotly_chart(px.bar(scope_counts, x="ScopeDepthApprox", y="size",
                           title="Identifier-token counts by approximate brace depth"), use_container_width=True)
    st.caption("This trace is extracted from source files in the project repository. Scope depth is brace-based and approximate, not a compiler-grade parse. To study an external compiler workload, rerun scripts/extract_source_trace.py against that source tree.")
    st.download_button("Download source identifier trace", trace.to_csv(index=False).encode("utf-8"),
                       file_name="source_identifier_trace.csv", mime="text/csv")
else:
    st.info("Source trace not available. Run scripts/extract_source_trace.py.")


st.header("11. Modern specialized hash-table baseline")
if not modern_baseline.empty and "Baseline" in modern_baseline.columns:
    available = modern_baseline[modern_baseline["Baseline"] != "UNAVAILABLE"].copy()
    if not available.empty:
        st.plotly_chart(px.line(available, x="LoadFactor", y="InsertionMedianMs", markers=True,
                                title="Boost.UnorderedFlatMap insertion time"), use_container_width=True)
        st.dataframe(available, use_container_width=True, hide_index=True)
        st.download_button("Download modern baseline results", modern_baseline.to_csv(index=False).encode("utf-8"),
                           file_name="modern_baseline_results.csv", mime="text/csv")
    else:
        st.info("Boost.UnorderedFlatMap was not available at build time. The std::unordered_map baseline remains available.")
else:
    st.info("Run the backend to generate modern_baseline_results.csv.")


st.subheader("Source-trace scope/deletion/growth workload")
source_bench = read_csv("source_trace_benchmark.csv")
if not source_bench.empty:
    st.plotly_chart(px.bar(source_bench, x="CollisionMethod", y="MedianTraceMs",
                           error_y="CI95Ms", title="Scope-aware source-token trace replay (median time, 95% CI)"),
                    use_container_width=True)
    c1, c2 = st.columns(2)
    with c1:
        st.plotly_chart(px.bar(source_bench, x="CollisionMethod", y="DynamicRehashes",
                               title="Dynamic rehashes across trace repetitions"), use_container_width=True)
    with c2:
        st.plotly_chart(px.bar(source_bench, x="CollisionMethod", y="FailedOperations",
                               title="Failed operations in source-trace replay"), use_container_width=True)
    st.caption("The trace is extracted from this repository's C/C++ sources. Brace-depth scope reconstruction is approximate; first-seen identifiers in a scope are treated as declarations, later occurrences as lookups. This is reproducible source-derived workload evidence, not a production compiler trace or a full C++ parser.")
    st.dataframe(source_bench, use_container_width=True, hide_index=True)
    st.download_button("Download source-trace benchmark", source_bench.to_csv(index=False).encode("utf-8"),
                       file_name="source_trace_benchmark.csv", mime="text/csv")
else:
    st.info("Run the backend after generating source_identifier_trace.csv to produce source_trace_benchmark.csv.")

st.subheader("Hybrid vs uniform-hashing reference curve")
hybrid = filtered[filtered["CollisionMethod"] == "Adaptive Quadratic-to-Robin-Hood"].copy()
if not hybrid.empty:
    hybrid = hybrid.groupby("LoadFactor", as_index=False).agg(
        ObservedMissMean=("UnsuccessfulMeanProbes", "mean"),
        ObservedMissP95=("UnsuccessfulP95Probes", "mean"))
    hybrid["Uniform-hashing expected miss probes"] = 1 / (1 - hybrid["LoadFactor"])
    fig = go.Figure()
    fig.add_trace(go.Scatter(x=hybrid["LoadFactor"], y=hybrid["ObservedMissMean"], mode="lines+markers", name="Observed hybrid miss mean"))
    fig.add_trace(go.Scatter(x=hybrid["LoadFactor"], y=hybrid["ObservedMissP95"], mode="lines+markers", name="Observed hybrid miss P95"))
    fig.add_trace(go.Scatter(x=hybrid["LoadFactor"], y=hybrid["Uniform-hashing expected miss probes"], mode="lines+markers", name="Uniform-hashing reference"))
    fig.update_layout(title="Hybrid unsuccessful-search probes vs uniform-hashing reference",
                      xaxis_title="Load factor α", yaxis_title="Probe count")
    st.plotly_chart(fig, use_container_width=True)
    st.caption("The uniform-hashing curve is a theoretical reference under its own assumptions, not an exact analytical model for the two-phase hybrid. The manuscript must state assumptions and discuss deviations.")
else:
    st.info("Select the adaptive hybrid in the filter above to view this comparison.")

st.header("12. Experimental limits and interpretation")
st.warning(
    "This dashboard reports measurements from the supplied workloads and current implementation. "
    "The project-source token trace is not a production compiler trace. The local standard-library "
    "baseline is std::unordered_map; it is not a substitute for a separately integrated Abseil SwissTable/F14 "
    "or an instrumented Clang/GCC symbol table. Peak process memory is not measured by TableArrayBytes. "
    "Do not claim these missing comparisons as completed."
)


st.header("13. Source-derived scoped trace replay")
trace_bench = read_csv("source_trace_benchmark.csv")
if not trace_bench.empty and {"CollisionMethod", "MedianTraceMs", "CI95Ms", "FailedOperations", "DynamicRehashes"}.issubset(trace_bench.columns):
    st.caption("The trace is extracted from the C/C++ source files in this repository and replayed with brace-depth-approximated scope frames. It is not a production compiler trace or parser-grade scope analysis.")
    fig = px.bar(trace_bench, x="CollisionMethod", y="MedianTraceMs", error_y="CI95Ms", title="Scoped source-trace replay time (median and bootstrap interval)")
    fig.update_layout(xaxis_tickangle=-25, yaxis_title="Milliseconds per replay", xaxis_title="Collision strategy")
    st.plotly_chart(fig, use_container_width=True)
    st.dataframe(trace_bench, use_container_width=True, hide_index=True)
    st.download_button("Download scoped trace benchmark", trace_bench.to_csv(index=False).encode("utf-8"), file_name="source_trace_benchmark.csv", mime="text/csv")
else:
    st.info("Run the C++ backend to generate source_trace_benchmark.csv.")
