import os
import pandas as pd
import streamlit as st

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
DATA_DIR = os.path.join(BASE_DIR, "..", "data")

RESULTS_PATH = os.path.join(DATA_DIR, "results.csv")
TABLE_PATH = os.path.join(DATA_DIR, "table.csv")


def load_results():
    if not os.path.exists(RESULTS_PATH):
        return pd.DataFrame()

    return pd.read_csv(RESULTS_PATH)


def load_table():
    if not os.path.exists(TABLE_PATH):
        return pd.DataFrame()

    return pd.read_csv(TABLE_PATH)


def inject_css():
    st.markdown("""
    <style>
    .main {
        padding-top: 1rem;
    }

    .block-container {
        max-width: 1400px;
        padding-top: 2rem;
        padding-bottom: 3rem;
    }

    .hero {
        padding: 35px;
        border-radius: 20px;
        background: linear-gradient(135deg, #111827, #1e293b);
        border: 1px solid #334155;
        margin-bottom: 25px;
    }

    .hero-title {
        font-size: 42px;
        font-weight: 800;
        margin-bottom: 10px;
    }

    .hero-subtitle {
        font-size: 18px;
        color: #cbd5e1;
        line-height: 1.6;
    }

    .section-title {
        font-size: 28px;
        font-weight: 750;
        margin-top: 30px;
        margin-bottom: 15px;
    }

    .info-card {
        padding: 22px;
        border-radius: 16px;
        border: 1px solid #334155;
        background: #111827;
        min-height: 150px;
    }

    .info-card h3 {
        margin-top: 0;
    }

    .info-card p {
        color: #cbd5e1;
        line-height: 1.6;
    }

    .flow-box {
        padding: 18px;
        border-radius: 14px;
        text-align: center;
        background: #111827;
        border: 1px solid #334155;
        font-weight: 650;
    }

    .flow-arrow {
        text-align: center;
        font-size: 28px;
        padding-top: 12px;
    }

    .metric-card {
        padding: 20px;
        border-radius: 15px;
        background: #111827;
        border: 1px solid #334155;
        text-align: center;
    }

    .metric-value {
        font-size: 30px;
        font-weight: 800;
    }

    .metric-label {
        color: #94a3b8;
        font-size: 14px;
        margin-top: 5px;
    }

    .algorithm-card {
        padding: 20px;
        border-radius: 15px;
        border: 1px solid #334155;
        background: #0f172a;
        margin-bottom: 15px;
    }

    .algorithm-card h3 {
        margin-top: 0;
    }

    .small-text {
        color: #94a3b8;
        font-size: 14px;
    }

    .winner {
        padding: 24px;
        border-radius: 18px;
        background: linear-gradient(135deg, #172554, #1e3a8a);
        border: 1px solid #3b82f6;
    }
    </style>
    """, unsafe_allow_html=True)


def check_data():
    results = load_results()

    if results.empty:
        st.error(
            "Benchmark data was not found. Run the C++ backend first."
        )
        st.stop()

    return results