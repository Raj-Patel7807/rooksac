# Transposition tables and hashing

## Concept and value

Different move sequences often lead to identical board positions (transpositions). A Transposition Table (TT) caches the search results of evaluated positions so that when a state is revisited through a different move order or during iterative deepening, the engine can either immediately return a verified score cutoff or utilize the cached best move to guide move ordering.

## Zobrist Hashing Architecture

RookSac uses deterministic 64-bit Zobrist hashing (`rooksac::zobrist`):
- **Components XORed**:
  - Piece-square keys for each of 12 piece types across 64 squares ($12 \times 64$).
  - Side to move key (XORed if Black is to move).
  - Castling rights keys (16 possible binary configurations of White/Black Kingside/Queenside).
  - En-passant file keys ($0..7$ for target files).
- **Incremental Maintenance**: The Zobrist hash is maintained incrementally with $\mathcal{O}(1)$ XOR operations inside `Position::makeMove()` and restored upon `Position::unmakeMove()` using `UndoState::zobristHash`.
- **Integrity Validation**: `zobrist::computeHash()` is provided to compute the key from scratch. In debug and testing builds, `Position::checkInvariants()` asserts that the incremental hash exactly matches `computeHash()`.

## Transposition Table Design

`rooksac::tt::TranspositionTable` is a cache allocated as a flat array of 16-byte `Entry` structs:
- **`key` (64-bit)**: Full 64-bit verification key to eliminate collision misplays.
- **`move` (16-bit)**: Encoded best move for move ordering prioritization.
- **`score` (16-bit)**: Normalized centipawn score or mate score.
- **`depth` (8-bit)**: Nominal search depth at which the position was searched.
- **`bound` (8-bit)**: `Exact` (PV node), `Lower` (fail-high / beta cutoff), or `Upper` (fail-low).
- **`age` (8-bit)**: Generational counter advanced at search or iteration start to support generational replacement.

### Mate Score Normalization

Mate scores depend on distance from the root. To store mate evaluations independently of the search ply:
$$\text{scoreToTT}(\text{score}, \text{ply}) = \begin{cases} \text{score} + \text{ply} & \text{if winning mate} \\ \text{score} - \text{ply} & \text{if losing mate} \\ \text{score} & \text{otherwise} \end{cases}$$
Upon probing, the reverse translation is applied to restore correct distance relative to the probing ply.

### Replacement Policy

Entries are replaced when:
1. The table slot is uninitialized (`Bound::None`).
2. The stored key matches the probe key.
3. The new search depth is greater than or equal to the stored entry depth (`depth >= entry.depth`).
4. The stored entry belongs to an older search generation (`entry.age != currentAge`).

## UCI Integration

The engine advertises `option name Hash type spin default 16 min 1 max 1024` on handshake. Sending `setoption name Hash value <N>` resizes the transposition table dynamically.
