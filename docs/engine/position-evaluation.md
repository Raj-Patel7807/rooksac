# Position evaluation

## Concept and value

Evaluation estimates a position’s desirability when search does not reach a terminal result. Material evaluation counts piece value; positional evaluation considers where and how pieces and pawns are arranged. Both are needed because equal material positions can differ greatly.

## Implemented material evaluation and baseline agents

RookSac implements:
- `eval::evaluateMaterial(const Position&, const MaterialWeights&)`: Computes net material balance in centipawns from the perspective of White (positive favors White).
- `eval::evaluateMaterialSideToMove(const Position&, const MaterialWeights&)`: Computes net material balance relative to the side to move.
- `eval::MaterialWeights`: Configurable scale (default: P=100, N=320, B=330, R=500, Q=900).
- `Agent` pure virtual interface for move selection policies.
- `RandomAgent`: Uniform random legal move selector with an injectable 64-bit seed (`std::mt19937_64`).
- `GreedyAgent`: 1-ply greedy agent that evaluates all candidate moves via make/unmake and selects the move maximizing immediate material score.
- `playMatch` and `rooksac_match_harness`: Automated self-play match execution between any two agents.

### Known limitations of greedy baseline

`GreedyAgent` evaluates solely based on the material balance immediately after its own candidate move. It has zero depth lookahead into opponent replies, zero understanding of piece defense, king safety, or positional value. Consequently, it will greedily capture defended pieces with higher-value attackers (e.g. QxP where P is defended by another pawn).

## Planned positional evaluation

Positional evaluation terms—piece-square tables, pawn structure, king safety, mobility, and endgame phase awareness—remain planned for future sprints.

## Alternatives and trade-offs

A hand-crafted evaluator is inspectable and incremental but requires tuning. Neural evaluators may model richer patterns but add training, data, and integration complexity; they are future research, not Phase 0 scope.

## Edge cases and testing

Draw conditions, perspective consistency, king exposure, blocked passed pawns, and phase transitions must be considered. Use curated positions, term-level tests, search stability checks, and documented tuning experiments—not unsupported strength claims.

## Future improvements and references

Parameter tuning and NNUE research remain future options. See [search](search.md) and [Evaluation](https://www.chessprogramming.org/Evaluation).
