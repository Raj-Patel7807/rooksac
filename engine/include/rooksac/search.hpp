#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include "rooksac/material.hpp"
#include "rooksac/move.hpp"
#include "rooksac/move_list.hpp"
#include "rooksac/position.hpp"

namespace rooksac {
namespace search {

/// \brief Score constant representing practical infinity for minimax bounds.
inline constexpr std::int32_t kInfinity = 1'000'000;

/// \brief Base mate score for checkmate distance evaluations.
/// Checkmate scores are represented as (kMateScore - ply) for winning positions
/// and (-kMateScore + ply) for losing positions, prioritizing faster checkmates.
inline constexpr std::int32_t kMateScore = 30'000;

/// \brief Maximum search ply depth supported for mate-distance adjustments.
inline constexpr std::int32_t kMaxMatePly = 1'000;

/// \brief Score for terminal draws (stalemate, fifty-move rule).
inline constexpr std::int32_t kDrawScore = 0;

/// \brief Checks if a score indicates a checkmate sequence within known bounds.
[[nodiscard]] constexpr bool isMateScore(std::int32_t score) noexcept {
    return (score >= kMateScore - kMaxMatePly && score <= kMateScore) ||
           (score <= -kMateScore + kMaxMatePly && score >= -kMateScore);
}

/// \brief Search statistics collected during tree exploration.
struct SearchStats {
    std::uint64_t nodes = 0;
    std::uint64_t cutoffs = 0;
    std::uint32_t maxDepth = 0;
};

/// \brief Search limits and configuration.
struct SearchLimits {
    std::uint32_t depth = 1;  ///< Depth in plies (half-moves).
};

/// \brief Result of a depth-limited game-tree search.
struct SearchResult {
    std::optional<Move> bestMove;
    std::int32_t score = 0;  ///< Relative to side to move at the root node.
    SearchStats stats;
};

/// \brief Leaf evaluator signature (returns score in centipawns from side to move perspective).
using LeafEvaluator = std::function<std::int32_t(const Position&)>;

/// \brief Recursive depth-limited unpruned negamax search (kept for differential testing).
/// \param position The current position (restored to original state upon return).
/// \param depth Remaining search depth in plies.
/// \param ply Current depth from the root in plies (used for mate-distance scoring).
/// \param stats Search statistics to accumulate.
/// \param evaluator Static evaluation function for leaf nodes.
/// \return Score from the perspective of the side to move at this node.
[[nodiscard]] std::int32_t negamax(
    Position& position,
    std::uint32_t depth,
    std::uint32_t ply,
    SearchStats& stats,
    const LeafEvaluator& evaluator);

/// \brief Recursive depth-limited alpha-beta search with fail-soft bounds and move ordering.
/// \param position The current position (restored to original state upon return).
/// \param alpha Lower score bound from the current player's perspective.
/// \param beta Upper score bound from the current player's perspective.
/// \param depth Remaining search depth in plies.
/// \param ply Current depth from the root in plies (used for mate-distance scoring).
/// \param stats Search statistics to accumulate (nodes, cutoffs, max depth).
/// \param evaluator Static evaluation function for leaf nodes.
/// \return Score from the perspective of the side to move at this node.
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
        return eval::evaluateMaterialSideToMove(pos);
    });

/// \brief Root search function finding the best legal move and score using Alpha-Beta pruning.
/// \param position The root position (restored to original state upon return).
/// \param limits Search constraints (depth).
/// \param evaluator Leaf evaluator (defaults to evaluateMaterialSideToMove).
/// \return SearchResult containing best move, score, and statistics.
[[nodiscard]] SearchResult search(
    Position& position,
    const SearchLimits& limits,
    const LeafEvaluator& evaluator = [](const Position& pos) {
        return eval::evaluateMaterialSideToMove(pos);
    });

}  // namespace search
}  // namespace rooksac
