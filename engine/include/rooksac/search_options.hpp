#pragma once

#include <cstdint>

namespace rooksac {
namespace search {

/// \brief Runtime feature flags controlling which search enhancements are active.
///
/// Each flag can be toggled independently so that techniques can be isolated,
/// measured, and disabled without recompiling. The defaults match empirically
/// validated conservative settings.
///
/// Invariant: disabling all flags produces plain iterative-deepening alpha-beta,
/// which is preserved as the baseline for comparison and diagnosis.
struct SearchOptions {
    /// \brief Enable aspiration windows around the expected score from the previous iteration.
    ///
    /// On failure (score outside the window), the window is widened progressively
    /// (delta *= 2) and re-searched. Falls back to a full [-inf, +inf] window after
    /// two consecutive failures. Only applied at root depth >= 2.
    bool aspirationWindows = true;

    /// \brief Initial half-width of the aspiration window in centipawns.
    std::int32_t aspirationDelta = 50;

    /// \brief Enable Principal Variation Search (PVS / Negascout).
    ///
    /// After the first move is searched with the full window, subsequent moves are
    /// searched with a null window [alpha, alpha+1]. If a move exceeds alpha (fail-high),
    /// it is re-searched with the full window. Requires good move ordering to be effective.
    bool pvSearch = true;

    /// \brief Enable null-move pruning (NMP) with adaptive reduction.
    ///
    /// When not in check and sufficient material is present (zugzwang safeguard),
    /// the engine makes a "null move" (passes its turn) and searches at reduced depth.
    /// If the result is still >= beta the position is considered to fail-high and the
    /// branch is pruned.
    bool nullMovePruning = true;

    /// \brief Reduction applied to null-move search depth in plies.
    std::uint32_t nullMoveReduction = 3;

    /// \brief Minimum non-pawn piece count per side to permit null-move pruning.
    /// Guards against zugzwang positions where the null-move assumption breaks down.
    std::int32_t nullMoveMinPieces = 1;

    /// \brief Enable Late Move Reductions (LMR).
    ///
    /// Moves searched late in the move list (well after the PV and captures) and at
    /// sufficient depth are searched at a depth-scaled reduction. On a fail-high
    /// (the reduced search exceeds alpha), the position is re-searched at full depth.
    /// Using conservative conditions avoids missing tactical solutions.
    bool lmr = true;

    /// \brief Minimum search depth at which LMR is applied.
    std::uint32_t lmrMinDepth = 3;

    /// \brief Minimum move index (0-based) beyond which LMR is applied.
    std::uint32_t lmrMoveThreshold = 3;

    /// \brief Base LMR reduction in plies (depth-scaled formula adds depth/5 on top).
    std::uint32_t lmrBaseReduction = 1;

    /// \brief Enable the killer move heuristic.
    ///
    /// Two quiet moves per ply that caused a beta cutoff in a sibling node are stored
    /// and ordered just below captures. This improves pruning without full history tables.
    bool killerMoves = true;

    /// \brief Enable the history heuristic for quiet move ordering.
    ///
    /// A 12x64 table tracks how frequently each (piece, to-square) combination causes
    /// a beta cutoff at depth d (bonus = d^2). Quiet moves beyond killers are ordered
    /// by this score, producing better cutoffs in subsequent iterations.
    bool historyHeuristic = true;

    /// \brief Maximum ply depth for which killer moves are tracked.
    static constexpr std::uint32_t kMaxKillerPly = 64;

    /// \brief Number of killer move slots per ply (always 2 for the primary heuristic).
    static constexpr std::uint32_t kKillerSlots = 2;

    /// \brief Predefined "vanilla" configuration: all enhancements disabled.
    /// Produces plain alpha-beta with PV-first move ordering for differential testing.
    [[nodiscard]] static constexpr SearchOptions vanilla() noexcept {
        SearchOptions opts;
        opts.aspirationWindows  = false;
        opts.pvSearch           = false;
        opts.nullMovePruning    = false;
        opts.lmr                = false;
        opts.killerMoves        = false;
        opts.historyHeuristic   = false;
        return opts;
    }
};

}  // namespace search
}  // namespace rooksac
