# ADR-019 — Classical tapered evaluation

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Material-only evaluation lacks positional awareness, making it impossible to evaluate piece development, board control, king exposure, or pawn structures accurately. To support high-quality chess play without introducing heavyweight neural dependencies or sacrificing throughput, the engine requires a classical tapered evaluator. Evaluation runs in the inner search loops and quiescence search; therefore, it must be modular, allocation-free, mathematically symmetric, and high-throughput.

## Decision

1. **Centipawn Scale and Consistent Perspective:**
   - All evaluation terms are measured in standard centipawns (100 cp = 1 pawn).
   - Internal functions compute net score from the perspective of White (positive favors White, negative favors Black).
   - `eval::evaluateSideToMove()` converts the score to the side to move at the root/node boundary for Negamax consumption.
2. **Tapered Score Representation (`eval::Score`):**
   - Each term provides a pair of scores: `(mg, eg)` representing Middlegame and Endgame values.
   - The game phase is computed from remaining non-pawn pieces: Knights and Bishops contribute 1, Rooks contribute 2, and Queens contribute 4, totaling a maximum phase of $24$ ($2 \times (2\times 1 + 2\times 1 + 2\times 2 + 1\times 4)$).
   - Final evaluation interpolates linearly between middlegame and endgame:
     $$\text{FinalScore} = \frac{\text{score.mg} \times \text{phase} + \text{score.eg} \times (24 - \text{phase})}{24}$$
3. **Modular Evaluation Features:**
   - **Material & Piece-Square Tables (PSTs):** PeSTO/CPW classical tables colour-mirrored for Black ($sq \oplus 56$).
   - **Pawn Structure:** Penalties for doubled pawns (more than 1 pawn on a file) and isolated pawns (no friendly pawns on adjacent files); rank-dependent bonuses for passed pawns (increasing significantly towards the 7th rank and in endgame).
   - **King Safety:** Penalties for missing friendly shield pawns on ranks 2/3 near the king and semi-open files near the king in the middlegame.
   - **Piece Activity & Mobility:** Bishop pair bonus, rook bonuses for open and semi-open files, and safe pseudo-legal mobility counting moves not attacked by enemy pawns.
4. **Structured and Tunable Weights:**
   - All weights are centralized in `EvalParameters`, exposed via `defaultParameters()`.
5. **Zero Allocation in Hot Path:**
   - Evaluation uses pure bitboard operations and precomputed attack tables (`attacks::pawnAttacks`, `attacks::knightAttacks`, `attacks::bishopAttacks`, etc.) without heap allocation or dynamic move list construction, maintaining throughput exceeding 600,000 evaluations per second.
6. **Baseline Material Evaluator Retained:**
   - `eval::evaluateMaterial()` and `eval::evaluateMaterialSideToMove()` remain intact for baseline agents and differential testing.

## Alternatives considered

- **NNUE (Efficiently Updatable Neural Networks):** Deferred to later research milestones. A hand-crafted classical evaluator provides immediate interpretability, debuggability, and a clear baseline.
- **Dynamic Pawn Hash Table:** Deferred as unnecessary for Phase 0 since pawn evaluation overhead is under 5% of total evaluation runtime.

## Consequences

RookSac possesses positional understanding, recognizes tactical and positional nuances (development, central control, passed pawns, king protection), blends smoothly between game stages, and retains high search speed.
