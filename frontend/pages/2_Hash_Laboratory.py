import streamlit as st
import pandas as pd
import plotly.express as px
from utils import inject_css, check_data, load_table

st.set_page_config(
    page_title="Hash Laboratory",
    page_icon="🔬",
    layout="wide"
)

inject_css()

results = check_data()
table = load_table()

st.title(" Hash Function Laboratory")

st.markdown(
    "Explore how the three selected hash functions behave across the benchmark dataset."
)

hash_functions = results["HashFunction"].unique()

selected_hash = st.selectbox(
    "Select Hash Function",
    hash_functions
)

hash_data = results[
    results["HashFunction"] == selected_hash
]

col1, col2, col3 = st.columns(3)

with col1:
    st.metric(
        "Average Collisions",
        f"{hash_data['Collisions'].mean():,.0f}"
    )

with col2:
    st.metric(
        "Average Probes",
        f"{hash_data['AverageProbes'].mean():.3f}"
    )

with col3:
    st.metric(
        "Average Insertion Time",
        f"{hash_data['InsertionTime'].mean():.2f} ms"
    )

st.markdown("---")

st.subheader("Performance Across Load Factors")

metric = st.selectbox(
    "Metric",
    [
        "Collisions",
        "AverageProbes",
        "InsertionTime",
        "SuccessfulSearchTime",
        "UnsuccessfulSearchTime"
    ]
)

fig = px.line(
    hash_data,
    x="LoadFactor",
    y=metric,
    color="CollisionMethod",
    markers=True,
    title=f"{selected_hash} — {metric}"
)

fig.update_xaxes(
    tickformat=".0%"
)

st.plotly_chart(
    fig,
    use_container_width=True
)

st.markdown("---")

st.subheader("Hash Function Comparison")

comparison = (
    results
    .groupby("HashFunction")
    .agg({
        "Collisions": "mean",
        "AverageProbes": "mean",
        "InsertionTime": "mean",
        "SuccessfulSearchTime": "mean"
    })
    .reset_index()
)

fig2 = px.bar(
    comparison,
    x="HashFunction",
    y="AverageProbes",
    title="Average Probe Count by Hash Function",
    text_auto=".2f"
)

st.plotly_chart(
    fig2,
    use_container_width=True
)

st.markdown("---")

st.subheader("Current Symbol Table")

if not table.empty:
    occupied = table[
        table["Identifier"].notna()
        & (table["Identifier"] != "")
    ].copy()

    if not occupied.empty:
        st.dataframe(
            occupied,
            use_container_width=True,
            hide_index=True
        )