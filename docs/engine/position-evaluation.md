# Position evaluation

## Concept and value

Evaluation estimates a position’s desirability when search does not reach a terminal result. Material evaluation counts piece value; positional evaluation considers where and how pieces and pawns are arranged. Both are needed because equal material positions can differ greatly.

## Implemented Architecture

RookSac implements a modern classical tapered evaluation framework (`rooksac::eval`):
- **Centipawn Scale**: 100 cp = 1 pawn.
- **Perspective Convention**: Absolute evaluations compute from White's perspective (positive favors White, negative favors Black). Side-to-move evaluations (`evaluateSideToMove()`) return positive scores favoring the active side.
- **Tapered Score Representation (`eval::Score`)**: Every term accumulates a pair of values `(mg, eg)` representing Middlegame and Endgame evaluations.
- **Game Phase Blending**: Game phase is determined by non-pawn material (Knights: 1, Bishops: 1, Rooks: 2, Queens: 4), with a total opening phase of 24.
  $$\text{FinalScore} = \frac{\text{score.mg} \times \text{phase} + \text{score.eg} \times (24 - \text{phase})}{24}$$

## Modular Evaluation Features

1. **Material & Piece-Square Tables (PSTs)**:
   - Configurable piece values and 64-square tables for each piece type.
   - Colour-mirrored for Black: $\text{squareIndex} \oplus 56$.
2. **Pawn Structure**:
   - Doubled pawn penalties (multiple pawns on the same file).
   - Isolated pawn penalties (no friendly pawns on adjacent files).
   - Passed pawn bonuses scaled by advancement rank, with higher bonuses in the endgame.
3. **King Safety**:
   - Missing shield pawn penalties on ranks 2 and 3 near castled kings.
   - Semi-open file penalties near the king in the middlegame.
4. **Piece Activity & Mobility**:
   - Bishop pair bonus.
   - Rook activity bonuses on open (no pawns) and semi-open (only enemy pawns) files.
   - Safe piece mobility counting legal/pseudo-legal rays avoiding enemy pawn attacks.
5. **Centralized Tunable Parameters**:
   - All weights are encapsulated in `eval::EvalParameters` (`eval_types.hpp`), enabling structured parameter management and future automated tuning.
6. **Baseline Evaluator Retained**:
   - `eval::evaluateMaterial()` and `eval::evaluateMaterialSideToMove()` remain available for benchmark testing and baseline agents.

## Performance and Throughput

Evaluation is allocation-free and uses bitboard operations and precomputed ray-stepping attacks (`attacks::pawnAttacks`, `attacks::bishopAttacks`, etc.). Measured throughput exceeds 650,000 evaluations per second.
