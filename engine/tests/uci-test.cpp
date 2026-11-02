#include <cassert>
#include <iostream>
#include <sstream>
#include <string>

#include "rooksac/legal_movegen.hpp"
#include "rooksac/position.hpp"
#include "rooksac/uci.hpp"

namespace {

using namespace rooksac;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Assertion failed: " << message << '\n';
        std::abort();
    }
}

void testMoveFormattingAndParsing() {
    auto pos = Position::startPosition();

    // 1. Legal pawn move e2e4
    auto moveOpt = uci::parseMove(pos, "e2e4");
    require(moveOpt.has_value(), "parseMove parses valid e2e4");
    require(uci::formatMove(*moveOpt) == "e2e4", "formatMove outputs e2e4");

    // 2. Illegal move e2e5 (not legal in startpos)
    auto illegalOpt = uci::parseMove(pos, "e2e5");
    require(!illegalOpt.has_value(), "parseMove rejects illegal e2e5");

    // 3. Malformed string
    auto malformed1 = uci::parseMove(pos, "e2");
    require(!malformed1.has_value(), "parseMove rejects too short move");
    auto malformed2 = uci::parseMove(pos, "z9a1");
    require(!malformed2.has_value(), "parseMove rejects invalid square coordinates");

    // 4. Promotion move
    auto promoPos = Position::fromFen("8/4P3/8/8/8/8/8/4K2k w - - 0 1");
    require(promoPos.hasValue(), "Promotion FEN valid");
    auto promoMoveOpt = uci::parseMove(*promoPos, "e7e8q");
    require(promoMoveOpt.has_value(), "parseMove parses valid queen promotion");
    require(promoMoveOpt->promotion() == PieceType::Queen, "Promotion piece is Queen");
    require(uci::formatMove(*promoMoveOpt) == "e7e8q", "formatMove outputs e7e8q");

    auto underpromoMoveOpt = uci::parseMove(*promoPos, "e7e8n");
    require(underpromoMoveOpt.has_value(), "parseMove parses valid knight promotion");
    require(underpromoMoveOpt->promotion() == PieceType::Knight, "Promotion piece is Knight");
    require(uci::formatMove(*underpromoMoveOpt) == "e7e8n", "formatMove outputs e7e8n");
}

void testUciHandshakeAndReady() {
    std::istringstream in("uci\nisready\nquit\n");
    std::ostringstream out;
    std::ostringstream err;

    uci::EngineSession session(in, out, err);
    session.run();

    const std::string output = out.str();
    require(output.find("id name RookSac 0.1.0\n") != std::string::npos, "Emits id name");
    require(output.find("id author RookSac Authors\n") != std::string::npos, "Emits id author");
    require(output.find("option name Hash type spin default 16 min 1 max 1024\n") != std::string::npos, "Emits Hash option");
    require(output.find("uciok\n") != std::string::npos, "Emits uciok");
    require(output.find("readyok\n") != std::string::npos, "Emits readyok");
    require(err.str().empty(), "No errors on standard handshake");
}

void testPositionStartposAndMoves() {
    std::istringstream in("position startpos moves e2e4 e7e5\nquit\n");
    std::ostringstream out;
    std::ostringstream err;

    uci::EngineSession session(in, out, err);
    session.run();

    const Position& pos = session.currentPosition();
    // After 1. e4 e5, side to move is White and fullmove is 2
    require(pos.sideToMove() == Color::White, "Side to move after 1. e4 e5 is White");
    require(pos.fullmoveNumber() == 2, "Fullmove number is 2");
    require(pos.pieceAt(Square::E4) == Piece::WhitePawn, "White pawn is on e4");
    require(pos.pieceAt(Square::E5) == Piece::BlackPawn, "Black pawn is on e5");
    require(pos.pieceAt(Square::E2) == Piece::None, "e2 is empty");
    require(pos.pieceAt(Square::E7) == Piece::None, "e7 is empty");
    require(err.str().empty(), "No errors on valid position command");
}

void testPositionFenAndMoves() {
    std::istringstream in(
        "position fen r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3 moves f1b5 a7a6\nquit\n");
    std::ostringstream out;
    std::ostringstream err;

    uci::EngineSession session(in, out, err);
    session.run();

    const Position& pos = session.currentPosition();
    require(pos.sideToMove() == Color::White, "Side to move after moves is White");
    require(pos.pieceAt(Square::B5) == Piece::WhiteBishop, "White bishop is on b5");
    require(pos.pieceAt(Square::A6) == Piece::BlackPawn, "Black pawn is on a6");
    require(err.str().empty(), "No errors on valid FEN position command");
}

void testMalformedInputHandling() {
    std::istringstream in(
        "boguscommand\n"
        "position fen invalid_fen_string\n"
        "position startpos moves e2e5\n"
        "setoption name NonExistentOption value 123\n"
        "isready\n"
        "quit\n");
    std::ostringstream out;
    std::ostringstream err;

    uci::EngineSession session(in, out, err);
    session.run();

    const std::string output = out.str();
    require(output == "readyok\n", "Only readyok emitted to stdout; errors go to stderr");

    const std::string errOutput = err.str();
    require(errOutput.find("unknown command: boguscommand") != std::string::npos, "Logged unknown command");
    require(errOutput.find("invalid FEN") != std::string::npos, "Logged invalid FEN");
    require(errOutput.find("illegal or malformed move") != std::string::npos, "Logged illegal move");
    require(errOutput.find("setoption received") != std::string::npos, "Logged unsupported setoption");
}

void testGoDepthAndBestMove() {
    std::istringstream in("position startpos\ngo depth 2\nquit\n");
    std::ostringstream out;
    std::ostringstream err;

    uci::EngineSession session(in, out, err);
    session.run();

    const std::string output = out.str();
    require(output.find("bestmove ") != std::string::npos, "Emits bestmove");

    const std::size_t idx = output.find("bestmove ");
    const std::string moveStr = output.substr(idx + 9, 4);
    auto pos = Position::startPosition();
    auto parsedMove = uci::parseMove(pos, moveStr);
    require(parsedMove.has_value(), "Emitted bestmove is strictly legal in startpos");
}

void testGoMovetimeAndClock() {
    // Search with movetime 20ms
    std::istringstream in("position startpos\ngo movetime 20\nquit\n");
    std::ostringstream out;
    std::ostringstream err;

    uci::EngineSession session(in, out, err);
    session.run();

    const std::string output = out.str();
    require(output.find("bestmove ") != std::string::npos, "Emits bestmove under movetime limit");

    // Search with wtime and btime
    std::istringstream inClock("position startpos\ngo wtime 10000 btime 10000 winc 100 binc 100 movestogo 40\nquit\n");
    std::ostringstream outClock;
    std::ostringstream errClock;

    uci::EngineSession sessionClock(inClock, outClock, errClock);
    sessionClock.run();

    const std::string clockOutput = outClock.str();
    require(clockOutput.find("bestmove ") != std::string::npos, "Emits bestmove under clock parameters");
}

void testStopCommandInterruptsSearch() {
    // Search infinite or high depth, then send stop
    std::istringstream in("position startpos\ngo infinite\nstop\nquit\n");
    std::ostringstream out;
    std::ostringstream err;

    uci::EngineSession session(in, out, err);
    session.run();

    const std::string output = out.str();
    require(output.find("bestmove ") != std::string::npos, "Emits bestmove when stopped");
}

}  // namespace

int main() {
    testMoveFormattingAndParsing();
    testUciHandshakeAndReady();
    testPositionStartposAndMoves();
    testPositionFenAndMoves();
    testMalformedInputHandling();
    testGoDepthAndBestMove();
    testGoMovetimeAndClock();
    testStopCommandInterruptsSearch();

    std::cout << "All UCI protocol and engine session tests passed successfully.\n";
    return 0;
}

