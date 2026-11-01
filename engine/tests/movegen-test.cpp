#include <cstdlib>
#include <iostream>
#include <string>

#include "rooksac/attacks.hpp"
#include "rooksac/bitboard.hpp"
#include "rooksac/movegen.hpp"
#include "rooksac/position.hpp"
#include "rooksac/types.hpp"

namespace {

using namespace rooksac;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void testPawnAttacks() {
    // White pawn on D4 attacks C5 and E5
    const Bitboard wD4 = attacks::pawnAttacks(Square::D4, Color::White);
    require(popCount(wD4) == 2, "White D4 attacks 2 squares");
    require(testBit(wD4, Square::C5), "White D4 attacks C5");
    require(testBit(wD4, Square::E5), "White D4 attacks E5");

    // White pawn on A2 (edge) attacks B3 only
    const Bitboard wA2 = attacks::pawnAttacks(Square::A2, Color::White);
    require(popCount(wA2) == 1, "White A2 attacks 1 square");
    require(testBit(wA2, Square::B3), "White A2 attacks B3");

    // Black pawn on D5 attacks C4 and E4
    const Bitboard bD5 = attacks::pawnAttacks(Square::D5, Color::Black);
    require(popCount(bD5) == 2, "Black D5 attacks 2 squares");
    require(testBit(bD5, Square::C4), "Black D5 attacks C4");
    require(testBit(bD5, Square::E4), "Black D5 attacks E4");

    // Black pawn on H7 (edge) attacks G6 only
    const Bitboard bH7 = attacks::pawnAttacks(Square::H7, Color::Black);
    require(popCount(bH7) == 1, "Black H7 attacks 1 square");
    require(testBit(bH7, Square::G6), "Black H7 attacks G6");
}

void testKnightAttacks() {
    // Center square E4 has 8 knight moves
    const Bitboard nE4 = attacks::knightAttacks(Square::E4);
    require(popCount(nE4) == 8, "Knight on E4 has 8 attacks");
    require(testBit(nE4, Square::D6), "Knight E4 attacks D6");
    require(testBit(nE4, Square::F6), "Knight E4 attacks F6");
    require(testBit(nE4, Square::C5), "Knight E4 attacks C5");
    require(testBit(nE4, Square::G5), "Knight E4 attacks G5");
    require(testBit(nE4, Square::C3), "Knight E4 attacks C3");
    require(testBit(nE4, Square::G3), "Knight E4 attacks G3");
    require(testBit(nE4, Square::D2), "Knight E4 attacks D2");
    require(testBit(nE4, Square::F2), "Knight E4 attacks F2");

    // Corner square A1 has 2 knight moves (B3, C2)
    const Bitboard nA1 = attacks::knightAttacks(Square::A1);
    require(popCount(nA1) == 2, "Knight on A1 has 2 attacks");
    require(testBit(nA1, Square::B3), "Knight A1 attacks B3");
    require(testBit(nA1, Square::C2), "Knight A1 attacks C2");
}

void testKingAttacks() {
    // Center square E4 has 8 attacks
    const Bitboard kE4 = attacks::kingAttacks(Square::E4);
    require(popCount(kE4) == 8, "King on E4 has 8 attacks");

    // Corner square H8 has 3 attacks (G8, G7, H7)
    const Bitboard kH8 = attacks::kingAttacks(Square::H8);
    require(popCount(kH8) == 3, "King on H8 has 3 attacks");
    require(testBit(kH8, Square::G8), "King H8 attacks G8");
    require(testBit(kH8, Square::G7), "King H8 attacks G7");
    require(testBit(kH8, Square::H7), "King H8 attacks H7");
}

void testBishopAttacks() {
    // Empty board bishop on D4 has 13 diagonal squares
    const Bitboard emptyOcc = kEmptyBitboard;
    const Bitboard bD4 = attacks::bishopAttacks(Square::D4, emptyOcc);
    require(popCount(bD4) == 13, "Bishop on D4 on empty board has 13 attacks");

    // Blockers on F6 (ray up-right) and B2 (ray down-left)
    Bitboard occ = bit(Square::F6) | bit(Square::B2);
    const Bitboard bBlocked = attacks::bishopAttacks(Square::D4, occ);
    // Up-right should hit E5 and F6, but NOT G7 or H8
    require(testBit(bBlocked, Square::E5), "Bishop hits E5");
    require(testBit(bBlocked, Square::F6), "Bishop hits F6 (blocker included)");
    require(!testBit(bBlocked, Square::G7), "Bishop blocked from G7");
    require(!testBit(bBlocked, Square::H8), "Bishop blocked from H8");

    // Down-left should hit C3 and B2, but NOT A1
    require(testBit(bBlocked, Square::C3), "Bishop hits C3");
    require(testBit(bBlocked, Square::B2), "Bishop hits B2 (blocker included)");
    require(!testBit(bBlocked, Square::A1), "Bishop blocked from A1");
}

void testRookAttacks() {
    // Empty board rook on D4 has 14 cardinal squares (7 rank, 7 file)
    const Bitboard rD4 = attacks::rookAttacks(Square::D4, kEmptyBitboard);
    require(popCount(rD4) == 14, "Rook on D4 on empty board has 14 attacks");

    // Blockers on D6 (ray up) and B4 (ray left)
    Bitboard occ = bit(Square::D6) | bit(Square::B4);
    const Bitboard rBlocked = attacks::rookAttacks(Square::D4, occ);
    // Up should hit D5 and D6, but not D7 or D8
    require(testBit(rBlocked, Square::D5), "Rook hits D5");
    require(testBit(rBlocked, Square::D6), "Rook hits D6 (blocker included)");
    require(!testBit(rBlocked, Square::D7), "Rook blocked from D7");
    require(!testBit(rBlocked, Square::D8), "Rook blocked from D8");

    // Left should hit C4 and B4, but not A4
    require(testBit(rBlocked, Square::C4), "Rook hits C4");
    require(testBit(rBlocked, Square::B4), "Rook hits B4 (blocker included)");
    require(!testBit(rBlocked, Square::A4), "Rook blocked from A4");
}

void testQueenAttacks() {
    // Empty board queen on D4 has 13 bishop + 14 rook = 27 attacks
    const Bitboard qD4 = attacks::queenAttacks(Square::D4, kEmptyBitboard);
    require(popCount(qD4) == 27, "Queen on D4 on empty board has 27 attacks");
}

void testAttackDetection() {
    auto posOpt = Position::fromFen("r1bqkb1r/pppp1ppp/2n5/4p3/2B1n3/5N2/PPPP1PPP/RNBQK2R w KQkq - 0 5");
    require(posOpt.hasValue(), "Position FEN parses");
    const auto& pos = *posOpt;

    // Is Black's square F7 attacked by White?
    // White bishop on C4 attacks F7
    const bool f7Attacked = attacks::isSquareAttacked(
        Square::F7,
        Color::White,
        pos.totalOccupancy(),
        pos.pieces(Color::White, PieceType::Pawn),
        pos.pieces(Color::White, PieceType::Knight),
        pos.pieces(Color::White, PieceType::Bishop),
        pos.pieces(Color::White, PieceType::Rook),
        pos.pieces(Color::White, PieceType::Queen),
        pos.pieces(Color::White, PieceType::King));
    require(f7Attacked, "F7 is attacked by White (Bishop on C4)");

    // Is White's king on E1 attacked by Black?
    const bool e1Attacked = attacks::isSquareAttacked(
        Square::E1,
        Color::Black,
        pos.totalOccupancy(),
        pos.pieces(Color::Black, PieceType::Pawn),
        pos.pieces(Color::Black, PieceType::Knight),
        pos.pieces(Color::Black, PieceType::Bishop),
        pos.pieces(Color::Black, PieceType::Rook),
        pos.pieces(Color::Black, PieceType::Queen),
        pos.pieces(Color::Black, PieceType::King));
    require(!e1Attacked, "White king on E1 is NOT attacked");
}

void testStartPositionMoves() {
    const auto pos = Position::startPosition();
    MoveList moves;
    movegen::generatePseudoLegalMoves(pos, moves);

    // Initial position has exactly 20 pseudo-legal (and legal) moves:
    // 16 pawn moves (8 single pushes + 8 double pushes) + 4 knight moves (b1-a3, b1-c3, g1-f3, g1-h3)
    require(moves.size() == 20, "Start position has exactly 20 pseudo-legal moves");

    MoveList quiets;
    movegen::generatePseudoLegalQuiets(pos, quiets);
    require(quiets.size() == 20, "All start position moves are quiet moves");

    MoveList captures;
    movegen::generatePseudoLegalCaptures(pos, captures);
    require(captures.empty(), "Start position has 0 captures/promotions");
}

void testPseudoLegalCastlingAndPromotions() {
    // 1. Castling and move partition on Kiwipete
    const std::string fen = "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1";
    auto posOpt = Position::fromFen(fen);
    require(posOpt.hasValue(), "Test position parses");
    const auto& pos = *posOpt;

    MoveList moves;
    movegen::generatePseudoLegalMoves(pos, moves);

    // Partition test: captures + quiets == total moves
    MoveList caps;
    MoveList quiets;
    movegen::generatePseudoLegalCaptures(pos, caps);
    movegen::generatePseudoLegalQuiets(pos, quiets);
    require(caps.size() + quiets.size() == moves.size(), "Captures + quiets count matches total");

    // 2. Promotion position: White pawn on A7, A8 empty, B8 has Black rook
    // Moves for A7 pawn: push a7-a8 (4 promos) and capture a7xb8 (4 promos)
    const std::string promoFen = "1r2k3/P7/8/8/8/8/8/4K3 w - - 0 1";
    auto promoPosOpt = Position::fromFen(promoFen);
    require(promoPosOpt.hasValue(), "Promo position parses");
    const auto& promoPos = *promoPosOpt;

    MoveList promoMoves;
    movegen::generatePseudoLegalMoves(promoPos, promoMoves);

    require(promoMoves.contains(*Move::create(Square::A7, Square::A8, PieceType::Queen)), "Pawn push promo Q");
    require(promoMoves.contains(*Move::create(Square::A7, Square::A8, PieceType::Rook)), "Pawn push promo R");
    require(promoMoves.contains(*Move::create(Square::A7, Square::A8, PieceType::Bishop)), "Pawn push promo B");
    require(promoMoves.contains(*Move::create(Square::A7, Square::A8, PieceType::Knight)), "Pawn push promo N");

    require(promoMoves.contains(*Move::create(Square::A7, Square::B8, PieceType::Queen)), "Pawn capture promo Q");
    require(promoMoves.contains(*Move::create(Square::A7, Square::B8, PieceType::Rook)), "Pawn capture promo R");
    require(promoMoves.contains(*Move::create(Square::A7, Square::B8, PieceType::Bishop)), "Pawn capture promo B");
    require(promoMoves.contains(*Move::create(Square::A7, Square::B8, PieceType::Knight)), "Pawn capture promo N");

    MoveList promoCaps;
    MoveList promoQuiets;
    movegen::generatePseudoLegalCaptures(promoPos, promoCaps);
    movegen::generatePseudoLegalQuiets(promoPos, promoQuiets);
    require(promoCaps.size() + promoQuiets.size() == promoMoves.size(), "Promo captures + quiets count matches total");
}

void testEnPassantGeneration() {
    const std::string fen = "rnbqkbnr/pp1ppppp/8/2pP4/8/8/PPP1PPPP/RNBQKBNR w KQkq c6 0 2";
    auto posOpt = Position::fromFen(fen);
    require(posOpt.hasValue(), "EP test position parses");
    const auto& pos = *posOpt;

    MoveList moves;
    movegen::generatePseudoLegalMoves(pos, moves);

    // d5xc6 is generated
    const auto epMove = *Move::create(Square::D5, Square::C6);
    require(moves.contains(epMove), "En passant move d5xc6 is generated");

    MoveList caps;
    movegen::generatePseudoLegalCaptures(pos, caps);
    require(caps.contains(epMove), "En passant move is classified in captures");
}

}  // namespace

int main() {
    testPawnAttacks();
    testKnightAttacks();
    testKingAttacks();
    testBishopAttacks();
    testRookAttacks();
    testQueenAttacks();
    testAttackDetection();
    testStartPositionMoves();
    testPseudoLegalCastlingAndPromotions();
    testEnPassantGeneration();

    std::cout << "All attacks and pseudo-legal move generation tests passed successfully.\n";
    return EXIT_SUCCESS;
}
