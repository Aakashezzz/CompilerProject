````markdown
# Compiler Symbol Table Analyzer

## How to Run the Project

This project contains a C++ backend for benchmarking hash functions and collision resolution techniques, and a Python Streamlit frontend for visualization.

---

## 1. Requirements

Make sure you have the following installed:

- Python 3
- g++ / MinGW
- pip

Check Python:

```powershell
python --version
````

Check g++:

```powershell
g++ --version
```

---

## 2. Install Python Libraries

Open PowerShell inside the project folder and run:

```powershell
cd frontend
python -m pip install -r requirements.txt
```

The required libraries are:

* Streamlit
* Pandas
* Plotly

---

## 3. Compile the C++ Backend

Go to the backend folder:

```powershell
cd ..\backend
```

Compile the project:

```powershell
g++ main.cpp src/hash_functions.cpp src/symbol_table.cpp -Iinclude -std=c++17 -O2 -o compiler_project
```

If compilation is successful, `compiler_project.exe` will be created inside the `backend` folder.

---

## 4. Run the C++ Backend

Run:

```powershell
.\compiler_project.exe
```

The backend will perform the benchmark experiments and generate the required data files.

The generated files are:

```text
data/results.csv
data/table.csv
```

These files are used by the frontend for visualization.

---

## 5. Run the Streamlit Frontend

After the C++ program finishes, go to the frontend folder:

```powershell
cd ..\frontend
```

Run:

```powershell
python -m streamlit run app.py
```

A local URL will be displayed, usually:

```text
http://localhost:8501
```

Open the URL in a browser.

---

# Complete Setup

If setting up the project for the first time, run these commands in order:

```powershell
cd Compiler_project\frontend
python -m pip install -r requirements.txt

cd ..\backend
g++ main.cpp src/hash_functions.cpp src/symbol_table.cpp -Iinclude -std=c++17 -O2 -o compiler_project
.\compiler_project.exe

cd ..\frontend
python -m streamlit run app.py
```

---

# Project Structure

```text
Compiler_project/
│
├── backend/
│   ├── include/
│   │   ├── hash_functions.h
│   │   └── symbol_table.h
│   │
│   ├── src/
│   │   ├── hash_functions.cpp
│   │   └── symbol_table.cpp
│   │
│   └── main.cpp
│
├── data/
│   ├── results.csv
│   └── table.csv
│
└── frontend/
    ├── app.py
    ├── utils.py
    ├── requirements.txt
    │
    └── pages/
        ├── 1_Overview.py
        ├── 2_Hash_Laboratory.py
        ├── 3_Collision_Simulator.py
        └── 4_Performance_Analysis.py
```

---

# Project Workflow

```text
C++ Backend
     ↓
Hash Functions
     ↓
Collision Resolution
     ↓
Benchmark Experiments
     ↓
CSV Data
     ↓
Python / Pandas
     ↓
Plotly
     ↓
Streamlit Dashboard
```

---

# Important

### If C++ code is changed

Recompile and run the backend again:

```powershell
cd backend
g++ main.cpp src/hash_functions.cpp src/symbol_table.cpp -Iinclude -std=c++17 -O2 -o compiler_project
.\compiler_project.exe
```

Then start the frontend:

```powershell
cd ..\frontend
python -m streamlit run app.py
```

### If only frontend/Python code is changed

You only need to run:

```powershell
cd frontend
python -m streamlit run app.py
```

---

# Troubleshooting

### `streamlit is not recognized`

Use:

```powershell
python -m streamlit run app.py
```

instead of:

```powershell
streamlit run app.py
```

### `g++ is not recognized`

Install MinGW/g++ and add its `bin` directory to the Windows PATH.

### `python is not recognized`

Install Python and enable **Add Python to PATH** during installation.

### `No columns to parse from file`

The CSV file is empty or has not been generated.

Run the C++ backend first:

```powershell
cd backend
.\compiler_project.exe
```

Then start Streamlit again.

---

## Quick Start

For subsequent runs, use:

```powershell
cd Compiler_project\backend
.\compiler_project.exe

cd ..\frontend
python -m streamlit run app.py
```

```
```
