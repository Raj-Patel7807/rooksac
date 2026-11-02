# ADR-018 — Zobrist hashing and memory-bounded transposition table

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Search trees frequently encounter transpositions — identical positions reached via distinct move orderings. Without caching, identical subtrees are evaluated repeatedly. To resolve this, the engine requires a compact, fast position signature and a transposition table (TT) to store search results, bounds, and best moves. The implementation must ensure incremental hash correctness during make/unmake, prevent hash collisions from producing invalid moves or scores, properly normalize mate scores across ply depths, and support configurable memory sizing under UCI.

## Decision

1. **Deterministic Zobrist Hashing (`rooksac::zobrist`):**
   - Implemented 64-bit pseudo-random keys generated deterministically via a SplitMix64 generator seeded with `kDefaultSeed = 0x180f42b3917a26c5ULL`.
   - Keys cover all piece-square pairs ($12 \times 64$), side to move (XOR if Black), 16 castling rights bit combinations ($2^4$), and 8 en-passant file targets ($0..7$).
   - The key is incrementally maintained during `Position::makeMove()` and restored upon `Position::unmakeMove()` using `UndoState::zobristHash`.
   - `zobrist::computeHash()` is provided as an exact reference function, asserted in `Position::checkInvariants()`.
2. **Transposition Table (`rooksac::tt::TranspositionTable`):**
   - Configurable memory footprint ranging from 1 MB to 1024 MB (default 16 MB).
   - Cache entries are tightly packed into 16-byte structs: 64-bit verification key, 16-bit move encoding, 16-bit signed score, 8-bit depth, 8-bit bound (`Exact`, `Lower`, `Upper`), 8-bit generation/age, and 8-bit padding.
   - **Replacement Policy:** Depth-preferred with generational aging. Entries are overwritten if the slot is empty, matches key, new depth $\ge$ stored depth, or the stored entry is from a prior generation.
3. **Mate Score Normalization:**
   - To make stored mate evaluations independent of the ply at which they were found, scores within $[kMateScore - kMaxMatePly, kMateScore]$ are transformed on storage: `scoreToTT(score, ply) = score + ply`, and reversed on probe: `scoreFromTT(score, ply) = score - ply`.
4. **Integration with Search & Move Ordering:**
   - In `alphaBetaInternal()`, TT probe hits with sufficient depth (`entry.depth >= depth`) trigger immediate score cutoffs if the stored bound satisfies alpha-beta window conditions (`Exact`, `Lower >= beta`, `Upper <= alpha`).
   - The stored TT best move is prioritized with highest score (`kPvMoveBaseScore = 40'000`) in move ordering, accelerating cutoffs even when the depth is insufficient for an immediate score cutoff.
   - Evaluated root and node outcomes are committed to the TT with their corresponding bound.
5. **UCI Interoperability:**
   - Advertises `option name Hash type spin default 16 min 1 max 1024` on `uci`.
   - Dynamically reallocates TT memory when `setoption name Hash value <N>` is received.
   - `ucinewgame` clears the table.

## Alternatives considered

- **Unbounded Hash Map (`std::unordered_map`):** Rejected due to dynamic allocation overhead and cache misses in the search hot path.
- **Partial 32-bit Hash Keys:** Rejected to avoid hash collisions resulting in catastrophic misplays. A full 64-bit verification key provides near-zero collision probability across tournament games.

## Consequences

RookSac achieves significant node reductions on transpositions, retains best-move guidance across iterative deepening depths, and exposes standard memory configuration to UCI GUIs.
