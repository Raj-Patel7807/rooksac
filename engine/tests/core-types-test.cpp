#include <array>
#include <cstdlib>
#include <iostream>

#include "rooksac/bitboard.hpp"
#include "rooksac/move.hpp"
#include "rooksac/types.hpp"

namespace {

using rooksac::Bitboard;
using rooksac::Color;
using rooksac::File;
using rooksac::Move;
using rooksac::Piece;
using rooksac::PieceType;
using rooksac::Rank;
using rooksac::Square;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void testSquareMapping() {
    for (std::uint8_t index = 0; index < rooksac::kSquareCount; ++index) {
        const Square square = static_cast<Square>(index);
        const auto made = rooksac::tryMakeSquare(index % 8U, index / 8U);
        require(made.has_value() && *made == square, "file/rank square construction must cover all squares");
        require(rooksac::squareIndex(square) == index, "square index must round trip");
        require(static_cast<std::uint8_t>(rooksac::fileOf(square)) == index % 8U,
                "file must match square index");
        require(static_cast<std::uint8_t>(rooksac::rankOf(square)) == index / 8U,
                "rank must match square index");

        const auto algebraic = rooksac::squareToAlgebraic(square);
        require(algebraic[0] == static_cast<char>('a' + index % 8U),
                "algebraic file must match square index");
        require(algebraic[1] == static_cast<char>('1' + index / 8U),
                "algebraic rank must match square index");
        const auto parsed = rooksac::squareFromAlgebraic(algebraic[0], algebraic[1]);
        require(parsed.has_value() && *parsed == square, "algebraic coordinates must round trip");
    }

    require(rooksac::makeSquare(File::A, Rank::One) == Square::A1, "a1 must be square zero");
    require(rooksac::makeSquare(File::H, Rank::Eight) == Square::H8, "h8 must be square 63");
    require(!rooksac::tryMakeSquare(8U, 0U).has_value(), "invalid file must be rejected");
    require(!rooksac::tryMakeSquare(0U, 8U).has_value(), "invalid rank must be rejected");
    require(!rooksac::squareFromAlgebraic('i', '1').has_value(), "invalid algebraic file must be rejected");
    require(!rooksac::squareFromAlgebraic('a', '9').has_value(), "invalid algebraic rank must be rejected");
    require(rooksac::squareToAlgebraic(Square::None) == std::array<char, 2>{'?', '?'},
            "invalid square must have a safe coordinate representation");
}

void testMasksAndBitOperations() {
    constexpr std::array<Bitboard, 8> files = {
        rooksac::kFileA, rooksac::kFileB, rooksac::kFileC, rooksac::kFileD,
        rooksac::kFileE, rooksac::kFileF, rooksac::kFileG, rooksac::kFileH,
    };
    constexpr std::array<Bitboard, 8> ranks = {
        rooksac::kRank1, rooksac::kRank2, rooksac::kRank3, rooksac::kRank4,
        rooksac::kRank5, rooksac::kRank6, rooksac::kRank7, rooksac::kRank8,
    };

    Bitboard combinedFiles = rooksac::kEmptyBitboard;
    Bitboard combinedRanks = rooksac::kEmptyBitboard;
    for (const Bitboard file : files) {
        require(rooksac::popCount(file) == 8, "every file mask must contain eight squares");
        combinedFiles |= file;
    }
    for (const Bitboard rank : ranks) {
        require(rooksac::popCount(rank) == 8, "every rank mask must contain eight squares");
        combinedRanks |= rank;
    }
    require(combinedFiles == rooksac::kFullBitboard, "file masks must cover the board");
    require(combinedRanks == rooksac::kFullBitboard, "rank masks must cover the board");
    require((rooksac::kDarkSquares & rooksac::kLightSquares) == rooksac::kEmptyBitboard,
            "square colors must not overlap");
    require((rooksac::kDarkSquares | rooksac::kLightSquares) == rooksac::kFullBitboard,
            "square colors must cover the board");
    require(rooksac::testBit(rooksac::kDarkSquares, Square::A1), "a1 must be dark");
    require(rooksac::popCount(rooksac::kBoardEdge) == 28, "board edge must contain 28 squares");

    Bitboard board = rooksac::kEmptyBitboard;
    require(rooksac::leastSignificantSquare(board) == Square::None, "empty board has no least bit");
    require(rooksac::popLeastSignificantBit(board) == Square::None, "popping empty board is safe");
    rooksac::setBit(board, Square::A1);
    rooksac::setBit(board, Square::H8);
    rooksac::setBit(board, Square::None);
    require(rooksac::popCount(board) == 2, "invalid squares must not set a bit");
    require(rooksac::testBit(board, Square::A1), "set bit must be observable");
    rooksac::clearBit(board, Square::A1);
    require(!rooksac::testBit(board, Square::A1), "cleared bit must be absent");
    require(rooksac::popLeastSignificantBit(board) == Square::H8, "pop must return the least bit");
    require(board == rooksac::kEmptyBitboard, "pop must clear the returned bit");

    board = rooksac::bit(Square::C2) | rooksac::bit(Square::A1) | rooksac::bit(Square::H8);
    constexpr std::array<Square, 3> expected = {Square::A1, Square::C2, Square::H8};
    std::size_t index = 0;
    for (const Square square : rooksac::BitboardSquares(board)) {
        require(square == expected[index], "bitboard iteration must be least-bit first");
        ++index;
    }
    require(index == expected.size(), "bitboard iteration must visit every bit once");
}

void testPiecesAndMoves() {
    require(rooksac::makePiece(Color::White, PieceType::Queen) == Piece::WhiteQueen,
            "white queen encoding must be stable");
    require(rooksac::makePiece(Color::Black, PieceType::Knight) == Piece::BlackKnight,
            "black knight encoding must be stable");
    require(rooksac::typeOf(Piece::BlackKing) == PieceType::King, "piece type must decode");
    require(rooksac::colorOf(Piece::BlackKing) == Color::Black, "piece color must decode");
    require(!rooksac::colorOf(Piece::None).has_value(), "empty piece has no color");

    constexpr std::array<PieceType, 5> promotions = {
        PieceType::None, PieceType::Knight, PieceType::Bishop, PieceType::Rook, PieceType::Queen,
    };
    for (std::uint8_t fromIndex = 0; fromIndex < rooksac::kSquareCount; ++fromIndex) {
        for (std::uint8_t toIndex = 0; toIndex < rooksac::kSquareCount; ++toIndex) {
            if (fromIndex == toIndex) {
                continue;
            }
            for (const PieceType promotion : promotions) {
                const auto move = Move::create(static_cast<Square>(fromIndex),
                                               static_cast<Square>(toIndex), promotion);
                require(move.has_value(), "valid move fields must encode");
                require(move->from() == static_cast<Square>(fromIndex), "move source must decode");
                require(move->to() == static_cast<Square>(toIndex), "move target must decode");
                require(move->promotion() == promotion, "move promotion must decode");
            }
        }
    }

    require(!Move::create(Square::None, Square::A1).has_value(), "invalid source must be rejected");
    require(!Move::create(Square::A1, Square::None).has_value(), "invalid target must be rejected");
    require(!Move::create(Square::A1, Square::A1).has_value(), "null moves must be rejected");
    require(!Move::create(Square::A7, Square::A8, PieceType::King).has_value(),
            "invalid promotion must be rejected");
}

}  // namespace

int main() {
    testSquareMapping();
    testMasksAndBitOperations();
    testPiecesAndMoves();
    return EXIT_SUCCESS;
}
