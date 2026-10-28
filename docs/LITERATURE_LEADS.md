# Literature and documentation leads for manuscript revision

The entries below are starting points to verify against the exact edition/version used. Insert and format them in the actual manuscript reference list; this file does not mean the PDF manuscript has already been revised.

1. P. Celis, *Robin Hood Hashing*, Ph.D. thesis, University of Waterloo, 1986. This is a primary source for Robin Hood hashing and its probe-sequence variants; verify the thesis metadata and the exact double-hashing formulation cited.
2. O. Amble and D. E. Knuth, “Ordered Hash Tables,” *The Computer Journal*, vol. 17, no. 2, pp. 135–142, 1974. DOI: https://doi.org/10.1093/comjnl/17.2.135.
3. R. P. Brent, “Reducing the Retrieval Time of Scatter Storage Techniques,” *Communications of the ACM*, vol. 16, no. 2, pp. 105–109, 1973. DOI: https://doi.org/10.1145/361952.361995.
4. R. Pagh and F. F. Rodler, “Cuckoo Hashing,” *Journal of Algorithms*, vol. 51, no. 2, pp. 122–144, 2004. DOI: https://doi.org/10.1016/j.jalgor.2003.12.002.
5. D. E. Knuth, *The Art of Computer Programming, Volume 3: Sorting and Searching*, 2nd ed., Addison-Wesley, 1998. Consult the exact section/pages for the probing analysis quoted.
6. GNU gperf documentation, “Perfect Hash Function Generator.” Explain the distinction between lexer-level keyword recognition and mutable symbol-table storage. Record the documentation URL and access date in the manuscript.
7. LLVM/Clang source/API documentation for `clang::IdentifierTable`, `IdentifierInfo`, and `llvm::StringMap`. Pin a release tag or commit and distinguish API/source inspection from an instrumented performance benchmark.
8. Boost.Unordered documentation for `boost::unordered_flat_map`, which is used as the optional modern flat-hash-table baseline when the required header is available.
9. XXH64 reference implementation/test vectors (xxHash project) and MurmurHash3 x64/128 reference implementation/test vectors. State the seed and which output word is used. The project compares XXH64 seed 0, MurmurHash3 x64/128 seed 0 (low 64 bits), and a custom FNV-1a + avalanche function; the latter is not FxHash.

## Research gap to articulate

A compiler-specific gap should concern the interaction among identifier-table semantics (duplicate detection and shadowing), lexer-level keyword filtering, changing scopes/lifetimes, deletion and table growth, and the cost of a rehashing adaptive strategy. Avoid claiming a new Robin Hood-with-double-hashing algorithm: distinguish the established literature from the present hybrid, which is triangular probing followed by a rehash into classical linear-probing Robin Hood hashing. A repository-derived brace-depth trace is not equivalent to a production compiler's semantic identifier trace.
