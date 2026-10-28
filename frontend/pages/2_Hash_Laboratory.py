import streamlit as st
import pandas as pd
import plotly.express as px
from utils import inject_css, check_data, load_table

st.set_page_config(page_title="Hash Laboratory", page_icon="🔬", layout="wide")
inject_css()
results = check_data()
table = load_table()

st.title("Hash Function Laboratory")
st.caption("Explore one hash function at a time. Limit the number of collision strategies shown to keep comparisons readable.")
hash_functions = sorted(results["HashFunction"].dropna().unique())
selected_hash = st.selectbox("Select hash function", hash_functions)
hash_data = results[results["HashFunction"] == selected_hash].copy()
methods = sorted(hash_data["CollisionMethod"].dropna().unique())
selected_methods = st.multiselect("Collision strategies to compare (choose up to 3)", methods, default=methods[:2], max_selections=3)
if not selected_methods:
    st.info("Choose at least one collision strategy.")
    st.stop()
hash_data = hash_data[hash_data["CollisionMethod"].isin(selected_methods)]

c1, c2, c3 = st.columns(3)
c1.metric("Average collisions", f"{hash_data['Collisions'].mean():,.0f}")
c2.metric("Average probes", f"{hash_data['AverageProbes'].mean():.3f}")
c3.metric("Average insertion time", f"{hash_data['InsertionTime'].mean():.2f} ms")
st.divider()
st.subheader("Performance across load factors")
metric_options = [
    ("Collisions", "Collisions"),
    ("AverageProbes", "Average probes"),
    ("InsertionTime", "Insertion time (ms)"),
    ("SuccessfulSearchTime", "Successful-search time (ms)"),
    ("UnsuccessfulSearchTime", "Unsuccessful-search time (ms)"),
]
metric_col = st.selectbox("Metric", [x[0] for x in metric_options], format_func=lambda x: dict(metric_options)[x])
plot_data = hash_data.groupby(["LoadFactor", "CollisionMethod"], as_index=False)[metric_col].median()
fig = px.line(plot_data, x="LoadFactor", y=metric_col, color="CollisionMethod", markers=True,
              title=f"{selected_hash}: {dict(metric_options)[metric_col]}",
              labels={"LoadFactor":"Load factor", metric_col:dict(metric_options)[metric_col], "CollisionMethod":"Collision strategy"})
fig.update_xaxes(tickformat=".0%", dtick=0.1)
fig.update_layout(height=470, legend_title_text="Collision strategy", margin=dict(l=20, r=20, t=70, b=20), hovermode="x unified")
st.plotly_chart(fig, use_container_width=True)
st.caption("Lines show median values at each load factor. Select at most three strategies to avoid an overcrowded plot.")

st.divider()
st.subheader("Hash function comparison")
comparison = results.groupby("HashFunction", as_index=False).agg(
    AverageProbes=("AverageProbes", "median"),
    InsertionTime=("InsertionTime", "median"),
    Collisions=("Collisions", "median"))
comparison_metric = st.selectbox("Comparison metric", ["AverageProbes", "InsertionTime", "Collisions"],
                                format_func=lambda x: {"AverageProbes":"Median average probes", "InsertionTime":"Median insertion time (ms)", "Collisions":"Median collisions"}[x])
fig2 = px.bar(comparison.sort_values(comparison_metric), x="HashFunction", y=comparison_metric, text_auto=".2f",
              title=f"Hash functions compared by {comparison_metric}", labels={comparison_metric:comparison_metric, "HashFunction":"Hash function"})
fig2.update_layout(height=380, margin=dict(l=20, r=20, t=60, b=20))
st.plotly_chart(fig2, use_container_width=True)

st.divider()
st.subheader("Current symbol table")
if not table.empty:
    occupied = table[table["Identifier"].notna() & (table["Identifier"] != "")].copy()
    if not occupied.empty:
        st.dataframe(occupied, use_container_width=True, hide_index=True)
    else:
        st.info("The symbol-table CSV has no occupied entries.")
else:
    st.info("No symbol-table CSV is available yet.")
