# ADR 001: Native Chess Engine Core Architecture

## Status

Accepted (Consolidated from ADR-007 through ADR-013)

## Context

RookSac is an autonomous, high-performance chess engine written in clean, modern C++20. A correct, performant, and maintainable engine requires an unambiguous bitboard board representation, efficient attack masking, reversible move execution, strictly legal move generation certified by Perft suites, and clean separation between search, evaluation, and user interfaces.

## Consolidated Decisions

### 1. Repository Layout & Decoupled Architecture (Historical ADR-007)

- **Engine Core**: Located under `engine/` (`engine/include/rooksac` for headers, `engine/src` for implementation, `engine/apps` for CLI binaries, `engine/tests` for GoogleTest suites).
- **Tooling & ML**: Python scripts and training code are strictly kept outside the native engine build under `tools/`, `research/`, and `app/`. The engine hot path contains zero runtime Python dependencies.

### 2. Legal Provenance & Clean-Room Development (Historical ADR-008)

- RookSac is engineered clean-room under permissive MIT licensing.
- Copying GPL-licensed or third-party engine code (e.g., Stockfish) is forbidden. All algorithms (magic bitboards, search heuristics, evaluation terms) are authored natively based on standard chess programming literature.

### 3. Core Bitboard Primitives & Square Mapping (Historical ADR-009)

- Little-Endian Rank-File (LERF) mapping: Square 0 is `a1`, square 63 is `h8`.
- Bitboards are strongly typed as `uint64_t` with compiler-intrinsic bit manipulation (`std::countr_zero`, `rooksac::popCount`).
- Directional shifting invariants: `North = +8`, `South = -8`, `East = +1`, `West = -1`. File wrap guards prevent bit leakage across files A and H.

### 4. Position Representation & Reversible State (Historical ADR-010)

- `Position` holds 12 piece bitboards (`WhitePawn` through `BlackKing`), color occupancy bitboards, side-to-move, castling rights byte, en passant square, halfmove clock, and fullmove number.
- State reversibility: `makeMove` records irrevocable history (`StateHistory` struct containing previous castling rights, en passant, halfmove clock, and captured piece) allowing exact zero-loss state restoration via `unmakeMove`.
- FEN parsing and serialization strictly conform to standard Forsyth-Edwards Notation with malformed input rejection.

### 5. Attack Generation & Magic Bitboards (Historical ADR-011)

- Non-sliding piece attacks (Pawns, Knights, Kings) use precomputed static lookup tables.
- Sliding piece attacks (Rooks, Bishops, Queens) use Fancy Magic Bitboards: hash indexing through precomputed 64-bit magics and occupancy masks for $O(1)$ attack lookups without branching.

### 6. Legal Move Generation & Perft Certification (Historical ADR-012)

- Generate pseudo-legal moves and filter by validating that the moving side's king is not in check post-move.
- Certified against standard Perft reference suites (Initial position, Kiwipete, Positions 3, 4, 5) up to depth 6 matching exact canonical node counts.

### 7. Baseline Agents & Material Evaluation (Historical ADR-013)

- Decoupled `Agent` interface (`engine/include/rooksac/agent.hpp`) establishing `selectMove(const Position&)` abstraction.
- Baseline agents include `RandomAgent` and `MaterialAgent` for sanity testing, deterministic baseline comparison, and regression defense.

## Consequences

- Clean, robust C++20 foundation.
- Full state reversibility with zero heap allocation during search.
- Verified perft correctness on all standard tactical and endgame test suites.
