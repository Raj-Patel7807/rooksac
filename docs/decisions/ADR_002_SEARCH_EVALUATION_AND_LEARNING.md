# ADR 002: Search, Evaluation, and Learning Architecture

## Status
Accepted (Consolidated from ADR-014 through ADR-024)

## Context
A competitive chess engine demands deep, stable search pruning, robust transposition caching, balanced evaluation combining classical heuristics with modern neural techniques, and data-driven parameter tuning.

## Consolidated Decisions

### 1. Minimax & Negamax Framework (Historical ADR-014)
- Search operates in standard Negamax framework where scores are relative to the side to move (`score(player) = -negamax(opponent)`).
- Terminal detection handles checkmate with ply-distance scoring (`-kMateScore + ply`) ensuring shortest-path checkmates, and draw detection for stalemate, 50-move rule, and 3-fold repetition.

### 2. Alpha-Beta Pruning, Quiescence Search, & Move Ordering (Historical ADR-015, ADR-016)
- Alpha-Beta pruning with fail-soft bounds.
- Quiescence Search (`qsearch`): Searches tactical moves (captures and promotions) at leaf nodes with standing pat cutoffs and delta pruning to eliminate the horizon effect.
- Move Ordering hierarchy:
  1. Transposition table (TT) best move (PV move).
  2. Winning and equal captures ordered by MVV-LVA (Most Valuable Victim - Least Valuable Attacker).
  3. Killer moves (up to 2 non-capture quiet moves per ply that caused beta cutoffs).
  4. History heuristic / quiet moves.
- Iterative Deepening with dynamic time management (`move_time_ms`, soft/hard boundaries) ensuring safe interruption and anytime search return.

### 3. Zobrist Hashing & Transposition Table (Historical ADR-018)
- 64-bit Zobrist hashing keys initialized with deterministic pseudo-random seeds.
- Cache entries store 64-bit key, search depth, evaluation score (adjusted for mate plies), node bound type (`EXACT`, `LOWER_BOUND`, `UPPER_BOUND`), and best move.
- Replacement scheme: Depth-preferred with generational aging.

### 4. Advanced Classical Search & Benchmarking (Historical ADR-020)
- **Aspiration Windows**: Searches initial depths with narrow $[\alpha - \delta, \beta + \delta]$ window around previous iteration's score, widening exponentially upon fail-high or fail-low.
- **Principal Variation Search (PVS)**: Searches first move with full window; subsequent moves with null-window $[\alpha, \alpha + 1]$, re-searching full window only upon fail-high.
- **Late Move Reductions (LMR)**: Reduces search depth by 1 ply for late-ordered quiet moves at depth $\ge 3$.
- **Null-Move Pruning (NMP)**: Bypasses side-to-move turn with $R=2$ reduction when side has non-pawn material and is not in check.
- **Standard Benchmark Suite (`rooksac_bench`)**: 8 canonical positions covering startpos, tactical midgames, queen/rook endgames, and kingside attacks.

### 5. Classical Tapered Evaluation & Texel Tuning (Historical ADR-019, ADR-021)
- Piece-square tables (PST) and material values defined for both Opening and Endgame phases.
- Dynamic phase calculation based on remaining non-pawn material, interpolating evaluation score:
  $$\text{score} = \frac{\text{mg\_score} \cdot \text{phase} + \text{eg\_score} \cdot (24 - \text{phase})}{24}$$
- Texel Tuning framework optimizing evaluation coefficients by minimizing Mean Squared Error (MSE) between sigmoid win probability and actual game outcomes from master PGN datasets.
- Wald's Sequential Probability Ratio Test (SPRT) evaluating candidate parameter sets against baseline under Elo hypotheses $(H_0: \text{Elo} \le 0, H_1: \text{Elo} \ge 5)$.

### 6. Neural Network Research & Native Inference Integration (Historical ADR-022, ADR-023, ADR-024)
- **Supervised Policy-Value Pipeline (`research/nn/`)**: PyTorch pipeline using 18-plane feature tensors (12 piece planes, turn, castling rights, en passant) and 4,096-move action vocabulary, trained with joint loss ($\mathcal{L}_{\text{CE}} + c \cdot \mathcal{L}_{\text{MSE}}$).
- **Native C++ Neural Evaluator (`rooksac::NeuralEvaluator`)**: Native zero-allocation inference kernel evaluating position leaves directly inside C++ search, calibrated to centipawns via inverse sigmoid.
- **NNUE Research Track (`rooksac::NNUEEvaluator`)**: HalfKP-style 832 sparse feature representation with int16 dual accumulators supporting incremental `updateAccumulator` on make/unmake moves with exact parity tests.

## Consequences
- Highly efficient search engine combining deep pruning, move ordering, and stable transposition caching.
- Zero heap allocation throughout search and evaluation loops.
- Multi-path evaluation options (classical tapered, neural leaf, and NNUE accumulator).
