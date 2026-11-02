# ADR-013 — Baseline agents, material evaluation, and self-play match harness

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Before developing complex search algorithms (Alpha-Beta, Quiescence, Iterative Deepening) or positional evaluation terms (piece-square tables, pawn structure), the engine needs to verify the end-to-end move-selection contract: a position enters, legal moves are enumerated, a decision policy selects one valid legal move, and the game progresses with exact state preservation. Baseline agents also provide reproducible baselines for benchmark and strength validation.

## Decision

1. **`Agent` Interface:** `Agent` defines a pure virtual interface with `selectMove(Position&, const MoveList&)` returning `std::optional<Move>`. If the legal move list is empty (terminal state), it returns `std::nullopt`. The position state is guaranteed to be restored to its exact original state upon return.
2. **`RandomAgent`:** Selects uniformly from available legal moves using an injectable 64-bit Mersenne Twister seed (`std::mt19937_64`) for fully deterministic and reproducible testing.
3. **`GreedyAgent`:** Evaluates all candidate moves with 1-ply lookahead using material balance alone. Makes candidate moves safely via make/unmake, evaluates net material in centipawns from the mover's perspective, and selects the maximum score.
4. **Configurable `MaterialWeights`:** Standard piece values (P=100, N=320, B=330, R=500, Q=900) are encapsulated in `MaterialWeights` with explicit centipawn scaling, supporting custom weighting for experiments.
5. **Documented Greedy Limitations:** Greedy agent ignores tactical replies, king exposure, piece defense, and positional features. For example, it will eagerly capture a defended pawn using a queen if it gives an immediate net +100 material score, oblivious to being recaptured next ply.
6. **Self-Play Match Harness:** `playMatch` executes an automated game between two agents with turn alternation, legal move validation, termination checks, and plies bounds. `rooksac_match_harness` provides a standalone CLI tool to run baseline matches.

## Alternatives considered

- **Full Negamax Alpha-Beta Baseline:** Deferred to Sprint 09 to keep baseline agents isolated, testable, and lightweight.
- **Random Move Selection in Main Search:** Randomness is strictly isolated to `RandomAgent` for testing and baseline play; production engine search remains fully deterministic.

## Consequences

End-to-end agent decision contracts are validated and automated. Future search and evaluation engines can implement `Agent` or use `playMatch` directly for regression and self-play testing.
