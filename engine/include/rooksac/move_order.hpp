#pragma once

#include <cstdint>

#include "rooksac/move.hpp"
#include "rooksac/move_list.hpp"
#include "rooksac/position.hpp"

namespace rooksac {
namespace search {

/// \brief Base priorities for move ordering stages.
inline constexpr std::int32_t kPvMoveBaseScore = 40'000;
inline constexpr std::int32_t kCapturePromoBaseScore = 30'000;
inline constexpr std::int32_t kPromoBaseScore = 20'000;
inline constexpr std::int32_t kCaptureBaseScore = 10'000;
inline constexpr std::int32_t kQuietBaseScore = 0;

/// \brief Computes an integer ordering priority score for a move in the given position.
/// High score moves are searched first to maximize early alpha-beta cutoffs.
///
/// Priority staging:
/// 1. PV move from previous iteration: kPvMoveBaseScore (40,000)
/// 2. Captures with promotion: kCapturePromoBaseScore + MVV-LVA + promotion piece value
/// 3. Promotions: kPromoBaseScore + promotion piece value
/// 4. Captures: kCaptureBaseScore + MVV-LVA (victim * 10 - attacker)
/// 5. Quiet moves: kQuietBaseScore (0)
[[nodiscard]] std::int32_t scoreMove(
    const Position& position,
    Move move,
    std::optional<Move> pvMove = std::nullopt) noexcept;

/// \brief Sorts legal moves in-place according to staged priority scores.
/// Preserves the relative order of equally scored moves (stable sorting)
/// to maintain strict determinism across test runs.
void orderMoves(
    const Position& position,
    MoveList& moves,
    std::optional<Move> pvMove = std::nullopt) noexcept;

}  // namespace search
}  // namespace rooksac

