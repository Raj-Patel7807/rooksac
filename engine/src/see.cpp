#include "rooksac/see.hpp"

#include <algorithm>
#include <array>

#include "rooksac/bitboard.hpp"

namespace rooksac {
namespace search {

namespace {

/// \brief Returns all pieces of both sides directly attacking the target square,
/// given the current occupancy bitboard.
[[nodiscard]] Bitboard attackersTo(const Position& position, Square target, Bitboard occ) noexcept {
    Bitboard attackers = kEmptyBitboard;

    // Pawns: pawns that attack target are on squares from which a pawn attacks target.
    // White pawn on sq attacks target -> sq is in black pawn attacks from target
    attackers |= attacks::pawnAttacks(target, Color::Black) & position.pieces(Color::White, PieceType::Pawn);
    // Black pawn on sq attacks target -> sq is in white pawn attacks from target
    attackers |= attacks::pawnAttacks(target, Color::White) & position.pieces(Color::Black, PieceType::Pawn);

    // Knights
    attackers |= attacks::knightAttacks(target) &
                 (position.pieces(Color::White, PieceType::Knight) | position.pieces(Color::Black, PieceType::Knight));

    // Kings
    attackers |= attacks::kingAttacks(target) &
                 (position.pieces(Color::White, PieceType::King) | position.pieces(Color::Black, PieceType::King));

    // Bishops and Queens (diagonals)
    const Bitboard diagAttackers = position.pieces(Color::White, PieceType::Bishop) |
                                  position.pieces(Color::Black, PieceType::Bishop) |
                                  position.pieces(Color::White, PieceType::Queen)  |
                                  position.pieces(Color::Black, PieceType::Queen);
    if ((diagAttackers & occ) != 0) {
        attackers |= attacks::bishopAttacks(target, occ) & diagAttackers;
    }

    // Rooks and Queens (orthogonals)
    const Bitboard orthAttackers = position.pieces(Color::White, PieceType::Rook) |
                                  position.pieces(Color::Black, PieceType::Rook) |
                                  position.pieces(Color::White, PieceType::Queen) |
                                  position.pieces(Color::Black, PieceType::Queen);
    if ((orthAttackers & occ) != 0) {
        attackers |= attacks::rookAttacks(target, occ) & orthAttackers;
    }

    return attackers & occ;
}

/// \brief Selects the least valuable attacker of sideToMove attacking target.
/// Returns Square::None if no attacker exists.
[[nodiscard]] Square getLeastValuableAttacker(
    const Position& position,
    Bitboard attackers,
    Color sideToMove,
    PieceType& pieceTypeOut) noexcept {
    const Bitboard sidePieces = position.colorOccupancy(sideToMove);
    const Bitboard sideAttackers = attackers & sidePieces;
    if (sideAttackers == kEmptyBitboard) {
        pieceTypeOut = PieceType::None;
        return Square::None;
    }

    // Try in order: Pawn, Knight, Bishop, Rook, Queen, King
    constexpr std::array<PieceType, 6> order = {
        PieceType::Pawn,
        PieceType::Knight,
        PieceType::Bishop,
        PieceType::Rook,
        PieceType::Queen,
        PieceType::King
    };

    for (const PieceType pt : order) {
        const Bitboard cand = sideAttackers & position.pieces(sideToMove, pt);
        if (cand != kEmptyBitboard) {
            pieceTypeOut = pt;
            return leastSignificantSquare(cand);
        }
    }

    pieceTypeOut = PieceType::None;
    return Square::None;
}

}  // namespace

std::int32_t see(const Position& position, Move move) noexcept {
    const Square fromSq = move.from();
    const Square toSq   = move.to();
    if (!isValidSquare(fromSq) || !isValidSquare(toSq)) {
        return 0;
    }

    const Piece movedPiece = position.pieceAt(fromSq);
    if (movedPiece == Piece::None) {
        return 0;
    }

    Piece targetPiece = position.pieceAt(toSq);
    bool isEnPassant = false;

    if (typeOf(movedPiece) == PieceType::Pawn && isValidSquare(position.enPassantSquare()) &&
        toSq == position.enPassantSquare()) {
        isEnPassant = true;
        targetPiece = makePiece(opposite(position.sideToMove()), PieceType::Pawn);
    }

    // gain[0] is the material value of the captured piece (or promotion gain)
    // gain[d] is the net swing when sideToMove captures piece of type currentAttackerType:
    // gain[d] = pieceTypeValue(currentAttackerType)
    // Then in backward pass:
    // For d from depth-1 down to 0:
    // gain[d] = max(0, gain[d] - gain[d+1])
    // Finally return gain[0].
    //
    // Let's implement standard minimax swap cleanly:
    std::int32_t targetVal = (targetPiece != Piece::None) ? eval::pieceTypeValue(typeOf(targetPiece)) : 0;
    if (move.promotion() != PieceType::None) {
        targetVal += (eval::pieceTypeValue(move.promotion()) - eval::pieceTypeValue(PieceType::Pawn));
    }

    std::array<std::int32_t, 32> dList{};
    dList[0] = targetVal;
    std::size_t numCaptures = 1;

    Bitboard occ = position.totalOccupancy();
    occ &= ~bit(fromSq);
    if (isEnPassant) {
        const Square epCapturedSq = (position.sideToMove() == Color::White)
                                        ? static_cast<Square>(static_cast<std::uint8_t>(toSq) - 8U)
                                        : static_cast<Square>(static_cast<std::uint8_t>(toSq) + 8U);
        occ &= ~bit(epCapturedSq);
    }

    Bitboard attackers = attackersTo(position, toSq, occ);
    Color sideToMove = opposite(position.sideToMove());
    PieceType currentAttackerType = (move.promotion() != PieceType::None) ? move.promotion() : typeOf(movedPiece);

    while (numCaptures < dList.size()) {
        PieceType nextAttackerType = PieceType::None;
        const Square nextSq = getLeastValuableAttacker(position, attackers, sideToMove, nextAttackerType);
        if (!isValidSquare(nextSq) || nextAttackerType == PieceType::None) {
            break;
        }

        dList[numCaptures] = eval::pieceTypeValue(currentAttackerType);
        ++numCaptures;

        // If the attacking piece was king, and opponent still has attackers, king cannot capture
        // (the loop would terminate on king anyway as least valuable attacker is King last).
        occ &= ~bit(nextSq);

        const Bitboard diagAttackers = position.pieces(Color::White, PieceType::Bishop) |
                                      position.pieces(Color::Black, PieceType::Bishop) |
                                      position.pieces(Color::White, PieceType::Queen)  |
                                      position.pieces(Color::Black, PieceType::Queen);
        if ((diagAttackers & occ) != 0) {
            attackers |= (attacks::bishopAttacks(toSq, occ) & diagAttackers);
        }

        const Bitboard orthAttackers = position.pieces(Color::White, PieceType::Rook) |
                                      position.pieces(Color::Black, PieceType::Rook) |
                                      position.pieces(Color::White, PieceType::Queen) |
                                      position.pieces(Color::Black, PieceType::Queen);
        if ((orthAttackers & occ) != 0) {
            attackers |= (attacks::rookAttacks(toSq, occ) & orthAttackers);
        }

        attackers &= occ;
        currentAttackerType = nextAttackerType;
        sideToMove = opposite(sideToMove);
    }

    // Backward minimax resolution:
    // A player can always choose to stand pat (not recapture) if recapturing yields a net loss.
    // For i = numCaptures - 1 down to 1:
    // dList[i - 1] = max(0, dList[i - 1] - dList[i])
    // Special case for root (i = 0): the initial move was already played, so root is dList[0] - dList[1].
    for (std::size_t i = numCaptures - 1; i > 0; --i) {
        if (i == 1) {
            dList[0] = dList[0] - dList[1];
        } else {
            dList[i - 1] = std::max(0, dList[i - 1] - dList[i]);
        }
    }

    return dList[0];
}

}  // namespace search
}  // namespace rooksac
