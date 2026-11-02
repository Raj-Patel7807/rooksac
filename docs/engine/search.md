# Search

## Concept and progression

Search explores legal move trees to select a move. RookSac plans this progression: Negamax; alpha-beta pruning; move ordering; iterative deepening; quiescence search; transposition tables and Zobrist hashing; principal variation search; aspiration windows; killer moves and history heuristics; then only justified advanced work.

| Technique | Status | Problem solved / expected benefit | Trade-off |
| --- | --- | --- | --- |
| Negamax | Implemented | Uniform two-player score recursion with mate distance | Explores all branches; large tree. |
| Alpha-beta | Planned (Sprint 10) | Cuts branches that cannot affect the result | Depends heavily on ordering. |
| Ordering | Planned (Sprint 10) | Finds useful cutoffs earlier | Heuristics need validation. |
| Iterative deepening | Planned (Sprint 10) | Produces usable partial results and ordering | Repeats shallow work. |
| Quiescence | Planned (Sprint 11) | Reduces unstable capture-horizon errors | Can grow excessively without limits. |
| TT / hashing | Planned (Sprint 11) | Reuses repeated-position search | Needs correct keys and bounds. |
| PVS / aspiration | Planned | Narrows common-case search windows | Requires robust re-search behavior. |
| Killer / history | Planned | Learns ordering from prior cutoffs | Can bias ordering and needs aging policy. |

## Implemented architecture

Depth-limited Negamax search is implemented in `rooksac/search.hpp` and `engine/src/search.cpp`:
- **Side-to-Move Perspective:** All evaluations and returns are relative to the player to move at that node (`score = -negamax(..., depth - 1, ply + 1)`).
- **Terminal States Before Leaf Evaluation:** Terminal conditions (50-move draw, checkmate, stalemate) are evaluated before checking `depth == 0`, ensuring leaf checkmates are scored accurately.
- **Mate-Distance Scoring:** Delivering mate scores `kMateScore - ply` ($30000 - \text{ply}$), and receiving mate scores $-kMateScore + \text{ply}$. Faster mates are preferred and losses are delayed.
- **Configurable Leaf Evaluator:** Accepts any callable `std::function<std::int32_t(const Position&)>`, defaulting to `eval::evaluateMaterialSideToMove`.
- **`NegamaxAgent`:** Wraps search into the common `Agent` policy interface for match execution and self-play.

## Edge cases, testing, and future work

Terminal positions, fifty-move draw, mate-distance normalization, and state restoration are covered by `rooksac_search_test` in `engine/tests/search-test.cpp`. Equivalent classical Minimax is validated as a mathematical baseline. Alpha-beta pruning and move ordering follow in Sprint 10. See [ADR-014](../decisions/adr-014-minimax-and-negamax-search.md), [TT](transposition-table.md), and [Alpha-Beta](https://www.chessprogramming.org/Alpha-Beta).
