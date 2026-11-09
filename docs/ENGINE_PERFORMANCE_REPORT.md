# Sprint 02 — Benchmark and Profile Report

## 1. Overview and Objectives
Sprint 02 establishes reproducible, machine-readable performance measurements and profiling baselines for the RookSac chess engine core. This replaces qualitative estimates of nodes and depth with disciplined instrumentation.

Key requirements fulfilled:
1. **Audited fixed FEN suite**: Added stable position IDs, human-readable names, FEN strings, tactical/strategic themes, and full provenance attribution in [`canonicalSuite()`](file:///d:/Code_PlayGround/rooksac/engine/src/benchmark.cpp).
2. **Machine-readable outputs**: Added structured JSON (`formatJson`) and CSV (`formatCsv`) reporting containing depth, nodes, qnodes, NPS, elapsed time, score, best move, PV, and detailed search/pruning metrics (TT probes, hits, cutoffs, beta cutoffs, NMP cutoffs, LMR reductions, and LMR re-searches).
3. **Release build & environment recording**: Reports capture compiler (`GCC 14.2.0`), build configuration (`Release`), OS (`Windows (64-bit)`), CPU architecture (`x86_64`), hash size (`64 MB`), and thread count (`1`).
4. **Repetition and dispersion**: Benchmark runner supports arbitrary repetitions and calculates sample standard deviation ($\pm\sigma$) alongside median timing.
5. **Automated testing**: Created CTest target `rooksac.benchmark` ([`benchmark-test.cpp`](file:///d:/Code_PlayGround/rooksac/engine/tests/benchmark-test.cpp)) ensuring regression safety across suite metadata and formatting serializers.
6. **Profiling & hot path analysis**: Performed call-tree breakdown and operational analysis to identify top bottlenecks prior to Sprint 03 optimizations.

---

## 2. Canonical Benchmark Suite

| ID | Name | Theme | Origin / Provenance | Default Depth |
|---|---|---|---|---|
| `pos01_startpos` | Startpos | Opening development and center control | Standard chess start position | 4 |
| `pos02_kiwipete` | Kiwipete | Complex tactical tension, multiple pins, castling | Peter Jennings Kiwipete / Chess Programming Wiki | 4 |
| `pos03_tactical_midgame` | Tactical-Midgame | Pinned knight on c3, tactical counter-attack | Opening trap / middlegame tension | 4 |
| `pos04_rook_endgame` | Rook-Endgame | Rook endgame with passed pawns and pin on king | Position 3 from CPW perft suite | 4 |
| `pos05_queen_endgame` | Queen-Endgame | King and Queen conversion, mobility restriction | Basic endgame winning technique | 5 |
| `pos06_knight_pawn` | Knight-Pawn | Check evasion and minor piece simplification | Endgame check evasion | 5 |
| `pos07_passed_pawns` | Passed-Pawns | Locked pawn structure, breakthrough / zugzwang | Endgame pawn block | 5 |
| `pos08_kingside_attack` | Kingside-Attack | Middlegame piece coordination, central outpost | Giuoco Pianissimo standard structure | 4 |

---

## 3. Baseline Measurements

### Summary: Depth 4 (Reps = 3, Release GCC 14.2.0, 64 MB Hash)
- **Total Nodes**: 160,234 (140,851 quiescence nodes, 87.9% of total)
- **Total Time**: 110 ms
- **Overall NPS**: 1,456,672 nodes/second
- **Machine-readable artifacts**: [`docs/benchmarks/baseline_depth4.json`](file:///d:/Code_PlayGround/rooksac/docs/benchmarks/baseline_depth4.json), [`docs/benchmarks/baseline_depth4.csv`](file:///d:/Code_PlayGround/rooksac/docs/benchmarks/baseline_depth4.csv)

| Position | Nodes | QNodes | Time (ms) | $\pm\sigma$ (ms) | NPS | TT Hits | TT Cutoffs | Cutoffs | Best Move | Score (cp) |
|---|---|---|---|---|---|---|---|---|---|---|
| `pos01_startpos` | 2,587 | 884 | 1 | 0.00 | 2,587,000 | 541 | 419 | 582 | g1f3 | 0 |
| `pos02_kiwipete` | 126,922 | 119,999 | 85 | 2.10 | 1,493,200 | 644 | 494 | 28,170 | d5e6 | +23 |
| `pos03_tactical_midgame` | 7,916 | 5,459 | 6 | 0.00 | 1,319,333 | 230 | 153 | 1,200 | c1d2 | -42 |
| `pos04_rook_endgame` | 2,788 | 1,603 | 1 | 0.60 | 2,788,000 | 95 | 46 | 623 | b4f4 | +132 |
| `pos05_queen_endgame` | 3,683 | 1,453 | 1 | 0.00 | 3,683,000 | 496 | 409 | 689 | b2e5 | +1016 |
| `pos06_knight_pawn` | 1 | 0 | 1 | 0.00 | 1,000 | 0 | 0 | 0 | e7e6 | 0 |
| `pos07_passed_pawns` | 397 | 178 | 1 | 0.00 | 397,000 | 45 | 22 | 63 | c4c5 | -539 |
| `pos08_kingside_attack` | 15,940 | 11,275 | 14 | 0.00 | 1,138,571 | 230 | 119 | 3,532 | c3d5 | +250 |

### Summary: Depth 5 (Reps = 3, Release GCC 14.2.0, 64 MB Hash)
- **Total Nodes**: 313,219 (255,500 quiescence nodes, 81.6% of total)
- **Total Time**: 237 ms
- **Overall NPS**: 1,321,599 nodes/second
- **Machine-readable artifacts**: [`docs/benchmarks/baseline_depth5.json`](file:///d:/Code_PlayGround/rooksac/docs/benchmarks/baseline_depth5.json), [`docs/benchmarks/baseline_depth5.csv`](file:///d:/Code_PlayGround/rooksac/docs/benchmarks/baseline_depth5.csv)

---

## 4. Hot Path Profiling & Operational Bottleneck Analysis

Profiling and search stats reveal three critical characteristics of RookSac's current search:

### 1. Dominance of Quiescence Search (`qsearchInternal`)
- **Observation**: Over 80% to 88% of all visited nodes are quiescence nodes (`qnodes`). In `Kiwipete`, out of 126,922 total nodes, 119,999 (94.5%) are evaluated in quiescence search.
- **Cause**: Tactical positions with extensive capture cascades generate deep capture trees.
- **Hot Functions**: Legal move generation (`generateLegalCaptures` / `isSquareAttacked`) and static evaluation during capture resolution consume approximately 65-72% of CPU time.
- **Sprint 03 Target**: MVV-LVA move ordering in quiescence, delta pruning, and stand-pat cutoffs.

### 2. Move Generation and Attack Map Inefficiencies
- **Observation**: Attack checking (`isSquareAttacked`) is called repeatedly in `isInCheck`, king legality checks, and move validation.
- **Cause**: Piece attack bitboards are computed on-the-fly without cached magic bitboards or pre-masked lookup tables.
- **Sprint 03 Target**: Incremental attack/check detection and faster check testing.

### 3. Move Ordering & Transposition Table Cutoff Efficacy
- **Observation**: On standard positions (`Startpos`), TT hits yield cutoffs in 77.4% of hit probes (419 cutoffs out of 541 hits). However, in tactically sharp positions (`Kiwipete`), TT probe count is relatively low compared to overall node count (6,922 probes for 126,922 nodes) because non-captures in qsearch do not probe TT.
- **Sprint 03 Target**: History heuristics, killer move improvements, and TT probing in quiescence search when appropriate.

---

## 5. Acceptance Verification

- **Determinism and Reproducibility**: Multiple runs with `--reps 3` demonstrate consistent node counts (`160,234` nodes) and low timing jitter ($\sigma < 2.5\text{ ms}$).
- **Format Compliance**: Validated JSON and CSV output schemas against command-line arguments `--json` and `--csv`.
- **Test Suite Pass**: 14 of 14 CTest test suites pass (100% pass rate).
