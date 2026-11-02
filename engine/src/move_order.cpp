#include "rooksac/move_order.hpp"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "rooksac/material.hpp"

namespace rooksac {
namespace search {

namespace {

struct ScoredMove {
    Move move;
    std::int32_t score = 0;
};

}  // namespace

std::int32_t scoreMove(
    const Position& position,
    Move move,
    std::optional<Move> pvMove) noexcept {
    // 1. PV move from previous iteration receives absolute highest priority
    if (pvMove.has_value() && move.encoded() == pvMove->encoded()) {
        return kPvMoveBaseScore;
    }

    const Square fromSq = move.from();
    const Square toSq = move.to();
    const Piece movedPiece = position.pieceAt(fromSq);
    const Piece targetPiece = position.pieceAt(toSq);
    const PieceType promo = move.promotion();

    bool isCapture = (targetPiece != Piece::None);
    PieceType victimType = PieceType::None;

    if (isCapture) {
        victimType = typeOf(targetPiece);
    } else if (typeOf(movedPiece) == PieceType::Pawn && isValidSquare(position.enPassantSquare()) &&
               toSq == position.enPassantSquare()) {
        isCapture = true;
        victimType = PieceType::Pawn;
    }

    const bool isPromo = (promo != PieceType::None);

    if (isCapture && isPromo) {
        const std::int32_t victimVal = eval::pieceTypeValue(victimType);
        const std::int32_t attackerVal = eval::pieceTypeValue(typeOf(movedPiece));
        const std::int32_t promoVal = eval::pieceTypeValue(promo);
        return kCapturePromoBaseScore + (victimVal * 10 - attackerVal) + promoVal;
    }

    if (isPromo) {
        return kPromoBaseScore + eval::pieceTypeValue(promo);
    }

    if (isCapture) {
        const std::int32_t victimVal = eval::pieceTypeValue(victimType);
        const std::int32_t attackerVal = eval::pieceTypeValue(typeOf(movedPiece));
        return kCaptureBaseScore + (victimVal * 10 - attackerVal);
    }

    return kQuietBaseScore;
}

void orderMoves(
    const Position& position,
    MoveList& moves,
    std::optional<Move> pvMove) noexcept {
    const std::size_t count = moves.size();
    if (count <= 1) {
        return;
    }

    std::array<ScoredMove, MoveList::kMaxCapacity> scoredMoves{};
    for (std::size_t i = 0; i < count; ++i) {
        scoredMoves[i].move = moves[i];
        scoredMoves[i].score = scoreMove(position, moves[i], pvMove);
    }

    // Stable sort ensures moves with the same score maintain their deterministic generation order.
    std::stable_sort(
        scoredMoves.begin(),
        scoredMoves.begin() + static_cast<std::ptrdiff_t>(count),
        [](const ScoredMove& lhs, const ScoredMove& rhs) noexcept {
            return lhs.score > rhs.score;
        });

    for (std::size_t i = 0; i < count; ++i) {
        moves[i] = scoredMoves[i].move;
    }
}

}  // namespace search
}  // namespace rooksac
