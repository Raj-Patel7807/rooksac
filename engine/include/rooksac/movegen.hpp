#pragma once

#include "rooksac/move_list.hpp"
#include "rooksac/position.hpp"

namespace rooksac {

/// \brief Pseudo-legal move generator for RookSac.
///
/// Pseudo-legal moves follow piece movement rules, board occupancy, captures,
/// double pawn pushes, en-passant, promotions, and castling rights (empty path),
/// but do NOT filter moves that leave the king in check.
/// Full legal move filtering belongs to Sprint 07.
namespace movegen {

/// \brief Generates all pseudo-legal moves for the side to move in the given position.
/// \param position The position state.
/// \param moveList Destination move list to append moves to.
void generatePseudoLegalMoves(const Position& position, MoveList& moveList) noexcept;

/// \brief Generates only pseudo-legal captures and promotions (useful for quiescence search).
void generatePseudoLegalCaptures(const Position& position, MoveList& moveList) noexcept;

/// \brief Generates only pseudo-legal quiet moves (non-captures, non-promotions).
void generatePseudoLegalQuiets(const Position& position, MoveList& moveList) noexcept;

}  // namespace movegen
}  // namespace rooksac
