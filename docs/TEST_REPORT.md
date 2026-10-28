# Test report

Last validated in the preparation environment on 2026-10-28.

- C++17 backend compiled with `g++ -std=c++17 -O2`.
- Regression tests passed, including randomized round trips for all six collision methods, erase/rebuild, dynamic growth, scope shadowing, duplicate handling, keyword rejection, and XXH64 seed-0 test vectors.
- Python syntax checks passed for the Streamlit app, all pages, utilities, and source-trace/figure scripts.
- Benchmark generated 108 hash/strategy/load configurations with 10 repetitions each; run order is shuffled.
- Observed failed insertions across generated benchmark configurations: 0.
- Observed failed successful lookups across generated benchmark configurations: 0.
- Standard-library baseline: 6 load-factor rows, 10 repetitions each.
- Boost.UnorderedFlatMap baseline: 6 load-factor rows, 10 repetitions each in this environment where the header was available.
- Ablation output includes full hybrid, keyword-filter-off, unique-key duplicate-check-off, adaptive-switch-off, Robin Hood with per-key double hashing from first insertion, and double-hashing baseline.
- Source-derived trace: 5,403 lexical identifier/keyword token occurrences from six C/C++ source files in this repository; scope depth is brace-based approximation.
- Generated nine 300-dpi PNG figures from the benchmark CSVs under `figures/`.
- `XXH64("") == 0xef46db3751d8e999` and `XXH64("a") == 0xd24ec4f1a98c6e5b` passed; MurmurHash3 x64/128 low word for empty input with seed 0 is tested as zero.

## Scope and limitations

- No instrumented Clang/GCC internal identifier table was benchmarked; do not claim that comparison is complete.
- The workload is repository-source-derived, not a production compiler trace or compiler-grade parser.
- Memory values estimate slot arrays, not process RSS or string allocation overhead.
- The uniform-hashing curve is a reference, not an exact analytical derivation for the adaptive hybrid.
- Author identity, contribution, biography/photo approval, faculty name, and final manuscript references must be supplied/confirmed by the authors.
