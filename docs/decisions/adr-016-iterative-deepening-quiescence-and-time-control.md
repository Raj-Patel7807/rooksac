# ADR-016 — Iterative deepening, quiescence search, and cooperative time control

- **Status:** Accepted
- **Date:** 2026-11-02

## Context

Alpha-Beta search (Sprint 10) significantly reduced game-tree node counts. However, fixed-depth search remains vulnerable to the classic "horizon effect" — misjudging a position because a critical piece exchange is cut off just past the nominal search depth. Additionally, fixed-depth search is impractical for clocked games because search times grow exponentially and cannot be predicted reliably. An interruptible search controller is required that progressively deepens, stabilizes leaf evaluations via tactical quiescence, and terminates cooperatively under clock constraints without risking state corruption or empty fallback responses.

## Decision

1. **Iterative Deepening:** The search loop deepens progressively from depth 1 to the configured maximum depth. Only fully completed depth iterations commit their score, depth, and best move to `lastCompletedResult`. If an iteration is interrupted midway by time or an external stop token, the partial iteration is safely discarded and the last completed iteration's result is returned.
2. **Guaranteed Legal Fallback Move:** Before depth 1 begins, `lastCompletedResult.bestMove` is initialized with the first legal move from the root position. This guarantees that regardless of when an interruption occurs (even after 0 milliseconds), the engine always returns a valid, strictly legal move.
3. **Principal Variation (PV) First Ordering:** The best move discovered at iteration $d - 1$ is assigned top priority (`kPvMoveBaseScore = 40'000`) in the root move list for iteration $d$, ensuring that the most promising branch is explored first to maximize subsequent Alpha-Beta cutoffs.
4. **Quiescence Search with Stand-Pat:** Leaf nodes (`depth == 0`) transition into `quiescence()` to resolve tactical captures and promotions.
   - **Non-Check (Stand-Pat):** The static evaluation is computed. If `standPat >= beta`, a fail-soft cutoff is returned immediately. Only legal captures and promotions are generated and searched.
   - **In-Check (Evasion Requirement):** If the king is currently in check, stand-pat is strictly prohibited. All legal evasions must be generated and searched. If no evasions exist, checkmate is scored (`-kMateScore + ply`).
   - **Depth Safeguard:** Quiescence search depth is bounded by `kMaxQsearchDepth = 32` plies to prevent infinite tactical loops.
5. **Time Management (`TimeManager`):** A dedicated `TimeManager` component computes `optimalTimeMs` and `maximumTimeMs` from clock parameters (`remainingTimeMs`, `incrementMs`, `movesToGo`) incorporating a configurable safety reserve (`kDefaultSafetyReserveMs = 50ms`) to absorb communication latency and OS scheduling jitter. Emergency clocks (down to 1ms) are handled gracefully without division-by-zero or negative allocation.
6. **Cooperative Stop Token:** Search cancellation utilizes a thread-safe `SearchStopToken` (`std::atomic<bool>`), checked cooperatively at iteration starts and periodically every 1,024 nodes. Asynchronous thread cancellation or detachment is strictly avoided to preserve `Position` make/unmake invariants.

## Alternatives considered

- **Hard Asynchronous Thread Termination:** Rejected because interrupting execution mid-move leaves the board state mutated, corrupting `UndoState` and mailbox bitboard consistency.
- **Delta Pruning in Quiescence:** Deferred until full positional evaluation terms are introduced, ensuring that baseline quiescence search remains 100% tactically sound.

## Consequences

RookSac completely avoids tactical horizon blunders (such as sacrificing queens for defended pawns), supports responsive time-controlled searches with fixed time, clock, and node limits, and guarantees continuous legal fallback moves.

