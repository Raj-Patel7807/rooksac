#pragma once

#include <cstdint>

#include "rooksac/attacks.hpp"
#include "rooksac/material.hpp"
#include "rooksac/move.hpp"
#include "rooksac/position.hpp"

namespace rooksac {
namespace search {

/// \brief Static Exchange Evaluation (SEE) evaluates the net material balance
/// of a series of captures on a target square, assuming both sides play their
/// least valuable attackers first (minimax swap algorithm).
///
/// Handles:
/// - Direct sliding and non-sliding attacks (Pawns, Knights, Bishops, Rooks, Queens, Kings)
/// - Hidden X-ray attacks (revealed when a piece captures and vacates its ray)
/// - Promotions (evaluated as gaining the promoted piece and losing the pawn)
/// - En passant captures (special pawn removal on adjacent rank)
///
/// \param position The current board position.
/// \param move The candidate capture or move to evaluate.
/// \return Net material balance gained (positive) or lost (negative) in centipawns.
[[nodiscard]] std::int32_t see(const Position& position, Move move) noexcept;

/// \brief Returns true if SEE(position, move) >= threshold.
/// Frequently used with threshold = 0 to prune losing captures or order winning captures.
[[nodiscard]] inline bool seeGe(const Position& position, Move move, std::int32_t threshold = 0) noexcept {
    return see(position, move) >= threshold;
}

}  // namespace search
}  // namespace rooksac

