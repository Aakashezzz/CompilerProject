import streamlit as st
import plotly.graph_objects as go
import hashlib
from utils import inject_css

st.set_page_config(
    page_title="Collision Simulator",
    page_icon="💥",
    layout="wide"
)

inject_css()

st.title(" Collision Resolution Simulator")

st.markdown("""
This simulator demonstrates how open-addressing techniques find
alternative positions when the initial hash-table position is occupied.
""")

col1, col2 = st.columns(2)

with col1:
    key = st.text_input(
        "Identifier",
        value="studentCount"
    )

with col2:
    table_size = st.slider(
        "Table Size",
        min_value=7,
        max_value=31,
        value=13,
        step=2
    )

method = st.selectbox(
    "Collision Resolution Method",
    [
        "Linear Probing",
        "Quadratic Probing",
        "Double Hashing",
        "Robin Hood Hashing"
    ]
)

def hash_value(text):
    digest = hashlib.sha256(
        text.encode()
    ).hexdigest()

    return int(digest, 16)


def second_hash(text, size):
    value = hash_value(text)
    return 1 + (value % (size - 1))


def get_probe_sequence(text, size, method):
    primary = hash_value(text) % size

    if method == "Linear Probing":
        return [
            (primary + i) % size
            for i in range(size)
        ]

    if method == "Quadratic Probing":
        return [
            (primary + i * i) % size
            for i in range(size)
        ]

    if method == "Double Hashing":
        step = second_hash(text, size)

        return [
            (primary + i * step) % size
            for i in range(size)
        ]

    return [
        (primary + i) % size
        for i in range(size)
    ]


sequence = get_probe_sequence(
    key,
    table_size,
    method
)

primary = hash_value(key) % table_size

st.markdown("---")

col1, col2, col3 = st.columns(3)

with col1:
    st.metric(
        "Hash Value",
        f"{hash_value(key):,}"
    )

with col2:
    st.metric(
        "Initial Index",
        primary
    )

with col3:
    st.metric(
        "Probe Positions",
        len(sequence)
    )

st.subheader("Probe Sequence")

sequence_text = " → ".join(
    str(value)
    for value in sequence[:10]
)

if len(sequence) > 10:
    sequence_text += " → ..."

st.code(sequence_text)

st.markdown("---")

st.subheader("Hash Table Visualization")

fig = go.Figure()

for index in range(table_size):
    occupied = index == primary

    fig.add_trace(
        go.Scatter(
            x=[index],
            y=[0],
            mode="markers+text",
            marker=dict(
                size=45
            ),
            text=[
                f"{index}<br>"
                + ("INITIAL" if occupied else "EMPTY")
            ],
            textposition="middle center",
            showlegend=False,
            hovertemplate=f"Index: {index}<extra></extra>"
        )
    )

for i in range(min(len(sequence), 8) - 1):
    fig.add_annotation(
        x=sequence[i + 1],
        y=0,
        ax=sequence[i],
        ay=0,
        xref="x",
        yref="y",
        axref="x",
        ayref="y",
        showarrow=True,
        arrowhead=2
    )

fig.update_layout(
    height=300,
    xaxis=dict(
        title="Hash Table Index",
        tickmode="linear",
        dtick=1
    ),
    yaxis=dict(
        visible=False,
        range=[-1, 1]
    ),
    margin=dict(
        l=20,
        r=20,
        t=30,
        b=40
    )
)

st.plotly_chart(
    fig,
    use_container_width=True
)

st.markdown("---")

st.subheader("How the Method Works")

explanations = {
    "Linear Probing":
        "Checks the next consecutive table position until an available position is found.",

    "Quadratic Probing":
        "Uses increasing quadratic offsets to search alternative positions.",

    "Double Hashing":
        "Uses a second hash value to determine the distance between successive probes.",

    "Robin Hood Hashing":
        "Attempts to reduce probe-length inequality by allowing entries with shorter probe distances to be displaced."
}

st.info(explanations[method])

st.caption(
    "The simulator is an educational visualization of probing behavior. "
    "The benchmark results shown in Performance Analysis come from the C++ implementation."
)