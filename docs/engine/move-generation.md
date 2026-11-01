# Move generation

## Concept and value

Pseudo-legal moves obey how pieces move but may leave the moving side’s king in check. Legal moves additionally satisfy the king-safety rule. Correct move generation is foundational: search, evaluation comparisons, checkmate detection, and UCI output all depend on it.

## Implemented attack and pseudo-legal move generation

RookSac implements:
- Precomputed `constexpr` lookup tables for pawn attacks, knight attacks, and king attacks.
- Robust 4-direction ray stepping for bishop and rook attacks, with queen attacks defined as their union.
- Attack predicates: `attacks::attacksByColor` for global attack bitboards and `attacks::isSquareAttacked` for targeted check/threat queries.
- Bounded `MoveList` stack container (`std::array<Move, 256>`) avoiding heap allocations.
- Pseudo-legal move generators in `rooksac::movegen`: `generatePseudoLegalMoves`, `generatePseudoLegalCaptures`, and `generatePseudoLegalQuiets`. These generate all piece movements, double pawn pushes, promotions, en-passant captures, and castling (path emptiness verified) without filtering for king safety.

## Planned legal move filtering and certification

King safety filtering, check evasion, pinned piece constraints, castling through check verification, en-passant discovered check validation, checkmate/stalemate detection, and perft certification will be implemented in Sprint 07.

## Alternatives and trade-offs

Pseudo-legal generation plus filtering is straightforward to validate but can do extra work. Direct legal generation can reduce work but complicates pins, checks, and edge cases. Sliding attacks may use simple rays first and later a proven lookup approach if measured.

## Edge cases and testing

Pinned pieces, double check, discovered attacks, castling through attack, en-passant discovered checks, promotion choices, checkmate, and stalemate require dedicated tests. Perft and divide output are the principal future validation tools; see [perft](../testing/perft.md).

## Future improvements and references

Optimize only after verified correctness and profiling. See [board representation](board-representation.md) and [Move Generation](https://www.chessprogramming.org/Move_Generation).
