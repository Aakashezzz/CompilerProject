# Final status audit — do not interpret visualizations as proof of all 15 comments

## Verified in this archive

- Backend builds with C++17 and the regression suite passes.
- The benchmark was rerun with ten repetitions for 108 hash/strategy/load configurations, with shuffled configuration order.
- The generated `results.csv` has zero failed insertions and zero failed successful lookups across those configurations.
- Ablation data includes the full hybrid, keyword filter disabled, duplicate checks disabled for a unique-key-only benchmark, adaptive switch disabled, Robin Hood with per-key double hashing from the beginning, and ordinary double hashing.
- Unsuccessful-search mean, P50, P95 and maximum probe counts are recorded per configuration.
- The timing interval is a deterministic percentile-bootstrap interval for the median, stored as a conservative symmetric half-width for charting.
- Hash timing is separate from table-operation timing; hash quality includes occupancy, collision pairs, chi-square statistic and an approximate p-value.
- `std::unordered_map` and (in this environment) Boost.UnorderedFlatMap results are generated.
- Erase/rebuild, dynamic growth, nested scope/shadowing tests, canonical XXH64 vectors, and randomized round trips across the six methods pass.
- Nine 300-dpi PNG figures are generated from the current CSVs under `figures/`.
- Python source syntax checks pass. The Streamlit UI could not be launched in this environment because Streamlit is not installed here; install `frontend/requirements.txt` and smoke-test the browser UI on the target machine.

## Requirement-by-requirement truth

| # | Status in code/evidence | Remaining action |
|---:|---|---|
| 1 | Implementation distinction and manuscript insert draft supplied. | Insert/cite primary sources in the actual manuscript; have the novelty statement reviewed. |
| 2 | Ablation variants and visualizations generated. | Interpret the actual data; if the adaptive design does not improve the chosen metric after rehash cost, state that honestly. |
| 3 | Keyword-filter timing and ablation generated. | Likely remove it as a claimed contribution unless a real use case demonstrates benefit; compilers generally classify keywords lexically. |
| 4 | Per-strategy unsuccessful-search distributions and bounded loops measured. | State empirical maxima separately from theoretical capacity bound in the manuscript. |
| 5 | Ten repetitions, shuffled configuration order, median and bootstrap interval generated. | Rerun on the final machine and record CPU/OS/compiler/build/background load. |
| 6 | Repository C/C++ source trace, keyword tokens, approximate scope transitions, erase/rebuild and growth are implemented/tested. | A brace-depth trace is not compiler-grade semantic analysis; ideally run on an external named codebase and document its version. |
| 7 | Default semantics match across strategies; regression tests pass. The unique-key ablation explicitly uses a benchmark-only unsafe option. | Never use the unsafe option for normal data or duplicate-containing workloads. |
| 8 | `std::unordered_map` and Boost.UnorderedFlatMap baselines are measured. | No instrumented Clang/GCC internal identifier-table benchmark is included; this needs LLVM/Clang development headers and a separately designed apples-to-apples harness. |
| 9 | FxHash mislabel corrected; XXH64 seed-0 vectors and MurmurHash3 empty input tested; standalone timing and distribution diagnostics generated. | Report the chi-square p-value as approximate/descriptive; do not make general hash-quality or security claims from one key set. |
| 10 | Correctness/termination invariants and randomized tests documented. | Match the paper's pseudocode to the implementation and get the proof reviewed. |
| 11 | Hybrid is plotted against uniform-hashing reference; classical linear-probing reference curves also shown. | The hybrid curve is not an exact derived model; include that limitation and verify any claimed 0.4% gap using the final data. |
| 12 | Slot-array bytes, estimated old+new array peak, rehash count and rehash duration recorded. | This is not process RSS and excludes strings/allocator overhead; label it accordingly. |
| 13 | Compiler-specific literature leads and a draft research-gap paragraph supplied. | Authors must integrate and verify citations in the manuscript. |
| 14 | Reference leads and manuscript editing checklist supplied. | Cannot fill real author photos/bios, faculty full name, final DOI/page metadata, or template fields without the original editable manuscript and author-supplied details. |
| 15 | No identities or contributions invented. | Authors must confirm the author list, full names, team membership and individual contribution statements. |

## Important benchmark design note

`unsafeAssumeUniqueKeysForBenchmark` exists solely for the duplicate-check ablation with unique identifiers. If duplicates can occur, enabling it can violate symbol-table semantics. It is false by default and must stay false in production/normal demonstrations.

## What can be claimed

The code and experiments are substantially expanded and tested. Do **not** claim that all 15 supervisor comments are fully closed until the manuscript edits, author confirmations, external compiler-table baseline, and any desired external source trace have been completed.
