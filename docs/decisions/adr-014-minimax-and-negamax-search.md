# ADR-014 — Minimax and Negamax game-tree search

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Following the random and greedy baseline agents implemented in Sprint 08, the engine requires a depth-limited game-tree search that accounts for opponent responses and tactical look-ahead. Unpruned Negamax establishes the mathematical foundation of zero-sum two-player tree search, ensuring search correctness, deterministic reproducibility, terminal score conventions, and state preservation invariants before pruning algorithms (such as Alpha-Beta) are introduced.

## Decision

1. **Negamax Formulation:** Negamax formulation is adopted as the primary search algorithm instead of separate maximizing and minimizing procedures. In zero-sum games, $\max(a, b) = -\min(-a, -b)$, enabling a single recursive function where child evaluations are negated (`score = -negamax(..., depth - 1, ply + 1)`).
2. **Side-to-Move Score Convention:** Scores are consistently evaluated and returned from the perspective of the side to move at each node. A positive score favors the side to move.
3. **Terminal Detection Before Static Evaluation:** Terminal state checks (50-move rule, checkmate, stalemate) are evaluated before leaf node static evaluation (`depth == 0`). This prevents leaf nodes from misclassifying terminal checkmates or stalemates as ordinary static material positions.
4. **Mate-Distance Scoring:** Checkmate scores are represented as `kMateScore - ply` when delivering mate and `-kMateScore + ply` when receiving mate. This guarantees that the engine prefers the fastest checkmate and delays inevitable defeat. The base checkmate score (`kMateScore = 30000`) is reserved far above ordinary centipawn material bounds.
5. **Configurable Leaf Evaluator:** The search accepts a `LeafEvaluator` callable (defaulting to `eval::evaluateMaterialSideToMove`), allowing alternative or weighted evaluation functions without coupling search logic to board or evaluator internals.
6. **Isolated Search Module:** Search definitions and algorithms reside in `rooksac/search.hpp` and `engine/src/search.cpp`, keeping search independent of protocol handling (UCI) and board structures.
7. **`NegamaxAgent` Integration:** `NegamaxAgent` implements the standard `Agent` interface, enabling depth-limited search policies to participate in automated matches via `playMatch` and the `rooksac_match_harness` CLI tool.
8. **Mathematical Equivalence Verification:** Classical Minimax is maintained as a reference in tests to prove that Negamax produces mathematically identical scores and decisions across depths and positions.

## Alternatives considered

- **Direct Alpha-Beta Pruning:** Implementing Alpha-Beta immediately was deferred to Sprint 10 to establish an unpruned, verified baseline first.
- **White-Relative Global Scores:** Rejected in favor of side-to-move-relative scoring because side-to-move-relative simplifies recursive formulation, transposition table storage, and alpha-beta window manipulation.

## Consequences

RookSac now possesses look-ahead capability, correctly avoiding short-horizon blunders (such as sacrificing queens for defended pawns) and finding forced mates within the search horizon. Full state restoration is guaranteed across all search recursions.
