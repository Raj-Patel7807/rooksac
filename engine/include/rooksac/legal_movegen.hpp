#pragma once

#include <cstdint>

#include "rooksac/move.hpp"
#include "rooksac/move_list.hpp"
#include "rooksac/position.hpp"

namespace rooksac {

enum class GameResult {
    Ongoing,
    Checkmate,
    Stalemate,
    FiftyMoveDraw,
    InsufficientMaterialDraw,
};

namespace movegen {

/// \brief Returns whether the side to move is currently in check.
[[nodiscard]] bool isInCheck(const Position& position) noexcept;

/// \brief Returns whether the given color's king is currently attacked.
[[nodiscard]] bool isKingInCheck(const Position& position, Color color) noexcept;

/// \brief Tests whether a pseudo-legal move is fully legal (leaves king safe, castling not through check).
[[nodiscard]] bool isLegalMove(Position& position, Move move) noexcept;

/// \brief Generates all fully legal moves for the side to move.
void generateLegalMoves(Position& position, MoveList& legalMoves) noexcept;

/// \brief Generates only legal captures and promotions.
void generateLegalCaptures(Position& position, MoveList& legalCaptures) noexcept;

/// \brief Evaluates the game state: Ongoing, Checkmate, Stalemate, or FiftyMoveDraw.
[[nodiscard]] GameResult evaluateGameState(Position& position) noexcept;

}  // namespace movegen
}  // namespace rooksac
