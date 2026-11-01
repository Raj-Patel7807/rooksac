#include <cassert>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "rooksac/castling.hpp"
#include "rooksac/fen.hpp"
#include "rooksac/move.hpp"
#include "rooksac/position.hpp"
#include "rooksac/types.hpp"
#include "rooksac/undo_state.hpp"

namespace {

using rooksac::CastlingRights;
using rooksac::Color;
using rooksac::FenParseError;
using rooksac::File;
using rooksac::Move;
using rooksac::Piece;
using rooksac::PieceType;
using rooksac::Position;
using rooksac::Rank;
using rooksac::Square;
using rooksac::UndoState;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void testStartPositionFen() {
    auto posOpt = Position::fromFen(Position::kStartPositionFen);
    require(posOpt.hasValue(), "Start position FEN must parse successfully");
    const auto& pos = *posOpt;

    require(pos.sideToMove() == Color::White, "Start position side to move must be White");
    require(pos.castlingRights() == CastlingRights::All, "Start position must have all castling rights");
    require(pos.enPassantSquare() == Square::None, "Start position en-passant square must be None");
    require(pos.halfmoveClock() == 0, "Start position halfmove clock must be 0");
    require(pos.fullmoveNumber() == 1, "Start position fullmove number must be 1");

    require(pos.pieceAt(Square::E1) == Piece::WhiteKing, "E1 must be White King");
    require(pos.pieceAt(Square::D1) == Piece::WhiteQueen, "D1 must be White Queen");
    require(pos.pieceAt(Square::E8) == Piece::BlackKing, "E8 must be Black King");
    require(pos.pieceAt(Square::D8) == Piece::BlackQueen, "D8 must be Black Queen");

    require(pos.toFen() == Position::kStartPositionFen, "Start position FEN must round-trip exactly");
}

void testFenCorpus() {
    const std::vector<std::string> validFens = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "8/8/8/8/8/8/8/k6K w - - 50 100",
        "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1",
    };

    for (const auto& fen : validFens) {
        auto posOpt = Position::fromFen(fen);
        require(posOpt.hasValue(), "Valid FEN must parse successfully");
        require(posOpt->checkInvariants(), "Position must satisfy invariants");
        require(posOpt->toFen() == fen, "Valid FEN must round-trip exactly");
    }

    const std::vector<std::string> invalidFens = {
        "",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0",      // 5 fields
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 2",  // 7 fields
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP w KQkq - 0 1",             // 7 ranks
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR/8 w KQkq - 0 1", // 9 ranks
        "rnbqkbnr/pppppppp/9/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",    // rank too long
        "rnbqkbnr/pppppppp/7/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",    // rank too short
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1",    // invalid side
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq e4 0 1",    // invalid ep rank (rank 4 for white)
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq e2 0 1",    // invalid ep rank
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w X - 0 1",       // invalid castling
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - -1 1",   // negative halfmove
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0",    // zero fullmove
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNX w KQkq - 0 1",    // invalid piece char
    };

    for (const auto& fen : invalidFens) {
        auto posOpt = Position::fromFen(fen);
        require(!posOpt.hasValue(), "Invalid FEN must be rejected");
    }
}

void testMakeUnmakeQuietAndPawnMoves() {
    auto posOpt = Position::fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    require(posOpt.hasValue(), "Valid start position");
    auto pos = *posOpt;
    const std::string startFen = pos.toFen();

    // 1. e2-e4 (double pawn push, sets ep square to e3, resets clock)
    auto move1 = *Move::create(Square::E2, Square::E4);
    UndoState undo1;
    require(pos.makeMove(move1, undo1), "make e2-e4 must succeed");
    require(pos.enPassantSquare() == Square::E3, "EP square must be e3 after e2-e4");
    require(pos.sideToMove() == Color::Black, "Side to move must be Black");
    require(pos.halfmoveClock() == 0, "Halfmove clock must be 0 after pawn push");
    require(pos.fullmoveNumber() == 1, "Fullmove number must be 1 during Black move");

    // 2. g8-f6 (quiet knight move, resets ep, increments clock and fullmove)
    auto move2 = *Move::create(Square::G8, Square::F6);
    UndoState undo2;
    require(pos.makeMove(move2, undo2), "make g8-f6 must succeed");
    require(pos.enPassantSquare() == Square::None, "EP square must be reset after quiet move");
    require(pos.sideToMove() == Color::White, "Side to move must be White");
    require(pos.halfmoveClock() == 1, "Halfmove clock must be 1 after quiet knight move");
    require(pos.fullmoveNumber() == 2, "Fullmove number must increment after Black move");

    // Unmake 2
    pos.unmakeMove(move2, undo2);
    require(pos.enPassantSquare() == Square::E3, "EP square restored to e3");
    require(pos.sideToMove() == Color::Black, "Side to move restored to Black");
    require(pos.halfmoveClock() == 0, "Halfmove clock restored to 0");
    require(pos.fullmoveNumber() == 1, "Fullmove number restored to 1");
    require(pos.pieceAt(Square::G8) == Piece::BlackKnight, "Knight back on G8");
    require(pos.pieceAt(Square::F6) == Piece::None, "F6 empty");

    // Unmake 1
    pos.unmakeMove(move1, undo1);
    require(pos.toFen() == startFen, "Full undo must restore start position exactly");
}

void testMakeUnmakeCastling() {
    // White can castle kingside (e1-g1) and queenside (e1-c1)
    const std::string fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";
    auto posOpt = Position::fromFen(fen);
    require(posOpt.hasValue(), "Valid castling test fen");
    auto pos = *posOpt;

    // Castle White Kingside: e1-g1 (rook moves h1->f1)
    auto o_o = *Move::create(Square::E1, Square::G1);
    UndoState undo;
    require(pos.makeMove(o_o, undo), "Make O-O must succeed");
    require(pos.pieceAt(Square::G1) == Piece::WhiteKing, "King on G1");
    require(pos.pieceAt(Square::F1) == Piece::WhiteRook, "Rook on F1");
    require(pos.pieceAt(Square::E1) == Piece::None, "E1 empty");
    require(pos.pieceAt(Square::H1) == Piece::None, "H1 empty");
    require(!rooksac::hasCastlingRight(pos.castlingRights(), CastlingRights::WhiteKingside), "White lost kingside right");
    require(!rooksac::hasCastlingRight(pos.castlingRights(), CastlingRights::WhiteQueenside), "White lost queenside right");

    pos.unmakeMove(o_o, undo);
    require(pos.toFen() == fen, "Unmake O-O must restore position exactly");

    // Castle White Queenside: e1-c1 (rook moves a1->d1)
    auto o_o_o = *Move::create(Square::E1, Square::C1);
    require(pos.makeMove(o_o_o, undo), "Make O-O-O must succeed");
    require(pos.pieceAt(Square::C1) == Piece::WhiteKing, "King on C1");
    require(pos.pieceAt(Square::D1) == Piece::WhiteRook, "Rook on D1");
    require(pos.pieceAt(Square::E1) == Piece::None, "E1 empty");
    require(pos.pieceAt(Square::A1) == Piece::None, "A1 empty");

    pos.unmakeMove(o_o_o, undo);
    require(pos.toFen() == fen, "Unmake O-O-O must restore position exactly");
}

void testMakeUnmakeEnPassant() {
    const std::string fen = "rnbqkbnr/pp1ppppp/8/2pP4/8/8/PPP1PPPP/RNBQKBNR w KQkq c6 0 2";
    auto posOpt = Position::fromFen(fen);
    require(posOpt.hasValue(), "Valid en-passant test fen");
    auto pos = *posOpt;

    // En passant capture d5xc6
    auto epMove = *Move::create(Square::D5, Square::C6);
    UndoState undo;
    require(pos.makeMove(epMove, undo), "Make EP move must succeed");
    require(pos.pieceAt(Square::C6) == Piece::WhitePawn, "White pawn on C6");
    require(pos.pieceAt(Square::D5) == Piece::None, "D5 empty");
    require(pos.pieceAt(Square::C5) == Piece::None, "Black pawn on C5 captured");
    require(undo.capturedPiece == Piece::BlackPawn, "Undo state captured piece is Black Pawn");

    pos.unmakeMove(epMove, undo);
    require(pos.toFen() == fen, "Unmake EP move must restore position exactly");
    require(pos.pieceAt(Square::C5) == Piece::BlackPawn, "Black pawn restored to C5");
    require(pos.pieceAt(Square::D5) == Piece::WhitePawn, "White pawn restored to D5");
    require(pos.pieceAt(Square::C6) == Piece::None, "C6 empty");
}

void testMakeUnmakePromotion() {
    const std::string fen = "8/4P3/8/8/8/8/8/k6K w - - 0 1";
    auto posOpt = Position::fromFen(fen);
    require(posOpt.hasValue(), "Valid promotion test fen");
    auto pos = *posOpt;

    // Promote e7-e8=Q
    auto promoMove = *Move::create(Square::E7, Square::E8, PieceType::Queen);
    UndoState undo;
    require(pos.makeMove(promoMove, undo), "Make promotion must succeed");
    require(pos.pieceAt(Square::E8) == Piece::WhiteQueen, "E8 has White Queen");
    require(pos.pieceAt(Square::E7) == Piece::None, "E7 empty");

    pos.unmakeMove(promoMove, undo);
    require(pos.toFen() == fen, "Unmake promotion must restore pawn on E7");
    require(pos.pieceAt(Square::E7) == Piece::WhitePawn, "White pawn restored to E7");
    require(pos.pieceAt(Square::E8) == Piece::None, "E8 empty");
}

void testMoveSequenceUndo() {
    auto posOpt = Position::fromFen(Position::kStartPositionFen);
    auto pos = *posOpt;
    const std::string initialFen = pos.toFen();

    struct MoveRecord {
        Move move;
        UndoState undo;
    };
    std::vector<MoveRecord> history;

    // 1. e2-e4
    auto m1 = *Move::create(Square::E2, Square::E4);
    UndoState u1;
    require(pos.makeMove(m1, u1), "Move 1 must succeed");
    history.push_back({m1, u1});

    // 1... e7-e5
    auto m2 = *Move::create(Square::E7, Square::E5);
    UndoState u2;
    require(pos.makeMove(m2, u2), "Move 2 must succeed");
    history.push_back({m2, u2});

    // 2. g1-f3
    auto m3 = *Move::create(Square::G1, Square::F3);
    UndoState u3;
    require(pos.makeMove(m3, u3), "Move 3 must succeed");
    history.push_back({m3, u3});

    // 2... b8-c6
    auto m4 = *Move::create(Square::B8, Square::C6);
    UndoState u4;
    require(pos.makeMove(m4, u4), "Move 4 must succeed");
    history.push_back({m4, u4});

    // 3. f1-c4
    auto m5 = *Move::create(Square::F1, Square::C4);
    UndoState u5;
    require(pos.makeMove(m5, u5), "Move 5 must succeed");
    history.push_back({m5, u5});

    // Unmake all moves in reverse order
    while (!history.empty()) {
        const auto rec = history.back();
        history.pop_back();
        pos.unmakeMove(rec.move, rec.undo);
    }

    require(pos.toFen() == initialFen, "Sequence unmake must restore initial FEN exactly");
}

}  // namespace

int main() {
    testStartPositionFen();
    testFenCorpus();
    testMakeUnmakeQuietAndPawnMoves();
    testMakeUnmakeCastling();
    testMakeUnmakeEnPassant();
    testMakeUnmakePromotion();
    testMoveSequenceUndo();
    std::cout << "All Position and FEN tests passed successfully.\n";
    return EXIT_SUCCESS;
}
