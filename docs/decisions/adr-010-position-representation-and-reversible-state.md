# ADR-010 — Position state representation, FEN parser, and reversible make/unmake

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Search algorithms apply and undo millions of moves per second. If a move changes a castling right, en-passant square, halfmove clock, captured piece, or piece bitboard, and undo fails to restore any single field, the entire search and state validation corrupts. Furthermore, external chess state enters the engine via FEN strings, requiring strict validation to prevent malformed inputs from leaving positions in partially modified or invalid states.

## Decision

1. **Dual Bitboard + Mailbox Representation:** `Position` maintains piece bitboards indexed by `[color][piece_type]`, aggregate `colorOccupancy` bitboards, and `totalOccupancy` for fast bitwise queries. It also maintains a 64-element square mailbox (`std::array<Piece, 64>`) to enable O(1) piece queries at source and target squares during move application and undo.
2. **Strict Transactional FEN Parsing:** FEN strings are parsed into an isolated local `Position` object that commits only upon full validation of all six fields (piece placement, side to move, castling rights, en passant, halfmove clock, fullmove number). Syntactic and semantic errors return structured enum codes via `FenResult<Position>`, leaving caller state untouched.
3. **Explicit Reversible `UndoState`:** Reversible move metadata is stored in `UndoState` (`enPassantSquare`, `castlingRights`, `halfmoveClock`, `capturedPiece`).
4. **Make/Unmake Transaction Semantics:** `makeMove` updates castling rights when kings or rooks move or rooks are captured, clears/sets en passant targets, decrements/resets clocks, and moves pieces. `unmakeMove` exactly reverses piece placements (including castling rook moves, en-passant captured pawns, and promotions) and restores prior metadata from `UndoState`.
5. **Invariant Checking in Debug:** `checkInvariants()` validates mutual bitboard exclusivity, color occupancy alignment, total occupancy consistency with the mailbox, en-passant rank validity, and positive fullmove counters.

## Alternatives considered

- **Bitboards Only Without Mailbox:** Querying piece at square would require checking up to 12 bitboards. Mailbox lookup makes piece queries O(1) and simplifies piece removal/placement.
- **Copying Entire Position on Move:** Allocating or copying the full position struct on every move search node avoids unmake logic but imposes substantial memory copying overhead in deep trees.
- **Throwing Exceptions on FEN Errors:** Avoided in favor of zero-overhead structured error codes (`FenParseError`) and `FenResult<Position>` to keep engine core freestanding and deterministic.

## Consequences

The engine has an authoritative, verifiable board state and reversible move transitions ready for pseudo-legal attack generation (Sprint 06) and legal move filtering (Sprint 07). Callers can parse FEN safely and undo move sequences with exact state equivalence.
