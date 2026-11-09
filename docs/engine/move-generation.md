# Move generation

## Concept and value

Pseudo-legal moves obey how pieces move but may leave the moving side’s king in check. Legal moves additionally satisfy the king-safety rule. Correct move generation is foundational: search, evaluation comparisons, checkmate detection, and UCI output all depend on it.

## Implemented legal move generation and Perft certification

RookSac implements:

- Precomputed `constexpr` lookup tables for pawn attacks, knight attacks, and king attacks.
- Robust 4-direction ray stepping for bishop and rook attacks, with queen attacks defined as their union.
- Attack predicates: `attacks::attacksByColor` for global attack bitboards and `attacks::isSquareAttacked` for targeted check/threat queries.
- Bounded `MoveList` stack container (`std::array<Move, 256>`) avoiding heap allocations.
- Pseudo-legal move generators in `rooksac::movegen`: `generatePseudoLegalMoves`, `generatePseudoLegalCaptures`, and `generatePseudoLegalQuiets`.
- Legal move filtering in `rooksac::movegen`: `generateLegalMoves`, `generateLegalCaptures`, `isLegalMove`, and check detection `isInCheck`/`isKingInCheck`.
- Castling legality: verified that the king is not in check and that transit and destination squares are unattacked.
- En-passant legality: full transactional simulation detecting discovered checks along rank 4/5.
- Game state evaluation: `evaluateGameState` detecting `Checkmate`, `Stalemate`, `FiftyMoveDraw`, and `Ongoing`.
- Standalone Perft & Divide engine in `rooksac::perft`: certified against Chess Programming Wiki reference positions (Start pos, Kiwipete, Pos 3, Pos 4, Pos 5).

## Draw rule support and limitations

- **Fifty-move rule:** Fully supported (`halfmoveClock >= 100`).
- **Stalemate:** Fully supported (no legal moves and side to move is not in check).
- **Threefold repetition:** Limited to active branch history until a persistent Zobrist transposition table is introduced in later sprints.

## Planned search integration

Move ordering, MVV-LVA, alpha-beta search, and quiescence search will be implemented in subsequent sprints.

## Alternatives and trade-offs

Pseudo-legal generation plus filtering is straightforward to validate but can do extra work. Direct legal generation can reduce work but complicates pins, checks, and edge cases. Sliding attacks may use simple rays first and later a proven lookup approach if measured.

## Edge cases and testing

Pinned pieces, double check, discovered attacks, castling through attack, en-passant discovered checks, promotion choices, checkmate, and stalemate require dedicated tests. Perft and divide output are the principal future validation tools; see [perft](../testing/perft.md).

## Future improvements and references

Optimize only after verified correctness and profiling. See [board representation](board-representation.md) and [Move Generation](https://www.chessprogramming.org/Move_Generation).
