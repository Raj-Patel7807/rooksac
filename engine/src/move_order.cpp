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
    std::optional<Move> pvMove,
    const KillerTable* killers,
    std::uint32_t ply,
    const HistoryTable* history) noexcept {
    // 1. PV move from previous iteration receives absolute highest priority
    if (pvMove.has_value() && move.encoded() == pvMove->encoded()) {
        return kPvMoveBaseScore;
    }

    const Square fromSq = move.from();
    const Square toSq   = move.to();
    const Piece movedPiece  = position.pieceAt(fromSq);
    const Piece targetPiece = position.pieceAt(toSq);
    const PieceType promo   = move.promotion();

    bool isCapture     = (targetPiece != Piece::None);
    PieceType victimType = PieceType::None;

    if (isCapture) {
        victimType = typeOf(targetPiece);
    } else if (typeOf(movedPiece) == PieceType::Pawn && isValidSquare(position.enPassantSquare()) &&
               toSq == position.enPassantSquare()) {
        isCapture  = true;
        victimType = PieceType::Pawn;
    }

    const bool isPromo = (promo != PieceType::None);

    if (isCapture && isPromo) {
        const std::int32_t victimVal   = eval::pieceTypeValue(victimType);
        const std::int32_t attackerVal = eval::pieceTypeValue(typeOf(movedPiece));
        const std::int32_t promoVal    = eval::pieceTypeValue(promo);
        return kCapturePromoBaseScore + (victimVal * 10 - attackerVal) + promoVal;
    }

    if (isPromo) {
        return kPromoBaseScore + eval::pieceTypeValue(promo);
    }

    if (isCapture) {
        const std::int32_t victimVal   = eval::pieceTypeValue(victimType);
        const std::int32_t attackerVal = eval::pieceTypeValue(typeOf(movedPiece));
        return kCaptureBaseScore + (victimVal * 10 - attackerVal);
    }

    // 5. Killer move heuristic: quiet moves that previously caused a beta cutoff at this ply
    if (killers != nullptr && ply < SearchOptions::kMaxKillerPly) {
        const auto& plyKillers = (*killers)[ply];
        for (std::uint32_t k = 0; k < SearchOptions::kKillerSlots; ++k) {
            if (plyKillers[k].encoded() == move.encoded()) {
                // First killer slot scores higher than second
                return kKillerBaseScore - static_cast<std::int32_t>(k);
            }
        }
    }

    // 6. History heuristic: bonus for quiet moves that historically caused cutoffs
    if (history != nullptr && movedPiece != Piece::None) {
        const std::int32_t pieceIdx = historyPieceIndex(movedPiece);
        const std::int32_t sqIdx    = static_cast<std::int32_t>(squareIndex(toSq));
        if (pieceIdx >= 0 && pieceIdx < 12 && sqIdx >= 0 && sqIdx < 64) {
            return kQuietBaseScore + (*history)[static_cast<std::size_t>(pieceIdx)][static_cast<std::size_t>(sqIdx)];
        }
    }

    return kQuietBaseScore;
}

void orderMoves(
    const Position& position,
    MoveList& moves,
    std::optional<Move> pvMove,
    const KillerTable* killers,
    std::uint32_t ply,
    const HistoryTable* history) noexcept {
    const std::size_t count = moves.size();
    if (count <= 1) {
        return;
    }

    std::array<ScoredMove, MoveList::kMaxCapacity> scoredMoves{};
    for (std::size_t i = 0; i < count; ++i) {
        scoredMoves[i].move  = moves[i];
        scoredMoves[i].score = scoreMove(position, moves[i], pvMove, killers, ply, history);
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

void storeKillerMove(KillerTable& killers, std::uint32_t ply, Move move) noexcept {
    if (ply >= SearchOptions::kMaxKillerPly) {
        return;
    }

    auto& plyKillers = killers[ply];

    // Avoid storing duplicate (already in slot 0)
    if (plyKillers[0].encoded() == move.encoded()) {
        return;
    }

    // Shift existing killers down: slot 1 <- slot 0, slot 0 <- new move
    plyKillers[1] = plyKillers[0];
    plyKillers[0] = move;
}

void updateHistory(
    HistoryTable& history,
    const Position& position,
    Move move,
    std::uint32_t depth) noexcept {
    const Piece movedPiece = position.pieceAt(move.from());
    if (movedPiece == Piece::None) {
        return;
    }
    const std::int32_t pieceIdx = historyPieceIndex(movedPiece);
    const std::int32_t sqIdx    = static_cast<std::int32_t>(squareIndex(move.to()));
    if (pieceIdx < 0 || pieceIdx >= 12 || sqIdx < 0 || sqIdx >= 64) {
        return;
    }

    // Gravity bonus: depth^2 so deeper cutoffs receive more weight
    const std::int32_t bonus = static_cast<std::int32_t>(depth * depth);
    auto& entry = history[static_cast<std::size_t>(pieceIdx)][static_cast<std::size_t>(sqIdx)];
    entry += bonus;
    if (entry > kHistoryMax) {
        entry = kHistoryMax;
    }
}

void clearHistory(HistoryTable& history) noexcept {
    for (auto& row : history) {
        row.fill(0);
    }
}

}  // namespace search
}  // namespace rooksac
