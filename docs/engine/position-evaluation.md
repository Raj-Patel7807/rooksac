# Position evaluation

## Concept and value

Evaluation estimates a position’s desirability when search does not reach a terminal result. Material evaluation counts piece value; positional evaluation considers where and how pieces and pawns are arranged. Both are needed because equal material positions can differ greatly.

## Planned RookSac design

The planned classical path starts with material, then piece-square tables, activity and mobility, pawn structure, king safety, passed pawns, endgame factors, and game-phase awareness. Terms should have clear ownership, documented sign convention, and tests where a deterministic property can be asserted.

## Alternatives and trade-offs

A hand-crafted evaluator is inspectable and incremental but requires tuning. Neural evaluators may model richer patterns but add training, data, and integration complexity; they are future research, not Phase 0 scope.

## Edge cases and testing

Draw conditions, perspective consistency, king exposure, blocked passed pawns, and phase transitions must be considered. Use curated positions, term-level tests, search stability checks, and documented tuning experiments—not unsupported strength claims.

## Future improvements and references

Parameter tuning and NNUE research remain future options. See [search](search.md) and [Evaluation](https://www.chessprogramming.org/Evaluation).
