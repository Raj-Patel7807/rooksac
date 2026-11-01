# Move generation

## Concept and value

Pseudo-legal moves obey how pieces move but may leave the moving side’s king in check. Legal moves additionally satisfy the king-safety rule. Correct move generation is foundational: search, evaluation comparisons, checkmate detection, and UCI output all depend on it.

## Planned RookSac design

Future generation will cover pawn pushes and captures, knight, bishop, rook, queen, and king moves; captures, promotions, en passant, castling, check detection, and no-legal-move outcomes. The initial approach may generate pseudo-legal moves then filter after a reversible state transition; alternative direct legal generation can be evaluated later.

## Alternatives and trade-offs

Pseudo-legal generation plus filtering is straightforward to validate but can do extra work. Direct legal generation can reduce work but complicates pins, checks, and edge cases. Sliding attacks may use simple rays first and later a proven lookup approach if measured.

## Edge cases and testing

Pinned pieces, double check, discovered attacks, castling through attack, en-passant discovered checks, promotion choices, checkmate, and stalemate require dedicated tests. Perft and divide output are the principal future validation tools; see [perft](../testing/perft.md).

## Future improvements and references

Optimize only after verified correctness and profiling. See [board representation](board-representation.md) and [Move Generation](https://www.chessprogramming.org/Move_Generation).
