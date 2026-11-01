# ADR-011 — Piece attack generation, MoveList container, and pseudo-legal move generation

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Chess move generation and king safety require attack detection. Evaluating whether a king is in check or whether squares are guarded requires attack maps that ignore whether the attacking piece is pinned to its own king (a piece attacks a square even if moving would be illegal). Furthermore, search performance requires move generation without heap allocations, and separating pseudo-legal move generation from king safety checks simplifies testing and validation before perft certification (Sprint 07).

## Decision

1. **Precomputed Leaper Attack Tables:** Pawn, knight, and king attack bitboards are precomputed at compile time as `constexpr` lookup tables (`attacks::kPawnAttacks`, `attacks::kKnightAttacks`, `attacks::kKingAttacks`) without runtime allocation or startup cost.
2. **Ray-Stepping Baseline for Sliders:** Bishop and rook attacks use 4-direction ray-stepping that scans until the edge or the first blocking piece (friendly or enemy), capturing the blocker. Queen attacks are the union of bishop and rook rays. This baseline is transparent, robust, and free of complex magic bitboard hashing dependencies.
3. **Attack Predicates:** `attacks::attacksByColor` computes aggregate attack bitboards. `attacks::isSquareAttacked` evaluates whether an enemy piece attacks a specific target square by reversing ray queries against attacking piece bitboards.
4. **Stack-Allocated `MoveList`:** `MoveList` encapsulates a fixed-size `std::array<Move, 256>` with bounded size tracking. Theoretical maximum pseudo-legal moves in chess is 218; 256 provides safe capacity while remaining lightweight on the call stack.
5. **Pseudo-Legal Separation:** `movegen::generatePseudoLegalMoves`, `generatePseudoLegalCaptures`, and `generatePseudoLegalQuiets` produce all structurally valid piece moves, double pawn pushes, promotions, en-passant captures, and castling path empties for the side to move without king safety validation. Legal move filtering is isolated to Sprint 07.

## Alternatives considered

- **Magic Bitboards for Sliders Immediately:** Magic bitboards provide faster slider attacks via precalculated hash tables, but introduce significant initialization complexity and larger binary footprints. Ray stepping provides a solid baseline for validation; magic bitboards can be introduced later if profiled and measured.
- **Dynamic `std::vector<Move>` Container:** Dynamically allocating vectors during move generation introduces heap contention and cache misses on hot search paths.
- **Direct Legal Move Generation:** Filtering king in check during generation complicates pin detection and discovered checks, making validation harder than generating pseudo-legal moves followed by king safety checks.

## Consequences

Attack detection and pseudo-legal move generation are deterministic, modular, and verified. Sprint 07 can directly filter pseudo-legal move lists using `attacks::isSquareAttacked` on the king's square after `makeMove`, enabling perft certification and check detection.
