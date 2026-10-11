import streamlit as st
import pandas as pd
import plotly.express as px
from utils import inject_css, check_data

st.set_page_config(
    page_title="Performance Analysis",
    page_icon="📊",
    layout="wide"
)

inject_css()

results = check_data()

st.title(" Performance Analysis")

st.markdown(
    "Compare the measured behavior of all hash-function and collision-resolution combinations."
)

st.sidebar.header("Experiment Filters")

selected_hashes = st.sidebar.multiselect(
    "Hash Functions",
    results["HashFunction"].unique(),
    default=list(results["HashFunction"].unique())
)

selected_methods = st.sidebar.multiselect(
    "Collision Methods",
    results["CollisionMethod"].unique(),
    default=list(results["CollisionMethod"].unique())
)

filtered = results[
    results["HashFunction"].isin(selected_hashes)
    &
    results["CollisionMethod"].isin(selected_methods)
]

if filtered.empty:
    st.warning("Select at least one hash function and collision method.")
    st.stop()

st.markdown("---")

col1, col2, col3, col4 = st.columns(4)

with col1:
    st.metric(
        "Lowest Collisions",
        f"{filtered['Collisions'].min():,}"
    )

with col2:
    st.metric(
        "Lowest Avg Probes",
        f"{filtered['AverageProbes'].min():.3f}"
    )

with col3:
    st.metric(
        "Fastest Insertion",
        f"{filtered['InsertionTime'].min():.2f} ms"
    )

with col4:
    st.metric(
        "Fastest Successful Search",
        f"{filtered['SuccessfulSearchTime'].min():.2f} ms"
    )

st.markdown("---")

st.subheader("Insertion Performance")

fig = px.bar(
    filtered,
    x="HashFunction",
    y="InsertionTime",
    color="CollisionMethod",
    barmode="group",
    facet_col="LoadFactor",
    title="Insertion Time Across Load Factors"
)

fig.update_layout(
    height=650
)

st.plotly_chart(
    fig,
    use_container_width=True
)

st.markdown("---")

st.subheader("Collision Behavior")

collision_data = (
    filtered
    .groupby(
        ["HashFunction", "CollisionMethod"]
    )["Collisions"]
    .mean()
    .reset_index()
)

fig2 = px.bar(
    collision_data,
    x="HashFunction",
    y="Collisions",
    color="CollisionMethod",
    barmode="group",
    title="Average Collision Count"
)

st.plotly_chart(
    fig2,
    use_container_width=True
)

st.markdown("---")

st.subheader("Probe Distance")

probe_data = (
    filtered
    .groupby(
        ["HashFunction", "CollisionMethod"]
    )["AverageProbes"]
    .mean()
    .reset_index()
)

fig3 = px.bar(
    probe_data,
    x="HashFunction",
    y="AverageProbes",
    color="CollisionMethod",
    barmode="group",
    title="Average Probe Count"
)

st.plotly_chart(
    fig3,
    use_container_width=True
)
st.markdown("---")

st.subheader("Worst-Case Probe Length")

max_probe_data = (
    filtered
    .groupby(["LoadFactor", "CollisionMethod"])["MaxProbeDistance"]
    .mean()
    .reset_index()
)

fig_max = px.line(
    max_probe_data,
    x="LoadFactor",
    y="MaxProbeDistance",
    color="CollisionMethod",
    markers=True,
    log_y=True,
    title="Maximum Probe Distance vs Load Factor"
)

st.plotly_chart(
    fig_max,
    use_container_width=True
)

st.markdown("---")

st.subheader("Performance Heatmap")

heatmap_data = (
    filtered
    .groupby(
        ["HashFunction", "CollisionMethod"]
    )["AverageProbes"]
    .mean()
    .reset_index()
)

heatmap = heatmap_data.pivot(
    index="HashFunction",
    columns="CollisionMethod",
    values="AverageProbes"
)

fig4 = px.imshow(
    heatmap,
    text_auto=".2f",
    aspect="auto",
    title="Average Probe Count Heatmap"
)

st.plotly_chart(
    fig4,
    use_container_width=True
)

st.markdown("---")

st.subheader("Automatic Performance Ranking")

ranking = (
    filtered
    .groupby(
        ["HashFunction", "CollisionMethod"]
    )
    .agg({
        "InsertionTime": "mean",
        "SuccessfulSearchTime": "mean",
        "UnsuccessfulSearchTime": "mean",
        "Collisions": "mean",
        "AverageProbes": "mean"
    })
    .reset_index()
)

ranking["Score"] = (
    ranking["InsertionTime"].rank(pct=True)
    + ranking["SuccessfulSearchTime"].rank(pct=True)
    + ranking["UnsuccessfulSearchTime"].rank(pct=True)
    + ranking["Collisions"].rank(pct=True)
    + ranking["AverageProbes"].rank(pct=True)
)

ranking = ranking.sort_values("Score")

best = ranking.iloc[0]

st.markdown(
    f"""
    <div class="winner">
        <h2> Best Overall Configuration</h2>
        <h3>{best['HashFunction']} + {best['CollisionMethod']}</h3>
        <p>
        Average insertion time:
        <b>{best['InsertionTime']:.2f} ms</b>
        </p>
        <p>
        Average probes:
        <b>{best['AverageProbes']:.3f}</b>
        </p>
        <p>
        Average collisions:
        <b>{best['Collisions']:,.0f}</b>
        </p>
    </div>
    """,
    unsafe_allow_html=True
)

st.markdown("---")

st.subheader("Detailed Benchmark Data")

st.dataframe(
    ranking,
    use_container_width=True,
    hide_index=True
)