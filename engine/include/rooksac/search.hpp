#pragma once

#include <atomic>
#include <array>
#include <chrono>
#include <cstdint>
#include <functional>
#include <optional>

#include "rooksac/evaluation.hpp"
#include "rooksac/material.hpp"
#include "rooksac/move.hpp"
#include "rooksac/move_list.hpp"
#include "rooksac/position.hpp"
#include "rooksac/search_options.hpp"
#include "rooksac/time_management.hpp"
#include "rooksac/transposition_table.hpp"

namespace rooksac {
namespace search {

/// \brief Score constant representing practical infinity for minimax bounds.
inline constexpr std::int32_t kInfinity = 1'000'000;

/// \brief Base mate score for checkmate distance evaluations.
inline constexpr std::int32_t kMateScore = 30'000;

/// \brief Maximum search ply depth supported for mate-distance adjustments.
inline constexpr std::int32_t kMaxMatePly = 1'000;

/// \brief Score for terminal draws (stalemate, fifty-move rule).
inline constexpr std::int32_t kDrawScore = 0;

/// \brief Maximum depth in plies permitted inside quiescence search to prevent infinite loops.
inline constexpr std::uint32_t kMaxQsearchDepth = 32;

/// \brief Node check interval for checking timeout and cooperative stop token.
inline constexpr std::uint64_t kStopCheckInterval = 1024;

/// \brief Checks if a score indicates a checkmate sequence within known bounds.
[[nodiscard]] constexpr bool isMateScore(std::int32_t score) noexcept {
    return (score >= kMateScore - kMaxMatePly && score <= kMateScore) ||
           (score <= -kMateScore + kMaxMatePly && score >= -kMateScore);
}

/// \brief Thread-safe cooperative stop signal checked at safe intervals during search.
class SearchStopToken {
public:
    SearchStopToken() noexcept = default;

    void stop() noexcept { stop_.store(true, std::memory_order_relaxed); }
    void reset() noexcept { stop_.store(false, std::memory_order_relaxed); }
    [[nodiscard]] bool isStopped() const noexcept { return stop_.load(std::memory_order_relaxed); }

private:
    std::atomic<bool> stop_{false};
};

/// \brief Search statistics collected during tree exploration.
struct SearchStats {
    std::uint64_t nodes = 0;
    std::uint64_t qnodes = 0;
    std::uint64_t cutoffs = 0;
    std::uint64_t ttHits = 0;
    std::uint64_t ttCutoffs = 0;
    std::uint32_t maxDepth = 0;
};

/// \brief Search limits and resource constraints.
struct SearchLimits {
    std::uint32_t depth = 64;                         ///< Maximum iterative deepening depth in plies.
    std::optional<std::uint64_t> maxNodes{};          ///< Maximum node visits.
    std::optional<std::uint32_t> moveTimeMs{};        ///< Fixed time per move in milliseconds.
    std::optional<std::uint32_t> remainingTimeMs{};   ///< Clock time remaining in ms.
    std::optional<std::uint32_t> incrementMs{};       ///< Clock increment per move in ms.
    std::optional<std::uint32_t> movesToGo{};         ///< Moves to next time control.
    std::optional<TimeAllocation> timeAllocation{};   ///< Pre-computed or derived time allocation.

    constexpr SearchLimits() noexcept = default;
    constexpr explicit SearchLimits(std::uint32_t d) noexcept : depth(d) {}
};

/// \brief Result of an iterative deepening search.
struct SearchResult {
    std::optional<Move> bestMove;
    std::int32_t score = 0;                           ///< Relative to side to move at the root node.
    std::uint32_t completedDepth = 0;                 ///< Last fully completed depth iteration.
    SearchStats stats;
    bool interrupted = false;                         ///< True if search was interrupted by time or stop signal.
};

/// \brief Leaf evaluator signature.
using LeafEvaluator = std::function<std::int32_t(const Position&)>;

/// \brief Per-depth callback invoked after each completed iterative deepening iteration.
/// Called with the depth, score, nodes, and best move of the completed iteration.
/// Can be used to emit UCI 'info depth' lines during search.
/// The callback must be fast and must not call back into the engine.
using DepthCallback = std::function<void(std::uint32_t depth, std::int32_t score,
                                         std::uint64_t nodes, std::optional<Move> bestMove)>;

/// \brief Recursive quiescence search resolving tactical capture and promotion chains.
[[nodiscard]] std::int32_t quiescence(
    Position& position,
    std::int32_t alpha,
    std::int32_t beta,
    std::uint32_t ply,
    SearchStats& stats,
    const LeafEvaluator& evaluator);

/// \brief Recursive depth-limited unpruned negamax search (kept for differential testing).
[[nodiscard]] std::int32_t negamax(
    Position& position,
    std::uint32_t depth,
    std::uint32_t ply,
    SearchStats& stats,
    const LeafEvaluator& evaluator);

/// \brief Recursive depth-limited alpha-beta search with fail-soft bounds and move ordering.
[[nodiscard]] std::int32_t alphaBeta(
    Position& position,
    std::int32_t alpha,
    std::int32_t beta,
    std::uint32_t depth,
    std::uint32_t ply,
    SearchStats& stats,
    const LeafEvaluator& evaluator);

/// \brief Unpruned minimax root search (kept for shallow differential testing).
[[nodiscard]] SearchResult searchMinimax(
    Position& position,
    const SearchLimits& limits,
    const LeafEvaluator& evaluator = [](const Position& pos) {
        return eval::evaluateSideToMove(pos);
    });

/// \brief Progressive Iterative Deepening search controller with time management, stop token,
/// optional transposition table, and configurable search enhancements.
///
/// The \p options parameter controls which advanced techniques (aspiration windows, PVS,
/// null-move pruning, LMR, killer heuristic) are active. Defaults to all enabled.
/// Pass `SearchOptions::vanilla()` to obtain plain alpha-beta without enhancements.
///
/// The optional \p depthCallback is invoked after each completed iterative-deepening
/// iteration and can be used to emit UCI 'info' lines. It must not modify the position
/// or call back into the engine.
[[nodiscard]] SearchResult search(
    Position& position,
    const SearchLimits& limits,
    const LeafEvaluator& evaluator = [](const Position& pos) {
        return eval::evaluateSideToMove(pos);
    },
    SearchStopToken* stopToken = nullptr,
    tt::TranspositionTable* ttable = nullptr,
    const SearchOptions& options = SearchOptions{},
    const DepthCallback* depthCallback = nullptr);

}  // namespace search
}  // namespace rooksac
