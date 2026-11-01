# Board representation

## Concept and value

A bitboard is a 64-bit integer whose bits correspond one-for-one with chess squares. Bitwise AND, OR, XOR, shifts, and population counts make sets of squares compact and efficient. A planned piece bitboard holds squares occupied by one color and piece type; color and total occupancy bitboards accelerate attack queries.

## Planned RookSac design

The preferred, not yet implemented, direction is bitboards with one documented square-index convention (to be fixed before code), piece and occupancy sets, side to move, castling rights, an en-passant target or absence, halfmove clock, fullmove number, and a Zobrist key when hashing arrives. FEN is the external text representation of this complete state.

## Alternatives and trade-offs

A mailbox or 2D array is easier to inspect square by square and can simplify early code. Bitboards better match attack-set operations and common high-performance techniques, but demand precise indexing and careful debugging. A hybrid mailbox cache remains an option only if evidence justifies it.

## Edge cases and testing

State must distinguish side to move, lost castling rights, legal en-passant state, promotions, and counters. Future tests should round-trip FEN, test make/unmake restoration, validate occupancy invariants, and exercise unusual legal positions.

## Future work and references

Implementation details, incremental updates, and table layout remain open. See [move generation](move-generation.md), [glossary](../reference/glossary.md), and [Chess Programming Wiki](https://www.chessprogramming.org/Bitboards).
