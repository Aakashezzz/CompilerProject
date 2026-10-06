import streamlit as st
from utils import inject_css, check_data

st.set_page_config(
    page_title="Project Overview",
    page_icon="📘",
    layout="wide"
)

inject_css()
results = check_data()

st.title(" Project Overview")

st.markdown("""
A compiler uses a symbol table to store information about identifiers
encountered during compilation. This project investigates how different
hash functions and collision resolution techniques affect symbol-table
performance.
""")

st.markdown(
    '<div class="section-title">The Experimental Design</div>',
    unsafe_allow_html=True
)

col1, col2 = st.columns(2)

with col1:
    st.markdown("""
    <div class="algorithm-card">
        <h3>Hash Functions</h3>
        <p>MurmurHash3</p>
        <p>FxHash</p>
        <p>xxHash</p>
    </div>
    """, unsafe_allow_html=True)

with col2:
    st.markdown("""
    <div class="algorithm-card">
        <h3>Collision Resolution</h3>
        <p>Linear Probing</p>
        <p>Quadratic Probing</p>
        <p>Double Hashing</p>
        <p>Robin Hood Hashing</p>
    </div>
    """, unsafe_allow_html=True)

st.markdown(
    '<div class="section-title">Experimental Matrix</div>',
    unsafe_allow_html=True
)

matrix = (
    results
    .groupby(["HashFunction", "CollisionMethod"])
    .size()
    .reset_index(name="Experiments")
)

st.dataframe(
    matrix,
    use_container_width=True,
    hide_index=True
)

st.markdown(
    '<div class="section-title">Compiler Connection</div>',
    unsafe_allow_html=True
)

cols = st.columns(7)

flow = [
    "Program",
    "Lexical Analysis",
    "Tokens",
    "Identifiers",
    "Hash Function",
    "Symbol Table",
    "Performance"
]

for i, text in enumerate(flow):
    with cols[i]:
        st.markdown(
            f'<div class="flow-box">{text}</div>',
            unsafe_allow_html=True
        )
        if i < len(flow) - 1:
            st.markdown(
                '<div class="flow-arrow">↓</div>',
                unsafe_allow_html=True
            )

st.markdown(
    '<div class="section-title">Measured Parameters</div>',
    unsafe_allow_html=True
)

parameters = [
    "Insertion Time",
    "Successful Search Time",
    "Unsuccessful Search Time",
    "Collision Count",
    "Total Probes",
    "Average Probe Distance",
    "Maximum Probe Distance",
    "Load Factor"
]

cols = st.columns(4)

for i, parameter in enumerate(parameters):
    with cols[i % 4]:
        st.markdown(
            f"""
            <div class="algorithm-card">
                <h4>{parameter}</h4>
            </div>
            """,
            unsafe_allow_html=True
        )