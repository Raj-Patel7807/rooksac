#include "rooksac/movegen.hpp"

#include <cassert>

#include "rooksac/attacks.hpp"
#include "rooksac/bitboard.hpp"
#include "rooksac/castling.hpp"
#include "rooksac/types.hpp"

namespace rooksac {
namespace movegen {

namespace {

void addPawnPushMoves(Square from, Square to, MoveList& moveList, bool generatesPromotions, bool generatesQuiets) noexcept {
    const auto r = rankOf(to);
    if (r == Rank::Eight || r == Rank::One) {
        if (generatesPromotions) {
            moveList.pushBack(*Move::create(from, to, PieceType::Queen));
            moveList.pushBack(*Move::create(from, to, PieceType::Rook));
            moveList.pushBack(*Move::create(from, to, PieceType::Bishop));
            moveList.pushBack(*Move::create(from, to, PieceType::Knight));
        }
    } else if (generatesQuiets) {
        moveList.pushBack(*Move::create(from, to, PieceType::None));
    }
}

void addPawnCaptureMoves(Square from, Square to, MoveList& moveList, bool generatesCaptures) noexcept {
    const auto r = rankOf(to);
    if (r == Rank::Eight || r == Rank::One) {
        if (generatesCaptures) {
            moveList.pushBack(*Move::create(from, to, PieceType::Queen));
            moveList.pushBack(*Move::create(from, to, PieceType::Rook));
            moveList.pushBack(*Move::create(from, to, PieceType::Bishop));
            moveList.pushBack(*Move::create(from, to, PieceType::Knight));
        }
    } else if (generatesCaptures) {
        moveList.pushBack(*Move::create(from, to, PieceType::None));
    }
}

void generatePawnMoves(const Position& position, Color us, MoveList& moveList, bool capturesOnly, bool quietsOnly) noexcept {
    const Bitboard ourPawns = position.pieces(us, PieceType::Pawn);
    const Bitboard totalOcc = position.totalOccupancy();
    const Bitboard enemyOcc = position.colorOccupancy(opposite(us));
    const Square epSquare = position.enPassantSquare();

    const bool genCaptures = !quietsOnly;
    const bool genQuiets = !capturesOnly;

    if (us == Color::White) {
        for (const Square from : BitboardSquares(ourPawns)) {
            const auto fromIdx = squareIndex(from);
            const auto singleStepSq = static_cast<Square>(fromIdx + 8U);

            // Single and double pushes
            if (!testBit(totalOcc, singleStepSq)) {
                // Promotions are included in capturesOnly; non-promotion pushes in quietsOnly
                addPawnPushMoves(from, singleStepSq, moveList, /*generatesPromotions=*/!quietsOnly, /*generatesQuiets=*/!capturesOnly);

                // Double push from rank 2
                if (rankOf(from) == Rank::Two && genQuiets) {
                    const auto doubleStepSq = static_cast<Square>(fromIdx + 16U);
                    if (!testBit(totalOcc, doubleStepSq)) {
                        moveList.pushBack(*Move::create(from, doubleStepSq, PieceType::None));
                    }
                }
            }

            // Captures (diagonal)
            const Bitboard attackTargets = attacks::pawnAttacks(from, Color::White);
            const Bitboard standardCaptures = attackTargets & enemyOcc;
            for (const Square to : BitboardSquares(standardCaptures)) {
                addPawnCaptureMoves(from, to, moveList, genCaptures);
            }

            // En passant capture
            if (epSquare != Square::None && genCaptures) {
                if (testBit(attackTargets, epSquare)) {
                    moveList.pushBack(*Move::create(from, epSquare, PieceType::None));
                }
            }
        }
    } else {
        // Black pawns
        for (const Square from : BitboardSquares(ourPawns)) {
            const auto fromIdx = squareIndex(from);
            const auto singleStepSq = static_cast<Square>(fromIdx - 8U);

            // Single and double pushes
            if (!testBit(totalOcc, singleStepSq)) {
                addPawnPushMoves(from, singleStepSq, moveList, /*generatesPromotions=*/!quietsOnly, /*generatesQuiets=*/!capturesOnly);

                // Double push from rank 7
                if (rankOf(from) == Rank::Seven && genQuiets) {
                    const auto doubleStepSq = static_cast<Square>(fromIdx - 16U);
                    if (!testBit(totalOcc, doubleStepSq)) {
                        moveList.pushBack(*Move::create(from, doubleStepSq, PieceType::None));
                    }
                }
            }

            // Captures (diagonal)
            const Bitboard attackTargets = attacks::pawnAttacks(from, Color::Black);
            const Bitboard standardCaptures = attackTargets & enemyOcc;
            for (const Square to : BitboardSquares(standardCaptures)) {
                addPawnCaptureMoves(from, to, moveList, genCaptures);
            }

            // En passant capture
            if (epSquare != Square::None && genCaptures) {
                if (testBit(attackTargets, epSquare)) {
                    moveList.pushBack(*Move::create(from, epSquare, PieceType::None));
                }
            }
        }
    }
}

void generatePieceMoves(
    Square from,
    Bitboard targets,
    const Position& position,
    Color us,
    MoveList& moveList,
    bool capturesOnly,
    bool quietsOnly) noexcept {
    const Bitboard enemyOcc = position.colorOccupancy(opposite(us));
    const Bitboard ourOcc = position.colorOccupancy(us);

    // Filter out our own pieces
    targets &= ~ourOcc;

    if (capturesOnly) {
        targets &= enemyOcc;
    } else if (quietsOnly) {
        targets &= ~enemyOcc;
    }

    for (const Square to : BitboardSquares(targets)) {
        moveList.pushBack(*Move::create(from, to, PieceType::None));
    }
}

void generateCastlingMoves(const Position& position, Color us, MoveList& moveList) noexcept {
    const CastlingRights rights = position.castlingRights();
    const Bitboard totalOcc = position.totalOccupancy();

    if (us == Color::White) {
        if (hasCastlingRight(rights, CastlingRights::WhiteKingside)) {
            // Path between e1 and h1 (f1, g1) must be empty
            if (!testBit(totalOcc, Square::F1) && !testBit(totalOcc, Square::G1)) {
                moveList.pushBack(*Move::create(Square::E1, Square::G1, PieceType::None));
            }
        }
        if (hasCastlingRight(rights, CastlingRights::WhiteQueenside)) {
            // Path between e1 and a1 (b1, c1, d1) must be empty
            if (!testBit(totalOcc, Square::B1) && !testBit(totalOcc, Square::C1) && !testBit(totalOcc, Square::D1)) {
                moveList.pushBack(*Move::create(Square::E1, Square::C1, PieceType::None));
            }
        }
    } else {
        if (hasCastlingRight(rights, CastlingRights::BlackKingside)) {
            // Path between e8 and h8 (f8, g8) must be empty
            if (!testBit(totalOcc, Square::F8) && !testBit(totalOcc, Square::G8)) {
                moveList.pushBack(*Move::create(Square::E8, Square::G8, PieceType::None));
            }
        }
        if (hasCastlingRight(rights, CastlingRights::BlackQueenside)) {
            // Path between e8 and a8 (b8, c8, d8) must be empty
            if (!testBit(totalOcc, Square::B8) && !testBit(totalOcc, Square::C8) && !testBit(totalOcc, Square::D8)) {
                moveList.pushBack(*Move::create(Square::E8, Square::C8, PieceType::None));
            }
        }
    }
}

void generateMovesInternal(const Position& position, MoveList& moveList, bool capturesOnly, bool quietsOnly) noexcept {
    const Color us = position.sideToMove();
    const Bitboard totalOcc = position.totalOccupancy();

    // 1. Pawns
    generatePawnMoves(position, us, moveList, capturesOnly, quietsOnly);

    // 2. Knights
    for (const Square sq : BitboardSquares(position.pieces(us, PieceType::Knight))) {
        const Bitboard att = attacks::knightAttacks(sq);
        generatePieceMoves(sq, att, position, us, moveList, capturesOnly, quietsOnly);
    }

    // 3. Bishops
    for (const Square sq : BitboardSquares(position.pieces(us, PieceType::Bishop))) {
        const Bitboard att = attacks::bishopAttacks(sq, totalOcc);
        generatePieceMoves(sq, att, position, us, moveList, capturesOnly, quietsOnly);
    }

    // 4. Rooks
    for (const Square sq : BitboardSquares(position.pieces(us, PieceType::Rook))) {
        const Bitboard att = attacks::rookAttacks(sq, totalOcc);
        generatePieceMoves(sq, att, position, us, moveList, capturesOnly, quietsOnly);
    }

    // 5. Queens
    for (const Square sq : BitboardSquares(position.pieces(us, PieceType::Queen))) {
        const Bitboard att = attacks::queenAttacks(sq, totalOcc);
        generatePieceMoves(sq, att, position, us, moveList, capturesOnly, quietsOnly);
    }

    // 6. King
    for (const Square sq : BitboardSquares(position.pieces(us, PieceType::King))) {
        const Bitboard att = attacks::kingAttacks(sq);
        generatePieceMoves(sq, att, position, us, moveList, capturesOnly, quietsOnly);
    }

    // 7. Castling (classified as quiet moves)
    if (!capturesOnly) {
        generateCastlingMoves(position, us, moveList);
    }
}

}  // namespace

void generatePseudoLegalMoves(const Position& position, MoveList& moveList) noexcept {
    generateMovesInternal(position, moveList, /*capturesOnly=*/false, /*quietsOnly=*/false);
}

void generatePseudoLegalCaptures(const Position& position, MoveList& moveList) noexcept {
    generateMovesInternal(position, moveList, /*capturesOnly=*/true, /*quietsOnly=*/false);
}

void generatePseudoLegalQuiets(const Position& position, MoveList& moveList) noexcept {
    generateMovesInternal(position, moveList, /*capturesOnly=*/false, /*quietsOnly=*/true);
}

}  // namespace movegen
}  // namespace rooksac
