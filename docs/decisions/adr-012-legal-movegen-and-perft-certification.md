# ADR-012 — Legal move filtering, game evaluation, and perft certification

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Chess engines must only search and return strictly legal moves. In standard chess, moves are illegal if they leave the mover's king in check. Furthermore, castling has strict legality preconditions: the king must not currently be in check, and neither the transit square nor the destination square may be under attack by any enemy piece. En-passant captures can expose the king via discovered horizontal rays across the 5th/4th ranks. Perft (path enumeration performance test) is the universally trusted gold standard for certifying that move generation, rules, special cases, and make/unmake state transitions are 100% correct across millions of nodes.

## Decision

1. **Filtering Pseudo-Legal Moves by King Safety:** Legal move generation filters pseudo-legal moves using make/unmake. A candidate move is executed on the position via `Position::makeMove`; if the moving side's king square is attacked by any enemy piece (`attacks::isSquareAttacked`), the move is illegal and discarded. The position is restored via `Position::unmakeMove`.
2. **Authoritative Castling Legality Rules:** Castling moves (e1g1, e1c1, e8g8, e8c8) explicitly check:
   - King is not currently in check (`!isInCheck(position)`).
   - Neither the transit square (f1/d1/f8/d8) nor the target square (g1/c1/g8/c8) is attacked by the opponent.
3. **Discovered Check En-Passant Legality:** Handled naturally by transactional make/unmake. When a pawn captures en passant, both the capturing pawn and the captured pawn are updated on the bitboards before king safety is tested, correctly detecting horizontal discovered checks through the vacated file.
4. **Game State Evaluation:** `movegen::evaluateGameState` checks the legal move count and king attack status:
   - 0 legal moves + king in check -> `GameResult::Checkmate`.
   - 0 legal moves + king not in check -> `GameResult::Stalemate`.
   - Halfmove clock >= 100 -> `GameResult::FiftyMoveDraw`.
   - Repetition draw tracking is documented as limited to the active search tree history until a dedicated repetition table is added.
5. **Perft and Divide Diagnostics:** `perft::perft(Position&, int)` recursively enumerates all leaf nodes to depth $N$. `perft::divide(Position&, int)` computes leaf counts partitioned by each root legal move to localize any rule discrepancies.
6. **Certified Test Suite:** Certified against canonical reference positions from the Chess Programming Wiki:
   - Start position (depths 1-4).
   - Position 2: Kiwipete (depths 1-3).
   - Position 3 (depths 1-4).
   - Position 4 (depths 1-3).
   - Position 5 (depths 1-3).

## Alternatives considered

- **Direct Legal Generation (generating only evasions when in check):** More complex and error-prone around double checks and absolute pins. Pseudo-legal generation + king safety check provides a simple, bulletproof single authoritative path that passes all canonical perft suites.
- **Separate Perft Binary Executable:** A standalone diagnostic binary can be exposed via tools, but integrating the perft engine directly into `rooksac_core` and `rooksac_perft_test` ensures automated CI execution without build bloat.

## Consequences

Legal move generation is certified, complete, and verified against millions of chess positions. Search algorithms (Sprint 08) and evaluation functions can rely completely on the legality guarantee of `movegen::generateLegalMoves`.
