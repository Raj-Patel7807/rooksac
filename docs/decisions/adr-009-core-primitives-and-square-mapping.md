# ADR-009 — Core primitives and square mapping

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Position parsing, move generation, hashing, and UCI coordinate conversion need one stable square convention and compact value types. A mismatch in rank direction or a shift using an invalid square can silently corrupt every later subsystem.

## Decision

Use strongly typed C++20 value types in the `rooksac` namespace. Squares use `a1 = 0` through `h8 = 63`, with files increasing from `a` to `h` and ranks from `1` to `8`. `Square::None` is the only sentinel and bitboard helpers return an empty mask or no square for it, so they never shift by 64.

Pieces use explicit color/type encodings. Moves pack source, target, and promotion in 15 bits; promotions encode knight, bishop, rook, and queen. Special move flags are intentionally absent until a position-aware subsystem can validate them.

## Alternatives considered

- Use `a8 = 0` or a mailbox-only square mapping.
- Expose untyped integers for squares and pieces.
- Add castling, en-passant, and double-pawn-push flags before position rules exist.

## Consequences

Bitboard masks, algebraic coordinates, FEN handling, and future UCI conversion share one mapping. Header-only primitives have no mutable global state or runtime dependency. Callers still need position-aware validation for chess legality; these value types do not generate moves or enforce rules.
