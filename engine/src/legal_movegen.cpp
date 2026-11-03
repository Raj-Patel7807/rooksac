#include "rooksac/legal_movegen.hpp"

#include "rooksac/material.hpp"

#include "rooksac/attacks.hpp"
#include "rooksac/movegen.hpp"
#include "rooksac/undo_state.hpp"

namespace rooksac {
namespace movegen {

bool isKingInCheck(const Position& position, Color color) noexcept {
    const Bitboard kingBb = position.pieces(color, PieceType::King);
    if (kingBb == kEmptyBitboard) {
        return false;
    }
    const Square kingSq = leastSignificantSquare(kingBb);
    const Color enemy = opposite(color);

    return attacks::isSquareAttacked(
        kingSq,
        enemy,
        position.totalOccupancy(),
        position.pieces(enemy, PieceType::Pawn),
        position.pieces(enemy, PieceType::Knight),
        position.pieces(enemy, PieceType::Bishop),
        position.pieces(enemy, PieceType::Rook),
        position.pieces(enemy, PieceType::Queen),
        position.pieces(enemy, PieceType::King));
}

bool isInCheck(const Position& position) noexcept {
    return isKingInCheck(position, position.sideToMove());
}

bool isLegalMove(Position& position, Move move) noexcept {
    const Color us = position.sideToMove();
    const Color enemy = opposite(us);
    const Square from = move.from();
    const Square to = move.to();
    const Piece movingPiece = position.pieceAt(from);
    const PieceType movingType = typeOf(movingPiece);

    // Castling special check:
    // 1. King must NOT be in check currently.
    // 2. Transit square and target square must NOT be attacked.
    if (movingType == PieceType::King) {
        if (from == Square::E1 && to == Square::G1) {
            // White O-O: e1, f1, g1 must not be attacked
            if (isInCheck(position)) return false;
            const auto occ = position.totalOccupancy();
            const auto p = position.pieces(enemy, PieceType::Pawn);
            const auto n = position.pieces(enemy, PieceType::Knight);
            const auto b = position.pieces(enemy, PieceType::Bishop);
            const auto r = position.pieces(enemy, PieceType::Rook);
            const auto q = position.pieces(enemy, PieceType::Queen);
            const auto k = position.pieces(enemy, PieceType::King);
            if (attacks::isSquareAttacked(Square::F1, enemy, occ, p, n, b, r, q, k)) return false;
            if (attacks::isSquareAttacked(Square::G1, enemy, occ, p, n, b, r, q, k)) return false;
        } else if (from == Square::E1 && to == Square::C1) {
            // White O-O-O: e1, d1, c1 must not be attacked
            if (isInCheck(position)) return false;
            const auto occ = position.totalOccupancy();
            const auto p = position.pieces(enemy, PieceType::Pawn);
            const auto n = position.pieces(enemy, PieceType::Knight);
            const auto b = position.pieces(enemy, PieceType::Bishop);
            const auto r = position.pieces(enemy, PieceType::Rook);
            const auto q = position.pieces(enemy, PieceType::Queen);
            const auto k = position.pieces(enemy, PieceType::King);
            if (attacks::isSquareAttacked(Square::D1, enemy, occ, p, n, b, r, q, k)) return false;
            if (attacks::isSquareAttacked(Square::C1, enemy, occ, p, n, b, r, q, k)) return false;
        } else if (from == Square::E8 && to == Square::G8) {
            // Black O-O: e8, f8, g8 must not be attacked
            if (isInCheck(position)) return false;
            const auto occ = position.totalOccupancy();
            const auto p = position.pieces(enemy, PieceType::Pawn);
            const auto n = position.pieces(enemy, PieceType::Knight);
            const auto b = position.pieces(enemy, PieceType::Bishop);
            const auto r = position.pieces(enemy, PieceType::Rook);
            const auto q = position.pieces(enemy, PieceType::Queen);
            const auto k = position.pieces(enemy, PieceType::King);
            if (attacks::isSquareAttacked(Square::F8, enemy, occ, p, n, b, r, q, k)) return false;
            if (attacks::isSquareAttacked(Square::G8, enemy, occ, p, n, b, r, q, k)) return false;
        } else if (from == Square::E8 && to == Square::C8) {
            // Black O-O-O: e8, d8, c8 must not be attacked
            if (isInCheck(position)) return false;
            const auto occ = position.totalOccupancy();
            const auto p = position.pieces(enemy, PieceType::Pawn);
            const auto n = position.pieces(enemy, PieceType::Knight);
            const auto b = position.pieces(enemy, PieceType::Bishop);
            const auto r = position.pieces(enemy, PieceType::Rook);
            const auto q = position.pieces(enemy, PieceType::Queen);
            const auto k = position.pieces(enemy, PieceType::King);
            if (attacks::isSquareAttacked(Square::D8, enemy, occ, p, n, b, r, q, k)) return false;
            if (attacks::isSquareAttacked(Square::C8, enemy, occ, p, n, b, r, q, k)) return false;
        }
    }

    // Make move, check king safety for 'us', then unmake
    UndoState undo;
    if (!position.makeMove(move, undo)) {
        return false;
    }

    const bool kingSafe = !isKingInCheck(position, us);
    position.unmakeMove(move, undo);

    return kingSafe;
}

void generateLegalMoves(Position& position, MoveList& legalMoves) noexcept {
    legalMoves.clear();
    MoveList pseudoMoves;
    generatePseudoLegalMoves(position, pseudoMoves);

    for (const Move move : pseudoMoves) {
        if (isLegalMove(position, move)) {
            legalMoves.pushBack(move);
        }
    }
}

void generateLegalCaptures(Position& position, MoveList& legalCaptures) noexcept {
    legalCaptures.clear();
    MoveList pseudoCaptures;
    generatePseudoLegalCaptures(position, pseudoCaptures);

    for (const Move move : pseudoCaptures) {
        if (isLegalMove(position, move)) {
            legalCaptures.pushBack(move);
        }
    }
}

GameResult evaluateGameState(Position& position) noexcept {
    MoveList legalMoves;
    generateLegalMoves(position, legalMoves);

    if (legalMoves.empty()) {
        if (isInCheck(position)) {
            return GameResult::Checkmate;
        }
        return GameResult::Stalemate;
    }

    if (position.halfmoveClock() >= 100) {
        return GameResult::FiftyMoveDraw;
    }

    if (eval::hasInsufficientMaterial(position)) {
        return GameResult::InsufficientMaterialDraw;
    }

    return GameResult::Ongoing;
}

}  // namespace movegen
}  // namespace rooksac
