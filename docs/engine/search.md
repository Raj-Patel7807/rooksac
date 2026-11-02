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
| TT / hashing | Planned (Sprint 12) | Reuses repeated-position search | Needs correct keys and bounds. |
| PVS / aspiration | Planned | Narrows common-case search windows | Requires robust re-search behavior. |
| Killer / history | Planned | Learns ordering from prior cutoffs | Can bias ordering and needs aging policy. |

## Implemented architecture

Iterative-deepening Alpha-Beta search with quiescence, PV-first move ordering, and cooperative time control is implemented in `rooksac/search.hpp`, `engine/src/search.cpp`, `rooksac/move_order.hpp`, `engine/src/move_order.cpp`, `rooksac/time_management.hpp`, and `engine/src/time_management.cpp`:

- **Iterative Deepening:** `search()` loops from depth 1 to the requested limit, keeping the best result from the last completed iteration. A legal fallback move is guaranteed even if the first iteration is interrupted.
- **Cooperative Stop Token:** `SearchStopToken` (wraps `std::atomic<bool>`) is checked at the start of each iterative deepening iteration and every `kStopCheckInterval` (1024) nodes inside the search. The outer caller signals it; the search reads it without blocking.
- **Quiescence Search:** At `depth == 0`, `alphaBetaInternal` calls `quiescenceInternal` rather than returning the static evaluation. Quiescence expands capture moves, stands pat when the current evaluation is already good enough (stand-pat pruning), and returns to a quiet position before evaluating. Checks in quiescence are expanded fully. Maximum quiescence depth is `kMaxQsearchDepth` (32).
- **PV-First Move Ordering:** The principal variation move from the previous iteration is passed to `orderMoves` and scored at `kPvMoveBaseScore` (40,000), placing it above all other moves.
- **Staged Move Ordering:** All non-PV moves are scored before branching:
  1. Capture + Promotion (30,000+)
  2. Promotion (20,000+)
  3. MVV-LVA Captures (10,000+)
  4. Quiet moves (0)
  Moves within each bucket preserve their stable generation order.
- **Fail-Soft Alpha-Beta:** Implements fail-soft bounds ($\alpha, \beta$). On beta cutoff ($s \ge \beta$), search terminates early and reports the cut score and increments `stats.cutoffs`.
- **Minimax Differential Verification:** Plain `searchMinimax` (no quiescence, no pruning) is maintained alongside `search` to prove search correctness on quiet positions.
- **Terminal States Before Leaf Evaluation:** Terminal conditions (50-move draw, checkmate, stalemate) are evaluated before checking `depth == 0`.
- **Mate-Distance Scoring:** Delivering mate scores `kMateScore - ply` ($30000 - \text{ply}$), and receiving mate scores $-kMateScore + \text{ply}$.
- **`NegamaxAgent`:** Uses iterative deepening with Alpha-Beta pruning, quiescence, and PV-first move ordering for rapid move selection in matches.
- **Time Management:** `TimeManager::allocate()` in `time_management.cpp` converts `TimeControl` limits into `TimeAllocation` (`optimalMs`, `maximumMs`) using a sudden-death divisor formula, safety reserves, and emergency-clock handling. See [ADR-016](../decisions/adr-016-iterative-deepening-quiescence-and-time-control.md) and [time management](time-management.md).

## Edge cases, testing, and future work

Tests covering quiescence tactical horizon, check-evasion in quiescence, iterative deepening fallback on pre-signaled stop, cooperative stop token, time allocation formula, node/time limits, and search determinism are in `rooksac_search_test` (`engine/tests/search-test.cpp`). Transposition tables and Zobrist hashing follow in Sprint 12. See [ADR-015](../decisions/adr-015-alpha-beta-pruning-and-move-ordering.md), [ADR-016](../decisions/adr-016-iterative-deepening-quiescence-and-time-control.md), [TT](transposition-table.md), and [Alpha-Beta](https://www.chessprogramming.org/Alpha-Beta).
