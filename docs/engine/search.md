# Search

## Concept and progression

Search explores legal move trees to select a move. RookSac plans this progression: Negamax; alpha-beta pruning; move ordering; iterative deepening; quiescence search; transposition tables and Zobrist hashing; principal variation search; aspiration windows; killer moves and history heuristics; then only justified advanced work.

| Technique | Status | Problem solved / expected benefit | Trade-off |
| --- | --- | --- | --- |
| Negamax | Implemented | Uniform two-player score recursion with mate distance | Explores all branches; large tree. |
| Alpha-beta | Implemented | Cuts branches that cannot affect the result | Depends heavily on ordering. |
| Ordering | Implemented | Finds useful cutoffs earlier | Staging and scoring overhead. |
| Iterative deepening | Planned (Sprint 11) | Produces usable partial results and ordering | Repeats shallow work. |
| Quiescence | Planned (Sprint 11) | Reduces unstable capture-horizon errors | Can grow excessively without limits. |
| TT / hashing | Planned (Sprint 12) | Reuses repeated-position search | Needs correct keys and bounds. |
| PVS / aspiration | Planned | Narrows common-case search windows | Requires robust re-search behavior. |
| Killer / history | Planned | Learns ordering from prior cutoffs | Can bias ordering and needs aging policy. |

## Implemented architecture

Depth-limited Alpha-Beta search with staged move ordering is implemented in `rooksac/search.hpp`, `engine/src/search.cpp`, `rooksac/move_order.hpp`, and `engine/src/move_order.cpp`:
- **Fail-Soft Alpha-Beta:** Implements fail-soft bounds ($\alpha, \beta$). On beta cutoff ($s \ge \beta$), search terminates early and reports the cut score and increments `stats.cutoffs`.
- **Staged Move Ordering:** Moves are scored before branching:
  1. Capture + Promotion (30,000+)
  2. Promotion (20,000+)
  3. MVV-LVA Captures (10,000+)
  4. Quiet moves (0)
  Moves within each bucket preserve their stable generation order.
- **Minimax Differential Verification:** Plain `searchMinimax` is maintained alongside `search` (Alpha-Beta) to prove that Alpha-Beta produces mathematically identical scores across depths.
- **Terminal States Before Leaf Evaluation:** Terminal conditions (50-move draw, checkmate, stalemate) are evaluated before checking `depth == 0`.
- **Mate-Distance Scoring:** Delivering mate scores `kMateScore - ply` ($30000 - \text{ply}$), and receiving mate scores $-kMateScore + \text{ply}$.
- **`NegamaxAgent`:** Uses Alpha-Beta pruning with move ordering for rapid move selection in matches.

## Edge cases, testing, and future work

Differential tests between plain Minimax and Alpha-Beta, fail-low, fail-high, exact score, check evasions, and promotions are covered by `rooksac_search_test` in `engine/tests/search-test.cpp`. Quiescence search and iterative deepening follow in Sprint 11. See [ADR-015](../decisions/adr-015-alpha-beta-pruning-and-move-ordering.md), [TT](transposition-table.md), and [Alpha-Beta](https://www.chessprogramming.org/Alpha-Beta).
