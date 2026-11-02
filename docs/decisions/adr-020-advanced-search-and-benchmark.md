# ADR-020 — Advanced classical search enhancements and benchmark suite

- **Status:** Accepted
- **Context / problem:** Baseline alpha-beta with staged move ordering and quiescence search is functionally complete and correct, but tree size scales exponentially with search depth. Classical search improvements (Aspiration Windows, Principal Variation Search, Late Move Reductions, Null-Move Pruning, and Killer Heuristics) are required to reduce tree explosion while preserving tactical accuracy. A repeatable benchmark suite is needed to objectively measure node efficiency, throughput (NPS), and search stability across positions.
- **Options considered:**
  1. Monolithic search rewrite bundling all heuristics unconditionally.
  2. Independent, modular search enhancements controlled via structured `SearchOptions`, with a `vanilla()` baseline preset for differential testing and isolation.
- **Decision:** Implement modular search enhancements behind explicit feature flags in `SearchOptions`:
  - **Aspiration Windows:** Narrow initial search window $[\text{score}-\Delta, \text{score}+\Delta]$ around previous iterative deepening score; widen on fail-low or fail-high, falling back to full window.
  - **Principal Variation Search (PVS):** Full window for the first move; null window $[\alpha, \alpha+1]$ for subsequent moves; full re-search on fail-high.
  - **Late Move Reductions (LMR):** Reduced depth ($d - 1 - R$) for late quiet moves ($i \ge 3$) at depth $\ge 3$; full-depth re-search on fail-high.
  - **Null-Move Pruning (NMP):** Pass turn ($R=3$) with zugzwang safeguards (minimum non-pawn pieces requirement, no king checks, no PV nodes).
  - **Killer Move Heuristic:** 2 quiet move slots per ply recorded on beta cutoff, ordered above ordinary quiet moves ($kKillerBaseScore = 5,000$).
  - **Benchmark Suite:** Canonical 8-position fixed FEN suite reporting per-position and total nodes, time, NPS, best moves, compiler, and build metadata. Executable as standalone `rooksac_bench` and via protocol-safe UCI `bench` command writing reports to diagnostic stream.
- **Consequences:**
  - Significant node count reduction in tactical and positional searches (e.g., node reduction verified in `rooksac_search_test`).
  - Clear diagnosis: passing `SearchOptions::vanilla()` completely restores unenhanced Alpha-Beta behavior for regression and differential validation.
  - Transparent benchmark harness provides reproducible NPS and node counts across compilers and platforms.
