# Implementation notes

See [`REVIEW_RESPONSE_MATRIX.md`](REVIEW_RESPONSE_MATRIX.md) for the detailed mapping to all 15 supervisor comments and explicit remaining requirements.

## Algorithms and invariants

- Linear probing, triangular-number quadratic probing, double hashing, and classical Robin Hood linear probing are distinct strategies.
- Classical Robin Hood uses linear probing and its early-termination lookup invariant.
- A separate **Robin Hood + double hashing** baseline is implemented for comparison. A displaced resident continues along its own coprime double-hash step; successful lookup follows that key's sequence, while unsuccessful lookup scans until an empty slot or the capacity bound (no linear-Robin-Hood early-miss rule).
- The hybrid is named **Adaptive Quadratic-to-Robin-Hood**: triangular probing before the threshold, then rehash into classical linear-probing Robin Hood.
- Probe loops are bounded by table capacity. Double hashing chooses a step coprime with capacity to ensure its probe sequence can cover every slot.
- `SymbolTableOptions` supports keyword rejection, duplicate-precheck ablation, adaptive switching and optional dynamic growth. The default API preserves duplicate semantics; the isolated unique-key ablation may set `unsafeAssumeUniqueKeysForBenchmark=true` to disable duplicate checks only when every inserted key is guaranteed unique. Never enable it in normal use.
- `erase()` rebuilds the table to avoid tombstone-related lookup bugs across different probing schemes. This is a simple correct reference implementation, not an optimized deletion strategy.
- `ScopedSymbolEnvironment` maintains a table per scope, searches from inner to outer scope, supports shadowing, and drops local tables on scope exit.

## Measurements and dashboard

Run the source trace extraction before the benchmark:

```bash
python scripts/extract_source_trace.py --root . --output data/source_identifier_trace.csv
```

Then compile and run the C++ backend:

```bash
g++ backend/main.cpp backend/src/hash_functions.cpp backend/src/symbol_table.cpp -Ibackend/include -std=c++17 -O2 -o backend/compiler_project
./backend/compiler_project 10
```

The backend generates the main benchmark, standard-library baseline, optional Boost flat-map baseline, ablation results, hash timing/quality results, and source-trace replay. The Research Evidence page reads those CSV files.

## Known limits

- The source trace is lexically extracted from C/C++ source files and approximates scope by brace depth; it is not a full parser and is not a production compiler trace.
- The Boost flat-map baseline is conditional on Boost headers being installed at build time. No Clang/GCC internal identifier-table implementation is instrumented in this project.
- The uniform-hashing and linear-probing curves are reference models, not a proven exact model of the hybrid. Timing intervals are percentile-bootstrap intervals for the median, represented as a symmetric half-width for plotting.
- Table-array peak bytes exclude string allocations, allocator overhead and process RSS.
- Canonical hash implementation test vectors should be checked before claiming exact compatibility with named published hash variants.
- Literature positioning, formal proof/analysis, final bibliography metadata, author biographies/photos, faculty identity and author-contribution statements must be completed in the actual manuscript with verified information.
