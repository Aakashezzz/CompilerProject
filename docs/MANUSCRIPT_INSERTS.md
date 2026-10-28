# Manuscript-ready insertions for the 15 review comments

These are draft paragraphs and reporting instructions to paste/adapt in the manuscript. They do not claim that the PDF manuscript itself has been edited. Replace bracketed machine/environment metadata with the actual final run environment.

## Related work and novelty (Comments 1 and 13)

Robin Hood hashing is an established collision-resolution family, not a new contribution of this work. Celis's thesis studies Robin Hood hashing and its probe-sequence variants; ordered hash tables and Brent's method are related classical approaches that alter placement or retrieval behavior. Modern flat tables, including SwissTable-style designs and Boost.UnorderedFlatMap, provide further practical comparison points. Accordingly, the method evaluated here should be described narrowly as an **adaptive two-phase table**: Phase 1 uses triangular-number quadratic probing; when the configured load threshold is reached, the occupied entries are rehashed into a table using classical linear-probing Robin Hood insertion. The contribution to evaluate is the policy of changing from one established strategy to another under the tested workload—not the invention of Robin Hood hashing.

For compiler contexts, keyword recognition is normally a lexical responsibility, often implemented by the lexer or generated lookup machinery such as gperf, whereas a symbol table stores identifiers with scope and binding semantics. The specific research gap should therefore be framed around the measured trade-off between adaptive rehash cost and probe behavior under compiler-like identifier traces, duplicate handling, nested scopes, deletion/rebuild, and table growth. The current trace is repository-derived and uses brace depth as an approximation; it is not a production compiler trace or a semantic parser.

## Correctness and termination (Comment 10)

Every probe loop is bounded by the table capacity. Double hashing chooses a step coprime to the capacity, so its probe sequence visits every slot before repeating. Triangular probing is restricted to power-of-two capacities, for which its offsets cover the table. Linear Robin Hood insertion continues the displaced entry along the next linear slot; lookup may use the Robin Hood early-miss condition only for this linear-probing strategy. For the per-key double-hashing Robin Hood variant, the lookup does not use that early-miss condition and stops only at an empty slot or after a full-capacity scan. Reaching the probe bound returns failure rather than looping indefinitely. Deletion rebuilds the table from surviving entries, preserving the strategy-specific lookup invariant. These statements should be checked against the final algorithm listing and tests before publication.

## Statistical method (Comment 5)

For each configuration, the benchmark uses ten repetitions by default (configurable from 1 to 30) and randomizes the order of the 108 hash/strategy/load configurations. The reported runtime point estimate is the sample median. Its interval is a percentile-bootstrap 95% confidence interval, represented as a conservative symmetric half-width for error-bar plotting. Report the exact compiler version, operating system, CPU, build flags, and background-load conditions from the final machine run. Do not compare tiny timing differences as meaningful without considering the interval and rerun variability.

## Analytical reference (Comment 11)

The dashboard plots the classical linear-probing approximations for expected successful and unsuccessful probes and the uniform-hashing reference for unsuccessful searches, alongside the hybrid's measured mean and P95 unsuccessful-search probe counts. These curves are reference models under different assumptions, not an exact derivation for the hybrid. State that limitation explicitly. Discuss the reported 0.4% difference from quadratic probing only after confirming the exact comparison, metric, load factor, and confidence interval from the regenerated results; do not treat a small point-estimate difference as meaningful without uncertainty analysis.

## Hash-function evaluation (Comment 9)

The benchmark compares MurmurHash3 x64/128 (seed 0, low 64-bit output), XXH64 (seed 0), and a custom FNV-1a plus avalanche finalizer. The latter must not be called FxHash. Hash-computation time is measured separately from table operations. Bucket distribution is summarized using occupied buckets, collision pairs, maximum occupancy, and a chi-square statistic with degrees of freedom; the displayed p-value is a Wilson–Hilferty approximation and should be treated as diagnostic rather than a definitive distribution-quality verdict. These measurements do not assess cryptographic security.

## Results and limits (Comments 2–8 and 12)

Use the generated figures and CSVs in `figures/` and `data/` to report: (i) the ablation variants; (ii) unsuccessful-search mean/P50/P95/maximum probes; (iii) medians and bootstrap intervals; (iv) repository-source trace replay; (v) `std::unordered_map` and optional Boost.UnorderedFlatMap comparisons; (vi) hash computation and bucket distribution; and (vii) slot-array memory estimates and rehash costs. The `PeakTableArrayBytes` metric estimates simultaneous old/new table slot arrays only; it is not total process RSS and excludes string heap allocations and allocator overhead. `erase()` rebuilds the table, so deletion timing should be described as rebuild-based deletion rather than an optimized tombstone/backward-shift scheme.

The benchmark does not contain an instrumented Clang/GCC internal identifier-table comparison. The source trace is extracted from this repository, not a production compiler corpus. Add a named external source corpus and an instrumented compiler-table baseline before claiming the full workload and compiler-baseline requests are satisfied.

## Human-supplied front matter (Comments 14 and 15)

The following cannot be completed responsibly without author confirmation: the four authors' full names and affiliations, each author's individual contribution, confirmation that every listed author belongs to the team, approved biographies and photographs, and the faculty member's full name. Remove template DOI/date/volume placeholders from the actual paper and verify each reference against its publisher or repository metadata. Do not invent these details.
