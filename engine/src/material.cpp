#include "rooksac/material.hpp"

#include "rooksac/bitboard.hpp"

namespace rooksac {
namespace eval {

std::int32_t evaluateMaterial(const Position& position, const MaterialWeights& weights) noexcept {
    const std::int32_t whiteMaterial =
        popCount(position.pieces(Color::White, PieceType::Pawn)) * weights.pawn +
        popCount(position.pieces(Color::White, PieceType::Knight)) * weights.knight +
        popCount(position.pieces(Color::White, PieceType::Bishop)) * weights.bishop +
        popCount(position.pieces(Color::White, PieceType::Rook)) * weights.rook +
        popCount(position.pieces(Color::White, PieceType::Queen)) * weights.queen;

    const std::int32_t blackMaterial =
        popCount(position.pieces(Color::Black, PieceType::Pawn)) * weights.pawn +
        popCount(position.pieces(Color::Black, PieceType::Knight)) * weights.knight +
        popCount(position.pieces(Color::Black, PieceType::Bishop)) * weights.bishop +
        popCount(position.pieces(Color::Black, PieceType::Rook)) * weights.rook +
        popCount(position.pieces(Color::Black, PieceType::Queen)) * weights.queen;

    return whiteMaterial - blackMaterial;
}

std::int32_t evaluateMaterialSideToMove(const Position& position, const MaterialWeights& weights) noexcept {
    const std::int32_t whitePerspective = evaluateMaterial(position, weights);
    return position.sideToMove() == Color::White ? whitePerspective : -whitePerspective;
}

}  // namespace eval
}  // namespace rooksac
