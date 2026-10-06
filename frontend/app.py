import streamlit as st
from utils import inject_css, load_results

st.set_page_config(
    page_title="Compiler Symbol Table Analyzer",
    page_icon="⚙️",
    layout="wide",
    initial_sidebar_state="expanded"
)

inject_css()

results = load_results()

st.markdown("""
<div class="hero">
    <div class="hero-title">Compiler Symbol Table Analyzer</div>
    <div class="hero-subtitle">
        Performance Evaluation of Hash Functions and Collision Resolution Techniques
    </div>
</div>
""", unsafe_allow_html=True)

st.markdown(
    '<div class="section-title">Compiler Symbol Table Pipeline</div>',
    unsafe_allow_html=True
)

cols = st.columns(9)

flow = [
    ("Source Code", 0),
    ("→", 1),
    ("Lexical Analysis", 2),
    ("→", 3),
    ("Identifier", 4),
    ("→", 5),
    ("Hash Function", 6),
    ("→", 7),
    ("Symbol Table", 8)
]

for text, index in flow:
    with cols[index]:
        if text == "→":
            st.markdown(
                '<div class="flow-arrow">→</div>',
                unsafe_allow_html=True
            )
        else:
            st.markdown(
                f'<div class="flow-box">{text}</div>',
                unsafe_allow_html=True
            )

st.markdown(
    '<div class="section-title">Project Overview</div>',
    unsafe_allow_html=True
)

col1, col2, col3 = st.columns(3)

with col1:
    st.markdown("""
    <div class="info-card">
        <h3>Hash Functions</h3>
        <p>
        The project evaluates MurmurHash3, FxHash and xxHash
        for distributing compiler identifiers across a hash table.
        </p>
    </div>
    """, unsafe_allow_html=True)

with col2:
    st.markdown("""
    <div class="info-card">
        <h3>Collision Resolution</h3>
        <p>
        Four open-addressing techniques are compared:
        Linear Probing, Quadratic Probing, Double Hashing
        and Robin Hood Hashing.
        </p>
    </div>
    """, unsafe_allow_html=True)

with col3:
    st.markdown("""
    <div class="info-card">
        <h3>Performance Evaluation</h3>
        <p>
        Insertion time, search time, collisions,
        probe counts and probe distance are measured
        across different load factors.
        </p>
    </div>
    """, unsafe_allow_html=True)

if not results.empty:
    st.markdown(
        '<div class="section-title">Experiment Summary</div>',
        unsafe_allow_html=True
    )

    col1, col2, col3, col4 = st.columns(4)

    with col1:
        st.metric(
            "Hash Functions",
            results["HashFunction"].nunique()
        )

    with col2:
        st.metric(
            "Collision Methods",
            results["CollisionMethod"].nunique()
        )

    with col3:
        st.metric(
            "Configurations",
            results[
                ["HashFunction", "CollisionMethod"]
            ].drop_duplicates().shape[0]
        )

    with col4:
        st.metric(
            "Load Factors",
            results["LoadFactor"].nunique()
        )

st.markdown(
    '<div class="section-title">How to Use the Analyzer</div>',
    unsafe_allow_html=True
)

steps = [
    ("01", "Overview", "Understand the compiler-symbol-table connection."),
    ("02", "Hash Laboratory", "Explore the three hash functions."),
    ("03", "Collision Simulator", "Understand how collisions are resolved."),
    ("04", "Performance Analysis", "Compare all experimental configurations.")
]

for number, title, description in steps:
    st.markdown(
        f"""
        <div class="algorithm-card">
            <h3>{number} &nbsp; {title}</h3>
            <div class="small-text">{description}</div>
        </div>
        """,
        unsafe_allow_html=True
    )

st.markdown("---")

st.markdown(
    """
    <div style="text-align:center; color:#64748b;">
        Compiler Design Project · Symbol Table Performance Evaluation
    </div>
    """,
    unsafe_allow_html=True
)