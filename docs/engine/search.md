# Search

## Concept and progression

Search explores legal move trees to select a move. RookSac plans this progression: Negamax; alpha-beta pruning; move ordering; iterative deepening; quiescence search; transposition tables and Zobrist hashing; principal variation search; aspiration windows; killer moves and history heuristics; then only justified advanced work.

| Technique | Status | Problem solved / expected benefit | Trade-off |
| --- | --- | --- | --- |
| Negamax | Implemented | Uniform two-player score recursion with mate distance | Explores all branches; large tree. |
| Alpha-beta | Implemented | Cuts branches that cannot affect the result | Depends heavily on ordering. |
| Ordering | Implemented | Finds useful cutoffs earlier | Staging and scoring overhead. |
| Iterative deepening | Implemented | Produces usable partial results and ordering | Repeats shallow work. |
| Quiescence | Implemented | Reduces unstable capture-horizon errors | Can grow excessively without limits. |
| TT / hashing | Implemented | Reuses repeated-position search | Needs correct keys and bounds. |
| PVS / aspiration | Implemented | Narrows common-case search windows | Requires robust re-search behavior. |
| Killer / history | Implemented (killers) | Learns ordering from prior cutoffs | Can bias ordering and needs aging policy. |
| Null-move pruning | Implemented | Fast forward-pruning on strong positions | Zugzwang risk without safeguards. |
| LMR | Implemented | Reduces depth of late quiet moves | Requires full re-search on fail-high. |

## Implemented architecture

Iterative-deepening Alpha-Beta search with quiescence, PV-first move ordering, aspiration windows, Principal Variation Search (PVS), Late Move Reductions (LMR), Null-Move Pruning (NMP), killer move heuristics, and cooperative time control is implemented in `rooksac/search.hpp`, `engine/src/search.cpp`, `rooksac/move_order.hpp`, `engine/src/move_order.cpp`, `rooksac/time_management.hpp`, and `engine/src/time_management.cpp`:

- **Iterative Deepening:** `search()` loops from depth 1 to the requested limit, keeping the best result from the last completed iteration. A legal fallback move is guaranteed even if the first iteration is interrupted.
- **Aspiration Windows:** Narrow initial window $[\text{score}-\Delta, \text{score}+\Delta]$ ($\Delta = 50\text{cp}$) around the prior iteration score. On fail-low or fail-high, bounds are widened to infinity and re-searched.
- **Principal Variation Search (PVS):** First move is searched with full window $[\alpha, \beta]$; subsequent moves are searched with null window $[\alpha, \alpha+1]$. If a null-window search fails high ($>\alpha$), a full-depth, full-window re-search is triggered.
- **Late Move Reductions (LMR):** For quiet moves at index $\ge 3$ and depth $\ge 3$ (when not in check and not a capture), search depth is reduced by 1 ply. Any fail-high triggers an immediate full-depth re-search.
- **Null-Move Pruning (NMP):** Passing a turn ($R=3$) when not in check, not at a PV node, and possessing at least one non-pawn piece (zugzwang safeguard).
- **Killer Move Heuristic:** Two quiet move slots per ply recorded on beta cutoff, scored at `kKillerBaseScore` (5,000) between captures and quiet moves.
- **Search Options Isolation:** All advanced techniques are controlled via `SearchOptions`. `SearchOptions::vanilla()` disables all enhancements for diagnostic and differential baseline runs.
- **Cooperative Stop Token:** `SearchStopToken` (wraps `std::atomic<bool>`) is checked at the start of each iterative deepening iteration and every `kStopCheckInterval` (1024) nodes inside the search. The outer caller signals it; the search reads it without blocking.
- **Quiescence Search:** At `depth == 0`, `alphaBetaInternal` calls `quiescenceInternal` rather than returning the static evaluation. Quiescence expands capture moves, stands pat when the current evaluation is already good enough (stand-pat pruning), and returns to a quiet position before evaluating. Checks in quiescence are expanded fully. Maximum quiescence depth is `kMaxQsearchDepth` (32).
- **PV-First Move Ordering:** The principal variation move from the previous iteration is passed to `orderMoves` and scored at `kPvMoveBaseScore` (40,000), placing it above all other moves.
- **Staged Move Ordering:** All non-PV moves are scored before branching:
  1. Capture + Promotion (30,000+)
  2. Promotion (20,000+)
  3. MVV-LVA Captures (10,000+)
  4. Killer Moves (5,000+)
  5. Quiet moves (0)
  Moves within each bucket preserve their stable generation order.
- **Fail-Soft Alpha-Beta:** Implements fail-soft bounds ($\alpha, \beta$). On beta cutoff ($s \ge \beta$), search terminates early and reports the cut score and increments `stats.cutoffs`.
- **Minimax Differential Verification:** Plain `searchMinimax` (no quiescence, no pruning) is maintained alongside `search` to prove search correctness on quiet positions.
- **Terminal States Before Leaf Evaluation:** Terminal conditions (50-move draw, checkmate, stalemate) are evaluated before checking `depth == 0`.
- **Mate-Distance Scoring:** Delivering mate scores `kMateScore - ply` ($30000 - \text{ply}$), and receiving mate scores $-kMateScore + \text{ply}$.
- **`NegamaxAgent`:** Uses iterative deepening with Alpha-Beta pruning, quiescence, and PV-first move ordering for rapid move selection in matches.
- **Time Management:** `TimeManager::allocate()` in `time_management.cpp` converts `TimeControl` limits into `TimeAllocation` (`optimalMs`, `maximumMs`) using a sudden-death divisor formula, safety reserves, and emergency-clock handling. See [ADR-016](../decisions/adr-016-iterative-deepening-quiescence-and-time-control.md) and [time management](time-management.md).

## Edge cases, testing, and future work

Tests covering quiescence tactical horizon, check-evasion in quiescence, iterative deepening fallback on pre-signaled stop, cooperative stop token, time allocation formula, node/time limits, search determinism, search options isolation, aspiration widening, null-move pruning safety, and killer moves are in `rooksac_search_test` (`engine/tests/search-test.cpp`). See [ADR-015](../decisions/adr-015-alpha-beta-pruning-and-move-ordering.md), [ADR-016](../decisions/adr-016-iterative-deepening-quiescence-and-time-control.md), [ADR-020](../decisions/adr-020-advanced-search-and-benchmark.md), [TT](transposition-table.md), and [Alpha-Beta](https://www.chessprogramming.org/Alpha-Beta).
