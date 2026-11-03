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


bool hasInsufficientMaterial(const Position& position) noexcept {
    // If there are any pawns, rooks, or queens on the board, checkmate is always potentially possible
    if (position.pieces(Color::White, PieceType::Pawn) != 0 ||
        position.pieces(Color::Black, PieceType::Pawn) != 0 ||
        position.pieces(Color::White, PieceType::Rook) != 0 ||
        position.pieces(Color::Black, PieceType::Rook) != 0 ||
        position.pieces(Color::White, PieceType::Queen) != 0 ||
        position.pieces(Color::Black, PieceType::Queen) != 0) {
        return false;
    }

    const auto wKnights = popCount(position.pieces(Color::White, PieceType::Knight));
    const auto bKnights = popCount(position.pieces(Color::Black, PieceType::Knight));
    const auto wBishops = popCount(position.pieces(Color::White, PieceType::Bishop));
    const auto bBishops = popCount(position.pieces(Color::Black, PieceType::Bishop));

    const auto wMinors = wKnights + wBishops;
    const auto bMinors = bKnights + bBishops;

    // 1. King vs King
    if (wMinors == 0 && bMinors == 0) {
        return true;
    }

    // 2. King + Minor vs King (K+N vs K, K+B vs K)
    if ((wMinors == 1 && bMinors == 0) || (wMinors == 0 && bMinors == 1)) {
        return true;
    }

    // 3. King + Bishop vs King + Bishop where both bishops are on the same square color
    if (wBishops == 1 && bBishops == 1 && wKnights == 0 && bKnights == 0) {
        const Bitboard wBishopBB = position.pieces(Color::White, PieceType::Bishop);
        const Bitboard bBishopBB = position.pieces(Color::Black, PieceType::Bishop);
        const bool wOnDark = (wBishopBB & kDarkSquares) != 0;
        const bool bOnDark = (bBishopBB & kDarkSquares) != 0;
        if (wOnDark == bOnDark) {
            return true;
        }
    }

    return false;
}

}  // namespace eval
}  // namespace rooksac

