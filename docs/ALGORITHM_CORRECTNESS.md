# Algorithm invariants, correctness, and termination

This note is intended to support the paper's formal explanation. It does not replace a proof reviewed against the final manuscript's pseudocode.

## Probe sequences and termination

- **Linear probing / classical Robin Hood:** a key with home index `h` follows `(h+i) mod m`. The loop examines at most `m` slots. The Robin Hood variant swaps the incoming entry with a resident whose displacement is smaller, then continues with the displaced resident at the next linear slot. Every operation terminates after at most `m` probes.
- **Triangular quadratic probing:** a key follows `(h+i(i+1)/2) mod m`. The implementation rounds initial capacities for `QUADRATIC` and `HYBRID` up to a power of two; triangular offsets form a permutation modulo powers of two. The loop is still explicitly bounded by `m` as a defensive termination guarantee.
- **Double hashing:** the step is selected so `gcd(step,m)=1`. Therefore the sequence `(h+i*step) mod m` visits every slot before repeating, and search/insertion stop after an empty slot or `m` probes.
- **Robin Hood with per-key double-hashing step:** when a resident is displaced, its current slot was reached on its own double-hashing sequence. Continuing from that slot by its own coprime step follows the next position on its sequence. Because different keys have different sequences, the linear-Robin-Hood early-miss rule is disabled: a miss ends only at an empty slot or after `m` probes. This implementation's empirical stress tests exercise findability after insertion and deletion/rebuild, but a manuscript claim should be restricted to the stated invariant and verified test coverage.
- **Adaptive hybrid:** before switching, entries use triangular probing. At the threshold, all occupied entries are copied and reinserted using classical linear-probing Robin Hood. The phase flag changes only as part of this rehash, and all future insert/search operations use the phase's matching sequence. A failed insertion after exhausting the bounded sequence returns `false`; it does not loop indefinitely.

## Duplicate and keyword semantics

The default API rejects C++ reserved keywords and duplicate names for every strategy. The keyword filter is an experiment, not a claim that a production compiler should move lexical keyword recognition into a symbol table. Real compiler lexers commonly classify keywords before symbol-table insertion.

The `precheckDuplicates=false` normally removes only the redundant outer pre-search while preserving insertion-path duplicate checks. For the isolated unique-key ablation only, `unsafeAssumeUniqueKeysForBenchmark=true` disables those checks; the workload contains only unique keys. This option is explicitly unsafe if duplicate keys may occur and must never be enabled for general symbol-table use.

## Deletion and growth

Deletion rebuilds the table from surviving entries rather than leaving tombstones. This is simple and preserves probe-sequence invariants for all implemented methods, at the cost of O(m+n) work per deletion. Optional growth doubles to a power-of-two capacity and reinserts all live entries. `ScopedSymbolEnvironment` models nested scope frames, shadowing, and scope exit by discarding the innermost frame.

## Empirical checks

`backend/tests/symbol_table_tests.cpp` checks known XXH64 seed-0 vectors, empty-input MurmurHash3 behavior, insertion/search, duplicate rejection, keyword rejection, deletion/rebuild, dynamic growth, scope shadowing, and randomized round trips for every strategy. The test binary should be rerun after any change to probing logic.
