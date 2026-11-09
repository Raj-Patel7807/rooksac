#include "rooksac/match.hpp"

#include <sstream>

#include "rooksac/legal_movegen.hpp"
#include "rooksac/move_list.hpp"
#include "rooksac/uci.hpp"
#include "rooksac/undo_state.hpp"

namespace rooksac {

std::string MatchResult::toPgn(
    std::string_view whiteName,
    std::string_view blackName,
    std::string_view eventName,
    std::string_view site,
    std::string_view initialFen) const {
    std::ostringstream ss;
    ss << "[Event \"" << eventName << "\"]\n";
    ss << "[Site \"" << site << "\"]\n";
    ss << "[Date \"????.??.??\"]\n";
    ss << "[Round \"-\"]\n";
    ss << "[White \"" << whiteName << "\"]\n";
    ss << "[Black \"" << blackName << "\"]\n";

    std::string resultStr = "*";
    if (winner.has_value()) {
        resultStr = (*winner == Color::White) ? "1-0" : "0-1";
    } else if (result == GameResult::Stalemate ||
               result == GameResult::FiftyMoveDraw ||
               result == GameResult::InsufficientMaterialDraw) {
        resultStr = "1/2-1/2";
    }
    ss << "[Result \"" << resultStr << "\"]\n";

    if (!initialFen.empty()) {
        ss << "[SetUp \"1\"]\n";
        ss << "[FEN \"" << initialFen << "\"]\n";
    }
    if (!terminationReason.empty()) {
        ss << "[Termination \"" << terminationReason << "\"]\n";
    }
    ss << "\n";

    // Format moves in algebraic coordinate notation numbered per full move
    for (std::size_t i = 0; i < moveHistory.size(); ++i) {
        if (i % 2 == 0) {
            ss << (i / 2 + 1) << ". ";
        }
        ss << uci::formatMove(moveHistory[i]) << " ";
    }
    ss << resultStr << "\n\n";

    return ss.str();
}

MatchResult playMatch(
    Agent& whiteAgent,
    Agent& blackAgent,
    Position initialPosition,
    std::uint32_t maxPlies) {
    MatchResult match;
    Position position = initialPosition;

    for (std::uint32_t ply = 0; ply < maxPlies; ++ply) {
        MoveList legalMoves;
        movegen::generateLegalMoves(position, legalMoves);

        const GameResult state = movegen::evaluateGameState(position);
        if (state != GameResult::Ongoing) {
            match.result = state;
            if (state == GameResult::Checkmate) {
                // The side to move is checkmated, so the winner is the opposing side
                match.winner = opposite(position.sideToMove());
                match.terminationReason = "Checkmate";
            } else if (state == GameResult::Stalemate) {
                match.terminationReason = "Stalemate";
            } else if (state == GameResult::FiftyMoveDraw) {
                match.terminationReason = "Fifty-move rule draw";
            } else if (state == GameResult::InsufficientMaterialDraw) {
                match.terminationReason = "Insufficient material draw";
            }
            break;
        }

        Agent& currentAgent = (position.sideToMove() == Color::White) ? whiteAgent : blackAgent;
        const auto moveOpt = currentAgent.selectMove(position, legalMoves);
        if (!moveOpt.has_value()) {
            match.result = GameResult::Ongoing;
            match.terminationReason = "Agent returned no move";
            break;
        }

        UndoState undo;
        const bool success = position.makeMove(*moveOpt, undo);
        (void)success;
        match.moveHistory.push_back(*moveOpt);
        match.totalPlies++;
    }

    if (match.result == GameResult::Ongoing && match.totalPlies >= maxPlies) {
        match.terminationReason = "Max plies reached";
    }

    match.finalFen = position.toFen();
    return match;
}

}  // namespace rooksac
