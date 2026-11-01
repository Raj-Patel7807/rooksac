# Board representation

## Concept and value

A bitboard is a 64-bit integer whose bits correspond one-for-one with chess squares. Bitwise AND, OR, XOR, shifts, and population counts make sets of squares compact and efficient. A planned piece bitboard holds squares occupied by one color and piece type; color and total occupancy bitboards accelerate attack queries.

## Implemented primitives and position state

RookSac uses `a1 = 0` through `h8 = 63`: files increase from `a` to `h` and ranks from `1` to `8`. `Square::None` is an explicit sentinel; bit helpers return an empty mask for it and never shift by 64. The core headers provide strongly typed colors, pieces, files, ranks, squares, a 64-bit `Bitboard`, file/rank/edge/light/dark masks, and safe least-significant-bit operations using C++20 `<bit>`.

Moves compactly encode source, target, and optional knight/bishop/rook/queen promotion.

The `Position` class manages piece bitboards by `[color][piece_type]`, aggregate `colorOccupancy` bitboards, `totalOccupancy`, and a 64-square `mailbox_` array for O(1) piece lookup. It encapsulates:
- Side to move (`Color`)
- Castling rights (`CastlingRights` bitflags: `WK`, `WQ`, `BK`, `BQ`)
- En-passant target square (`Square`)
- Halfmove clock (`uint32_t`)
- Fullmove number (`uint32_t`)

Position supports transactional FEN parsing via `Position::fromFen(std::string_view)` returning `FenResult<Position>`, complete FEN serialization via `Position::toFen()`, and exact reversible move execution via `makeMove(Move, UndoState&)` and `unmakeMove(Move, const UndoState&)`.

## Planned engine design

Attack generation, legal move filtering, perft tooling, evaluation, and search remain unimplemented. When hashing is introduced, incremental Zobrist hashing will be maintained during make/unmake.

## Alternatives and trade-offs

A mailbox or 2D array is easier to inspect square by square and can simplify early code. Bitboards better match attack-set operations and common high-performance techniques, but demand precise indexing and careful debugging. A hybrid mailbox cache remains an option only if evidence justifies it.

## Edge cases and testing

State must distinguish side to move, lost castling rights, legal en-passant state, promotions, and counters. Future tests should round-trip FEN, test make/unmake restoration, validate occupancy invariants, and exercise unusual legal positions.

## Future work and references

Implementation details, incremental updates, and table layout remain open. See [move generation](move-generation.md), [glossary](../reference/glossary.md), and [Chess Programming Wiki](https://www.chessprogramming.org/Bitboards).
