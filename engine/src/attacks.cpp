#include "rooksac/attacks.hpp"

namespace rooksac {
namespace attacks {

Bitboard attacksByColor(
    Color attackingColor,
    Bitboard occupancy,
    Bitboard pawns,
    Bitboard knights,
    Bitboard bishops,
    Bitboard rooks,
    Bitboard queens,
    Bitboard kings) noexcept {
    Bitboard allAttacks = kEmptyBitboard;

    for (const Square sq : BitboardSquares(pawns)) {
        allAttacks |= pawnAttacks(sq, attackingColor);
    }
    for (const Square sq : BitboardSquares(knights)) {
        allAttacks |= knightAttacks(sq);
    }
    for (const Square sq : BitboardSquares(bishops | queens)) {
        allAttacks |= bishopAttacks(sq, occupancy);
    }
    for (const Square sq : BitboardSquares(rooks | queens)) {
        allAttacks |= rookAttacks(sq, occupancy);
    }
    for (const Square sq : BitboardSquares(kings)) {
        allAttacks |= kingAttacks(sq);
    }

    return allAttacks;
}

bool isSquareAttacked(
    Square targetSquare,
    Color attackingColor,
    Bitboard occupancy,
    Bitboard pawns,
    Bitboard knights,
    Bitboard bishops,
    Bitboard rooks,
    Bitboard queens,
    Bitboard kings) noexcept {
    if (!isValidSquare(targetSquare)) {
        return false;
    }

    // A square is attacked by an enemy pawn if a pawn of attackingColor can attack targetSquare.
    // That is equivalent to: pawnAttacks(targetSquare, opposite(attackingColor)) & pawns.
    if ((pawnAttacks(targetSquare, opposite(attackingColor)) & pawns) != 0) {
        return true;
    }
    if ((knightAttacks(targetSquare) & knights) != 0) {
        return true;
    }
    if ((kingAttacks(targetSquare) & kings) != 0) {
        return true;
    }
    if ((bishopAttacks(targetSquare, occupancy) & (bishops | queens)) != 0) {
        return true;
    }
    if ((rookAttacks(targetSquare, occupancy) & (rooks | queens)) != 0) {
        return true;
    }

    return false;
}

}  // namespace attacks
}  // namespace rooksac
