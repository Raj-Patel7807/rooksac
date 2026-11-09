#pragma once

#include <array>
#include <cstdint>
#include <optional>

#include "rooksac/move.hpp"
#include "rooksac/move_list.hpp"
#include "rooksac/position.hpp"
#include "rooksac/search_options.hpp"

namespace rooksac {
namespace search {

/// \brief Base priorities for move ordering stages.
inline constexpr std::int32_t kPvMoveBaseScore = 50'000;
inline constexpr std::int32_t kCapturePromoBaseScore = 40'000;
inline constexpr std::int32_t kPromoBaseScore = 30'000;
inline constexpr std::int32_t kGoodCaptureBaseScore = 20'000;
inline constexpr std::int32_t kCaptureBaseScore = kGoodCaptureBaseScore;
inline constexpr std::int32_t kKillerBaseScore = 10'000;
inline constexpr std::int32_t kQuietBaseScore = 0;
inline constexpr std::int32_t kBadCaptureBaseScore = -10'000;

/// \brief Per-ply killer move storage. Two quiet moves that caused a beta cutoff
/// are kept per ply so they can be tried early in sibling nodes.
using KillerTable = std::array<std::array<Move, SearchOptions::kKillerSlots>, SearchOptions::kMaxKillerPly>;

/// \brief History heuristic table indexed by [piece index (0..11)][to-square (0..63)].
/// Updated when a quiet move causes a beta cutoff; scores are used to order quiet
/// moves below killers, improving alpha-beta pruning efficiency over iterations.
using HistoryTable = std::array<std::array<std::int32_t, 64>, 12>;

/// \brief Returns the piece index for a history table lookup (0..11).
/// Mapping: White Pawn=0, Knight=1, Bishop=2, Rook=3, Queen=4, King=5;
/// Black Pawn=6, ..., Black King=11.
[[nodiscard]] constexpr std::int32_t historyPieceIndex(Piece piece) noexcept {
    const auto color = colorOf(piece);
    const auto type  = typeOf(piece);
    // PieceType enum: Pawn=1, Knight=2, Bishop=3, Rook=4, Queen=5, King=6
    const auto typeIdx = static_cast<std::int32_t>(type) - 1;
    return typeIdx + (color == Color::Black ? 6 : 0);
}

/// \brief Maximum absolute value stored in the history table to prevent score overflow.
inline constexpr std::int32_t kHistoryMax = 8'000;

/// \brief Computes an integer ordering priority score for a move in the given position.
/// High score moves are searched first to maximize early alpha-beta cutoffs.
///
/// Priority staging:
/// 1. PV move from previous iteration: kPvMoveBaseScore (40,000)
/// 2. Captures with promotion: kCapturePromoBaseScore + MVV-LVA + promotion piece value
/// 3. Promotions: kPromoBaseScore + promotion piece value
/// 4. Captures: kCaptureBaseScore + MVV-LVA (victim * 10 - attacker)
/// 5. Killer moves: kKillerBaseScore (quiet moves that previously caused cutoffs)
/// 6. History heuristic: kQuietBaseScore + history bonus (historically good quiet moves)
[[nodiscard]] std::int32_t scoreMove(
    const Position& position,
    Move move,
    std::optional<Move> pvMove = std::nullopt,
    const KillerTable* killers = nullptr,
    std::uint32_t ply = 0,
    const HistoryTable* history = nullptr) noexcept;

/// \brief Sorts legal moves in-place according to staged priority scores.
/// Preserves the relative order of equally scored moves (stable sorting)
/// to maintain strict determinism across test runs.
void orderMoves(
    const Position& position,
    MoveList& moves,
    std::optional<Move> pvMove = std::nullopt,
    const KillerTable* killers = nullptr,
    std::uint32_t ply = 0,
    const HistoryTable* history = nullptr) noexcept;

/// \brief Records a quiet move that caused a beta cutoff in the killer table.
/// Shifts existing killers down, inserting the new move in slot 0.
void storeKillerMove(KillerTable& killers, std::uint32_t ply, Move move) noexcept;

/// \brief Updates the history table for a quiet move that caused a beta cutoff.
/// Uses depth^2 gravity: score += depth*depth, clamped to kHistoryMax.
void updateHistory(HistoryTable& history, const Position& position, Move move, std::uint32_t depth) noexcept;

/// \brief Zero-initialises a history table.
void clearHistory(HistoryTable& history) noexcept;

}  // namespace search
}  // namespace rooksac
