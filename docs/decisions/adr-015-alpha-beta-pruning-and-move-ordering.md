# ADR-015 — Alpha-Beta pruning, fail-soft bounds, and staged move ordering

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Sprint 09 established a mathematically verified, depth-limited Negamax search. However, unpruned game-tree exploration incurs exponential node growth ($O(b^d)$), severely restricting practical search depth. Alpha-Beta pruning eliminates subtrees that cannot influence the final root decision, drastically reducing node count while guaranteeing mathematical equivalence with the exhaustive minimax score. The efficiency of Alpha-Beta pruning depends critically on move ordering: evaluating the best moves first allows the remaining branches to be refuted with early beta cutoffs.

## Decision

1. **Fail-Soft Alpha-Beta Pruning:** Implement Alpha-Beta pruning using fail-soft bounds ($\alpha, \beta$). When a move returns a score $s \ge \beta$, the search terminates immediately (beta cutoff / fail-high) and returns $s$ rather than truncating to $\beta$. This preserves valuable bound information for future aspiration windows and root evaluations.
2. **Preservation of Exhaustive Minimax Path:** Retain `negamax` and `searchMinimax` as unpruned reference search paths dedicated to shallow differential testing. This ensures continuous verification that Alpha-Beta never diverges from the true minimax value.
3. **Staged Move Ordering:** Introduce isolated move ordering in `rooksac/move_order.hpp` and `engine/src/move_order.cpp`. Move priorities are staged into four explicit tiers:
   - **Tier 1 (30,000+):** Captures with promotion (MVV-LVA score + promotion piece value).
   - **Tier 2 (20,000+):** Quiet promotions (promotion piece value).
   - **Tier 3 (10,000+):** Captures scored by MVV-LVA ($\text{victimValue} \times 10 - \text{attackerValue}$).
   - **Tier 4 (0):** Quiet moves.
4. **Deterministic and Stable Ordering:** Moves within the same priority tier preserve their original generation order via `std::stable_sort` over a stack-allocated `ScoredMove` array. No heap allocations occur during ordering.
5. **Node and Cutoff Tracking:** `SearchStats` records total node visits (`nodes`), beta cutoffs (`cutoffs`), and maximum depth reached (`maxDepth`).
6. **No Speculative Pruning in Sprint 10:** Techniques with potential horizon or tactical risk (Late Move Reductions, Null-Move Pruning, Futility Pruning) are deliberately deferred to ensure that this baseline Alpha-Beta implementation remains completely lossless.

## Alternatives considered

- **Fail-Hard Alpha-Beta:** Rejected because fail-soft returns richer bound values without increasing node count, and is better suited for future principal variation search (PVS).
- **Inline Sorting in Search Loop:** Rejected to maintain modular separation of concerns; move ordering logic is fully decoupled from the recursive search function.

## Consequences

Alpha-Beta achieves dramatic tree pruning (e.g. 27,462 nodes reduced to 785 nodes at depth 3 on tactical benchmark positions — a ~35x reduction) while producing byte-for-byte identical minimax scores and legal moves across all test positions.
