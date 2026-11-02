#include "rooksac/match.hpp"

#include "rooksac/legal_movegen.hpp"
#include "rooksac/move_list.hpp"
#include "rooksac/undo_state.hpp"

namespace rooksac {

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
