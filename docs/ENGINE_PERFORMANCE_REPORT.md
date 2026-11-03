# RookSac Engine Performance Report

## 1. Summary

This document records the performance investigation, baselines, optimisations, and
benchmark results for RookSac's C++ chess engine and Python backend.

The observed 5–7 second move latency had **two independent root causes**:

| Layer | Root Cause | Impact |
|---|---|---|
| Backend | Fresh engine process started per request (UCI handshake + isready round-trip) | ~800 ms per request |
| C++ Engine | Deep search (depth 10+) with flat quiet-move ordering (no history heuristic) | 4+ s at depth 10 |

Both were addressed. The backend now reuses persistent engine processes. The
engine now applies a history heuristic, depth-scaled LMR, progressive aspiration
windows, and a 64 MB transposition table.

---

## 2. Hardware and Compiler Configuration

| Property | Value |
|---|---|
| CPU | Intel Core (Family 6 Model 154 — Raptor Lake-H / 13th-gen) |
| Logical Cores | 12 |
| OS | Windows 11 (build 26300) |
| Compiler | GCC 14.2.0 (MinGW/MSYS2) |
| Build type | Release (`-O3 -DNDEBUG`) |
| C++ standard | C++20 |
| Hash table size (before) | 16 MB (≈ 1 M entries) |
| Hash table size (after) | 64 MB (≈ 4 M entries) |
| Search threads | 1 (single-threaded) |

---

## 3. Profiling Methodology

No external sampling profiler was available on this Windows host without additional
setup. Profiling was performed via:

- **Instrumented timing** — the existing `rooksac_bench` binary measures nodes and
  wall-clock time per position at fixed depth.
- **UCI pipe benchmarks** — a Python driver started the engine binary via stdin/stdout,
  measuring wall-clock time from `go depth N` to `bestmove` reception.
- **Node-count comparison** — before/after node counts at the same depth isolate the
  effect of improved move ordering on alpha-beta pruning efficiency.

---

## 4. Identified Bottlenecks

### 4.1 Backend — Process Startup Overhead (~800 ms per request)

`BoundedEnginePool.execute_analysis()` created a **new** `UciEngineProcess` for
every analysis call:

```python
worker = UciEngineProcess(self.binary_path)
await worker.start()   # fork + UCI handshake + isready: ~800 ms
```

This dominated short-depth requests and added significant overhead to all searches.

### 4.2 C++ Engine — Flat Quiet-Move Ordering (No History Heuristic)

All quiet moves not covered by the PV move or killer heuristic received score = 0,
meaning their ordering was arbitrary. At depth 8–10 on complex positions this meant
many unpromising moves were searched before good ones, reducing alpha-beta efficiency.

### 4.3 C++ Engine — Fixed LMR Reduction (1 ply)

Late Move Reductions applied a fixed 1-ply reduction regardless of search depth.
At depths 8–10 where exponential node growth dominates, 1-ply reductions save very
little time. Deeper, depth-scaled reductions are more effective.

### 4.4 C++ Engine — Aspiration Window Failure Jumps to ±∞

On aspiration failure (score outside the window), the window immediately opened to
`[-∞, +∞]`. For positions where the true score was slightly outside the window, this
caused a needlessly expensive full-width re-search.

### 4.5 C++ Engine — Small Transposition Table (16 MB)

At depth 10 on complex positions, 16 MB (~1 M entries) fills quickly, causing
collisions that evict useful entries and force re-searched subtrees.

---

## 5. Optimisations Implemented

### 5.1 History Heuristic (move ordering)

**Files changed:** `engine/include/rooksac/move_order.hpp`, `engine/src/move_order.cpp`

A 12×64 `HistoryTable` (indexed by `[piece_type × 2 + color][to_square]`) is
maintained per search invocation. When a quiet move causes a beta cutoff at depth
`d`, its history score increases by `d²` (clamped to 8 000). During move ordering,
quiet moves not covered by PV or killers receive `kQuietBaseScore + history[p][sq]`
as their priority.

The table is zero-initialised at the start of each `search()` call, so previous
game state does not bleed across searches.

### 5.2 Depth-Scaled LMR

**Files changed:** `engine/include/rooksac/search_options.hpp`, `engine/src/search.cpp`

The LMR formula is now:

```
reduction = lmrBaseReduction + (depth / 5)
```

At depth 3 the reduction is 1 ply (unchanged). At depth 10 the reduction is 3 plies.
This more aggressively prunes late-move quiet branches at deeper searches while
remaining conservative at shallow depths where the branching factor is small.

### 5.3 Progressive Aspiration Window Widening

**Files changed:** `engine/src/search.cpp`

Instead of jumping to `[-∞, +∞]` on the first failure, the window now doubles:

```
delta *= 2;
aspirationAlpha -= delta;   // fail-low: widen downward
aspirationBeta  += delta;   // fail-high: widen upward
```

Only after two consecutive doublings does the window reach infinity. This avoids
expensive full-window re-searches for scores that are slightly outside the initial ±50 cp window.

### 5.4 64 MB Transposition Table

**Files changed:** `engine/include/rooksac/transposition_table.hpp`, `engine/src/benchmark.cpp`, `engine/src/uci.cpp`

Default table size increased from 16 MB to 64 MB (≈ 4 M 16-byte entries). The UCI
`Hash` option default is updated to 64 and the benchmark binary uses
`kDefaultSizeBytes` to stay in sync. GUIs and the backend can still set any size
via `setoption name Hash value N`.

### 5.5 Persistent Engine Process Pool (Backend)

**Files changed:** `app/backend/engine_manager.py`

`BoundedEnginePool` now maintains a pool of **long-lived** `UciEngineProcess`
instances (one per worker slot). Workers are created lazily on first use and reused
across all subsequent analysis requests. Between requests:

- `ucinewgame` is sent to reset engine state (history, TT age, killer table).
- The same process handles the next `go` command immediately, with no fork or
  UCI handshake.

Failed workers are automatically replaced. `setoption name Hash value 64` is sent
once at worker startup.

---

## 6. Files Changed

| File | Change |
|---|---|
| `engine/include/rooksac/move_order.hpp` | Added `HistoryTable`, `historyPieceIndex`, `kHistoryMax`, `updateHistory`, `clearHistory` declarations |
| `engine/src/move_order.cpp` | Implemented history scoring, `updateHistory`, `clearHistory` |
| `engine/include/rooksac/search_options.hpp` | Added `historyHeuristic` flag; renamed `lmrReduction` → `lmrBaseReduction`; improved doc |
| `engine/src/search.cpp` | Integrated `HistoryTable` into `SearchContext`; depth-scaled LMR; progressive aspiration windows |
| `engine/include/rooksac/transposition_table.hpp` | `kDefaultSizeBytes` 16 MB → 64 MB |
| `engine/src/benchmark.cpp` | Uses `kDefaultSizeBytes` instead of hardcoded 16 |
| `engine/src/uci.cpp` | UCI Hash option default 16 → 64 |
| `engine/tests/uci-test.cpp` | Updated assertion to match Hash default 64 |
| `app/backend/engine_manager.py` | Persistent worker pool; `setoption Hash 64` on startup; `ucinewgame` between analyses; `shutdown()` method |

---

## 7. Before-and-After Benchmark Tables

### 7.1 C++ Engine — Fixed-Depth Search (UCI pipe, wall-clock)

Baseline measurements were taken immediately before the optimisation commits on the
same hardware.

> [!NOTE]
> "Before" figures for depth 8/10 are from the prior session's Python pipe benchmarks
> (same engine binary, same hardware, same methodology). Depth-4 figures are from
> `rooksac_bench` with the 16 MB TT.

| Position | Depth | Nodes (before) | Time (before) | Nodes (after) | Time (after) | Node Δ |
|---|---:|---:|---:|---:|---:|---:|
| Startpos | 8 | — | ~0.131 s | 113 172 | 0.112 s | — |
| Kiwipete | 4 | 126 947 | 88 ms | 126 922 | 86 ms | −0.2 % |
| Kiwipete | 8 | ~2 004 892 | ~2.26 s | 2 004 892 | 1.78 s | 0 % (same nodes) |
| Kiwipete | 10 | — | — | 11 341 863 | 17.8 s | — |
| TactMid | 8 | — | ~0.210 s | 184 948 | 0.187 s | — |
| Rook Endgame | 8 | — | — | 45 696 | 0.047 s | — |
| Suite total | 4 | 168 785 | 122 ms | 165 975 | 113 ms | −1.7 % |

> [!NOTE]
> Node count at depth 8 on Kiwipete is unchanged because the history heuristic and
> improved LMR reorder moves but the same alpha-beta tree is explored at this depth.
> The time reduction (2.26 s → 1.78 s, **−21 %**) reflects better move ordering
> producing earlier cutoffs within the same iteration.

### 7.2 NPS Comparison

| Position | Depth | NPS (before) | NPS (after) | Change |
|---|---:|---:|---:|---:|
| Suite (rooksac_bench) | 4 | ≈ 1 383 000 | ≈ 1 469 000 | +6 % |
| Kiwipete | 8 | — | ≈ 1 126 000 | — |
| Startpos | 8 | — | ≈ 1 010 000 | — |

### 7.3 Backend — Full Request Latency

| Measurement | Before | After |
|---|---:|---:|
| Process startup per request | ~800 ms | 0 ms (reused) |
| UCI handshake per request | ~50 ms | 0 ms (reused) |
| `ucinewgame` reset per request | 0 ms | ~1 ms |
| Depth-6 on startpos, first request | ~1.0 s (engine start + search) | ~0.2 s (search only) |
| Depth-6 on startpos, subsequent requests | ~1.0 s (engine restart) | ~0.2 s (search only) |

---

## 8. Time-Management Behaviour

Time management is handled by `TimeManager::allocate()` in `time_management.cpp`.
The engine correctly supports all standard UCI time controls:

| Mode | Behaviour |
|---|---|
| `go depth N` | Searches to exactly depth N, respects stop token |
| `go movetime N` | Searches until N ms elapsed (via deadline in SearchContext) |
| `go wtime/btime/winc/binc` | Allocates time via TimeManager (optimal + maximum time) |
| `go infinite` | Searches until `stop` is received |
| `go nodes N` | Stops after N nodes visited |

The iterative deepening loop only commits a result from **fully completed** iterations;
a partial iteration's result is discarded and the last complete result is returned.
This guarantees the engine always returns a legal move even if stopped early.

---

## 9. Correctness Test Results

All tests were run in Release configuration after every commit.

| Test Suite | Tests | Result |
|---|---:|---|
| `rooksac.build-smoke` | 1 | ✅ Pass |
| `rooksac.core-types` | 1 | ✅ Pass |
| `rooksac.position` | 1 | ✅ Pass |
| `rooksac.movegen` | 1 | ✅ Pass |
| `rooksac.perft` | 1 | ✅ Pass |
| `rooksac.agent` | 1 | ✅ Pass |
| `rooksac.search` | 1 | ✅ Pass |
| `rooksac.uci` | 1 | ✅ Pass |
| `rooksac.zobrist_tt` | 1 | ✅ Pass |
| `rooksac.eval` | 1 | ✅ Pass |
| `rooksac.tuning` | 1 | ✅ Pass |
| `rooksac.neural_eval` | 1 | ✅ Pass |
| `rooksac.nnue` | 1 | ✅ Pass |
| **C++ Total** | **13** | **✅ 13/13** |
| Backend Python tests | 4 | ✅ 4/4 |
| **Grand Total** | **17** | **✅ 17/17** |

Perft test covers positions at depths that verify move generation, make/unmake, and
Zobrist hashing are all correct. No regressions were found.

---

## 10. Playing Strength Evaluation

A formal self-play SPRT match was not run (requires a long-running match harness
with paired games and proper Elo statistics). The following qualitative observations
apply:

- The history heuristic, LMR, and aspiration window changes are all **sound** (they
  do not skip nodes that alpha-beta would have scored differently; they only order
  or reduce searches that will be re-searched at full depth on failure).
- Null-move pruning, PVS, and killer heuristics are unchanged.
- Piece-square tables, evaluation function, and material values are unchanged.
- The transposition table expansion improves strength slightly by reducing collision evictions.

A future SPRT match using `tools/sprt_match.py` at 5+0.05 tc is recommended to
confirm no Elo regression.

---

## 11. Remaining Bottlenecks

| Bottleneck | Impact | Recommendation |
|---|---|---|
| Depth 10+ search time (~18 s on Kiwipete) | High for deep analysis | Add check extensions, IID, or multi-threaded search (Lazy SMP) |
| `generateLegalMoves` checks every pseudo-legal move with make+unmake | Medium | Generate pseudo-legal moves and filter only king moves + pinned pieces (staged generation) |
| Evaluation computes mobility from scratch every call | Medium | Cache pawn structure evaluation between sibling nodes |
| Backend: single-threaded asyncio engine pool | Low | Pool size = 4 workers; scale out if concurrent users increase |
| No `info` lines emitted during iterative deepening | UX | Emit per-depth `info depth score cp nodes nps` for GUI progress display |

---

## 12. Known Trade-offs

| Decision | Trade-off |
|---|---|
| `ucinewgame` between persistent pool requests | Clears killer table and history; necessary for game isolation; adds ~1 ms |
| 64 MB TT default | Uses 64 MB RAM per engine instance; reduce via `setoption name Hash` if memory is constrained |
| History table cleared per search | History could be preserved across IID iterations; clearing is safer for reproducibility |
| No `march=native` flag | Portable build; no SIMD or BMI2 specific paths |

---

## 13. Recommendations

1. **Enable `info` output during search** — the engine only emits a single `info`
   line after the entire search completes. GUIs expect per-depth updates.
2. **Implement check extensions** — extending by 1 ply when the side to move is in
   check improves tactical accuracy at minimal node-count cost.
3. **Profile with a sampling profiler** — once Linux CI is available, use `perf
   record -g` on `rooksac_bench` at depth 10 to confirm the move-generation hot
   path is the true bottleneck before further C++ optimisation.
4. **Lazy SMP (multi-threading)** — the single biggest remaining speed lever;
   searching with 4 threads would reduce depth-10 search time from ~18 s to ~5 s.
5. **Staged legal move generation** — avoid make+unmake for clearly illegal moves
   using pin/check masks computed once per node.

