import streamlit as st
import pandas as pd
import plotly.express as px
from utils import inject_css, check_data

st.set_page_config(page_title="Performance Analysis", page_icon="📊", layout="wide")
inject_css()
results = check_data().copy()
st.title("Performance Analysis")
st.caption("Use focused comparisons instead of plotting every configuration at once. Choose the configurations you want to compare below.")

hashes = sorted(results["HashFunction"].dropna().unique())
methods = sorted(results["CollisionMethod"].dropna().unique())
loads = sorted(results["LoadFactor"].dropna().unique())
with st.expander("Experiment filters", expanded=True):
    c1, c2, c3 = st.columns(3)
    selected_hashes = c1.multiselect("Hash functions", hashes, default=hashes[:1])
    selected_methods = c2.multiselect("Collision strategies (max 4 recommended)", methods, default=methods[:3], max_selections=6)
    selected_loads = c3.multiselect("Load factors", loads, default=loads)
filtered = results[results["HashFunction"].isin(selected_hashes) & results["CollisionMethod"].isin(selected_methods) & results["LoadFactor"].isin(selected_loads)].copy()
if filtered.empty:
    st.warning("Select at least one hash function, collision strategy, and load factor.")
    st.stop()

c1, c2, c3, c4 = st.columns(4)
c1.metric("Configurations shown", f"{len(filtered):,}")
c2.metric("Median average probes", f"{filtered['AverageProbes'].median():.3f}")
c3.metric("Median insertion time", f"{filtered['InsertionTime'].median():.2f} ms")
c4.metric("Median successful search", f"{filtered['SuccessfulSearchTime'].median():.2f} ms")
st.divider()

metric_map = {
    "Insertion time (ms)": "InsertionTime",
    "Successful-search time (ms)": "SuccessfulSearchTime",
    "Unsuccessful-search time (ms)": "UnsuccessfulSearchTime",
    "Average probes": "AverageProbes",
    "Collisions": "Collisions",
    "Maximum probe distance": "MaxProbeDistance",
}
available_labels = [label for label, col in metric_map.items() if col in filtered.columns]
metric_label = st.selectbox("Metric to plot", available_labels, index=0)
metric = metric_map[metric_label]

st.subheader("Trend across load factors")
trend = filtered.groupby(["LoadFactor", "CollisionMethod"], as_index=False)[metric].median()
fig = px.line(trend, x="LoadFactor", y=metric, color="CollisionMethod", markers=True,
              title=f"{metric_label} by load factor",
              labels={"LoadFactor":"Load factor", metric:"Median " + metric_label.lower(), "CollisionMethod":"Collision strategy"})
fig.update_xaxes(tickformat=".0%", dtick=0.1)
fig.update_layout(height=480, hovermode="x unified", legend_title_text="Collision strategy", margin=dict(l=20, r=20, t=65, b=20))
st.plotly_chart(fig, use_container_width=True)
st.caption("Each point is the median of the rows available for that load factor and strategy. Keep the number of selected strategies small for a readable comparison.")

st.divider()
st.subheader("Compare strategies at one load factor")
chosen_load = st.selectbox("Load factor for bar chart", selected_loads or loads, format_func=lambda x: f"{float(x):.0%}")
bar_data = filtered[filtered["LoadFactor"] == chosen_load].groupby("CollisionMethod", as_index=False)[metric].median().sort_values(metric)
if not bar_data.empty:
    fig2 = px.bar(bar_data, x="CollisionMethod", y=metric, color="CollisionMethod", text_auto=".3g",
                  title=f"{metric_label} at {float(chosen_load):.0%} load factor",
                  labels={metric:metric_label, "CollisionMethod":"Collision strategy"})
    fig2.update_layout(showlegend=False, height=420, xaxis_tickangle=-20, margin=dict(l=20, r=20, t=65, b=100))
    st.plotly_chart(fig2, use_container_width=True)
else:
    st.info("No data exists for this load factor under the current filters.")

st.divider()
st.subheader("Performance heatmap")
heatmap_data = filtered.groupby(["HashFunction", "CollisionMethod"], as_index=False)[metric].median()
heatmap = heatmap_data.pivot(index="HashFunction", columns="CollisionMethod", values=metric)
fig3 = px.imshow(heatmap, text_auto=".3g", aspect="auto", title=f"{metric_label} by hash function and strategy",
                 labels={"x":"Collision strategy", "y":"Hash function", "color":metric_label})
fig3.update_layout(height=max(300, 100 + 70 * len(heatmap.index)), margin=dict(l=20, r=20, t=65, b=20))
st.plotly_chart(fig3, use_container_width=True)

st.divider()
st.subheader("Automatic performance ranking")
ranking = filtered.groupby(["HashFunction", "CollisionMethod"], as_index=False).agg(
    InsertionTime=("InsertionTime", "median"), SuccessfulSearchTime=("SuccessfulSearchTime", "median"),
    UnsuccessfulSearchTime=("UnsuccessfulSearchTime", "median"), Collisions=("Collisions", "median"),
    AverageProbes=("AverageProbes", "median"))
score_cols = ["InsertionTime", "SuccessfulSearchTime", "UnsuccessfulSearchTime", "Collisions", "AverageProbes"]
ranking["Score"] = sum(ranking[col].rank(pct=True) for col in score_cols)
ranking = ranking.sort_values("Score").reset_index(drop=True)
best = ranking.iloc[0]
st.success(f"Best ranked in the current selection: {best['HashFunction']} + {best['CollisionMethod']} — median insertion {best['InsertionTime']:.2f} ms, median probes {best['AverageProbes']:.3f}.")
st.dataframe(ranking, use_container_width=True, hide_index=True)
st.download_button("Download filtered benchmark data", filtered.to_csv(index=False).encode("utf-8"), file_name="filtered_performance_results.csv", mime="text/csv")
