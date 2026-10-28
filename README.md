# Compiler Symbol Table Analyzer

A C++17 symbol-table implementation and Streamlit dashboard for exploring hash functions, open-addressing collision strategies, and repeatable benchmark measurements.

> **Research integrity note:** the benchmark uses deterministic, compiler-like synthetic identifier workloads. These are not traces collected from a production compiler. The project reports measurements from the machine on which it is run; do not copy benchmark numbers into a paper without recording the environment and rerunning the experiment.

## What changed in this revision

- Corrected the misleading **FxHash** label. That function is FNV-1a followed by a custom avalanche finalizer, so the dashboard and output call it **FNV-1a + avalanche**.
- Kept classical Robin Hood hashing as **linear probing plus displacement-based swaps**, and added a separate **Robin Hood + double-hashing** baseline in which displaced keys continue on their own coprime steps. This variant does not use the linear-Robin-Hood early-miss rule for unsuccessful searches.
- Renamed the hybrid strategy **Adaptive Quadratic-to-Robin-Hood**. It starts with triangular-number quadratic probing and, at the 50% threshold, rehashes once into classical Robin Hood linear probing.
- Made duplicate rejection and reserved-keyword rejection consistent across all collision strategies. Reserved keywords are rejected; they are not silently reported as stored symbols.
- Added a regression test suite for insertion, successful and unsuccessful lookup, duplicates, reserved words, capacity edge cases, and hybrid rehashing.
- Added randomized configuration order, 10-run timing summaries by default, median timings, percentile-bootstrap 95% confidence intervals, unsuccessful-search probe distributions, standalone hash timings, and chi-square bucket-distribution statistics.
- Added an ablation experiment for keyword filtering, duplicate-check, adaptive switching, Robin Hood + double hashing from the first insertion, and a double-hashing baseline.
- Added optional dynamic growth, erase/rebuild semantics, and a scope-aware environment wrapper with shadowing. Regression tests cover growth, deletion and scope exit.
- Added an identifier-token extractor for C/C++ source and a scope-depth-approximated source trace replay. This is a reproducible source-derived workload, not a production compiler trace or full C++ parse.
- Added `std::unordered_map` and optional `boost::unordered_flat_map` baselines (the latter is available only if the Boost header is installed when compiling).
- Added a Research Evidence page with ablation, source-trace, hash timing/quality, baseline, probe distribution, confidence interval, memory and rehash charts.
- Kept the existing Streamlit app and CSV interfaces working; the legacy timing columns remain as aliases for the median timing columns.

## Requirements

- C++17 compiler (`g++`/MinGW; GCC 8+ recommended)
- Python 3.10+
- Python packages in `frontend/requirements.txt`

## Build and run (Windows PowerShell)

From the repository root:

```powershell
python -m pip install -r frontend/requirements.txt
python scripts/extract_source_trace.py --root . --output data/source_identifier_trace.csv
g++ backend/main.cpp backend/src/hash_functions.cpp backend/src/symbol_table.cpp -Ibackend/include -std=c++17 -O2 -o backend/compiler_project.exe
.\backend\compiler_project.exe 10
python -m streamlit run frontend/app.py
```

The benchmark accepts an optional repetition count from 1 to 30. The default is 10:

```powershell
.\backend\compiler_project.exe 10
```

On Linux/macOS, compile to `backend/compiler_project` and run `./backend/compiler_project 10` from the repository root. The program detects whether it is launched from the root or `backend/` and writes to the repository's `data/` directory.

## Research Evidence dashboard

Open **Research Evidence** in the Streamlit sidebar after running the backend. It contains separate visualizations for the metrics above and download controls for the underlying CSV data. Plotly charts expose a camera/download control where supported by the installed frontend.

**Research limitations:** the source trace is approximate and not a production compiler trace; the project does not yet include an instrumented Clang/GCC identifier table; the hybrid's reference curve is not an exact derived model; XXH64 is checked against published seed-0 vectors and MurmurHash3 empty-input behavior; and manuscript-only work (inserting the references and argument into the paper, final reference metadata, approved author biographies/photos, faculty member’s full name, and confirmed individual author contributions) requires author-supplied details and final manuscript editing. See `docs/REVIEW_RESPONSE_MATRIX.md` for the per-comment status.

## Test the C++ implementation

```bash
g++ backend/tests/symbol_table_tests.cpp backend/src/hash_functions.cpp backend/src/symbol_table.cpp -Ibackend/include -std=c++17 -O2 -o backend/symbol_table_tests
./backend/symbol_table_tests
```

Expected output: `All symbol table tests passed.`

## Output files

| File | Contents |
|---|---|
| `data/results.csv` | Hash-function/collision-method combinations; median insertion/search timings, approximate 95% CIs, successful/unsuccessful lookup probe metrics, load factors, and correctness counters. |
| `data/baseline_results.csv` | Repeat-run timing comparison against C++ `std::unordered_map`. Its bucket implementation and allocator are standard-library/compiler dependent. |
| `data/hash_quality.csv` | Occupied buckets, collision pairs, maximum occupancy, and chi-square bucket-distribution statistic. |
| `data/hash_timing.csv` | Standalone hash computation time and confidence interval. |
| `data/ablation_results.csv` | Isolated keyword-filter, unique-key duplicate-check-off, adaptive-switch and double-hashing baseline measurements. |
| `data/modern_baseline_results.csv` | Optional `boost::unordered_flat_map` comparison; marks unavailable if Boost is absent. |
| `data/source_identifier_trace.csv` | Token-by-token source identifier/keyword trace with approximate brace depth. |
| `data/source_trace_benchmark.csv` | Repeated scope-aware replay measurements from the extracted trace. |
| `data/table.csv` | Small example symbol table used by the dashboard. |

`InsertionTime`, `SuccessfulSearchTime`, and `UnsuccessfulSearchTime` are retained as compatibility aliases for the corresponding median timing columns. Times are milliseconds for a batch of operations, not per-operation latency.

## Benchmark design and interpretation

- Table capacity: 8,192 slots (a power of two, required for full-cycle triangular-number probing in this implementation).
- Target load factors: 25%, 40%, 50%, 60%, 70%, and 80%.
- Default repetitions: 10; the reported confidence interval is a deterministic percentile-bootstrap 95% interval for the median, represented as a conservative symmetric half-width for chart error bars.
- Identifier names, types, scopes, and source-line metadata vary. Insertion order is shuffled with deterministic seeds for repeatability.
- Unsuccessful lookup metrics include mean, median (P50), P95, and maximum probes. The probe distribution is measured per missing-key lookup.
- The `std::unordered_map` baseline reports wall-clock times and bucket load factor; its probe counts are not comparable to the custom open-addressed tables.
- `Collisions`, `TotalProbes`, `AverageProbes`, `MaxProbeDistance`, and `RehashCount` describe insertion-side counters from the final repetition for that configuration; they are not averaged over repetitions. Timing fields are aggregated over all repetitions. `TableArrayBytes` estimates only the contiguous `TableEntry` vector, excluding dynamic string storage, allocator metadata, and process overhead.
- The standalone `SymbolTable` supports erase by rebuilding the table and optional dynamic growth. `ScopedSymbolEnvironment` models lexical scope stacks and shadowing using one table per scope. The source trace's scope reconstruction is brace-depth approximated and must not be presented as a full C++ parser or production compiler trace.
- `PeakTableArrayBytes` estimates simultaneous old/new slot arrays; it excludes dynamic string storage, allocator metadata, and total process RSS. `RehashDurationMs` is measured separately but is collected on the final repetition for each configuration.
- Hash implementations are in-repository implementations, not linked canonical reference libraries. In particular, `FNV-1a + avalanche` is not FxHash. XXH64 is checked against published seed-0 vectors; MurmurHash3 x64/128 is configured with seed 0 and checked on empty input. FNV-1a + avalanche is a custom function.

## Algorithms

- **Linear probing:** checks consecutive slots.
- **Quadratic probing:** uses triangular-number offsets `i(i+1)/2`. For the benchmark's power-of-two capacity this covers the full table.
- **Double hashing:** uses a key-dependent step size in `[1, capacity-1]`.
- **Robin Hood hashing:** uses linear probing and swaps entries when the incoming entry has travelled farther than the resident entry. Lookup uses the Robin Hood early-termination rule.
- **Robin Hood + double hashing:** a separate experimental baseline; displaced keys continue along their own coprime double-hash step. Successful search follows the key's sequence; unsuccessful search must continue until an empty slot or capacity bound.
- **Adaptive Quadratic-to-Robin-Hood:** uses triangular-number quadratic probing below the 50% threshold, then rehashes existing entries once into classical Robin Hood linear probing.

## Project structure

```text
CompilerProject-main/
├── backend/
│   ├── include/                 # Public C++ interfaces
│   ├── src/                     # Hash functions and symbol table algorithms
│   ├── tests/                   # Regression tests
│   └── main.cpp                 # Repeatable benchmark runner
├── data/                        # Generated CSV output
├── docs/                        # Implementation notes and review mapping
└── frontend/                    # Streamlit visualization dashboard
```

## Troubleshooting

- **`g++` is not recognized:** install MinGW-w64/GCC and add its `bin` directory to `PATH`.
- **`streamlit` is not recognized:** use `python -m streamlit run frontend/app.py`.
- **No benchmark data:** run the C++ backend from the repository root before opening the dashboard.
- **CSV shows stale values:** rerun the backend; generated CSV files are overwritten.
