#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

#include "rooksac/legal_movegen.hpp"
#include "rooksac/material.hpp"
#include "rooksac/negamax_agent.hpp"
#include "rooksac/position.hpp"
#include "rooksac/search.hpp"

namespace {

using namespace rooksac;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

/// \brief Reference Minimax implementation returning score from White's perspective.
/// Maximizes for White, minimizes for Black, without Negamax sign inversion.
std::int32_t referenceMinimax(
    Position& position,
    std::uint32_t depth,
    std::uint32_t ply,
    bool maximizingPlayer) {
    if (position.halfmoveClock() >= 100) {
        return search::kDrawScore;
    }

    MoveList legalMoves;
    movegen::generateLegalMoves(position, legalMoves);

    if (legalMoves.empty()) {
        if (movegen::isInCheck(position)) {
            // If maximizing player (White) is in check with no moves, White lost: -kMateScore + ply
            // If minimizing player (Black) is in check with no moves, Black lost (White won): kMateScore - ply
            return maximizingPlayer
                ? (-search::kMateScore + static_cast<std::int32_t>(ply))
                : (search::kMateScore - static_cast<std::int32_t>(ply));
        }
        return search::kDrawScore;
    }

    if (depth == 0) {
        // Material evaluation from White's perspective
        return eval::evaluateMaterial(position);
    }

    if (maximizingPlayer) {
        std::int32_t maxEval = -search::kInfinity;
        for (std::size_t i = 0; i < legalMoves.size(); ++i) {
            const Move m = legalMoves[i];
            UndoState undo;
            (void)position.makeMove(m, undo);
            const std::int32_t evaluation = referenceMinimax(position, depth - 1, ply + 1, false);
            position.unmakeMove(m, undo);
            maxEval = std::max(maxEval, evaluation);
        }
        return maxEval;
    } else {
        std::int32_t minEval = search::kInfinity;
        for (std::size_t i = 0; i < legalMoves.size(); ++i) {
            const Move m = legalMoves[i];
            UndoState undo;
            (void)position.makeMove(m, undo);
            const std::int32_t evaluation = referenceMinimax(position, depth - 1, ply + 1, true);
            position.unmakeMove(m, undo);
            minEval = std::min(minEval, evaluation);
        }
        return minEval;
    }
}

void testTerminalScoresAndDraws() {
    // 1. Stalemate position: Black king on a8 has no moves, not in check.
    // FEN: 7k/5Q2/6K1/8/8/8/8/8 b - - 0 1 -> wait, in 7k/5Q2/6K1... is Kh8 in check? Qf7 to h8 is not attack. No moves for king.
    auto staleOpt = Position::fromFen("k7/2Q5/2K5/8/8/8/8/8 b - - 0 1");
    require(staleOpt.hasValue(), "Stalemate FEN parses");
    auto stalePos = *staleOpt;

    search::SearchLimits limits{.depth = 2};
    auto staleRes = search::search(stalePos, limits);
    require(!staleRes.bestMove.has_value(), "Stalemate returns nullopt move");
    require(staleRes.score == search::kDrawScore, "Stalemate returns 0 score");

    // 2. Fifty-move draw rule: halfmove clock >= 100
    auto fiftyOpt = Position::fromFen("4k3/8/8/8/8/8/8/4K3 w - - 100 50");
    require(fiftyOpt.hasValue(), "Fifty-move FEN parses");
    auto fiftyPos = *fiftyOpt;
    auto fiftyRes = search::search(fiftyPos, limits);
    require(!fiftyRes.bestMove.has_value(), "Fifty move draw at root returns nullopt move");
    require(fiftyRes.score == search::kDrawScore, "Fifty move draw returns 0 score");

    // 3. Checkmate at root: Black king on h8 checkmated by Qh7# with Kg6
    // FEN: 7k/7Q/6K1/8/8/8/8/8 b - - 0 1
    auto mateOpt = Position::fromFen("7k/7Q/6K1/8/8/8/8/8 b - - 0 1");
    require(mateOpt.hasValue(), "Checkmate FEN parses");
    auto matePos = *mateOpt;
    auto mateRes = search::search(matePos, limits);
    require(!mateRes.bestMove.has_value(), "Checkmated position at root returns nullopt move");
    require(mateRes.score == -search::kMateScore, "Checkmated position at root scores -kMateScore");
}

void testMateInOneFixtures() {
    // Back-rank mate in one for White: Qd1 to d8#
    // White: Ke1, Qd1; Black: Kh8, pawns f7, g7, h7
    auto posOpt = Position::fromFen("7k/5ppp/8/8/8/8/8/3QK3 w - - 0 1");
    require(posOpt.hasValue(), "Mate in 1 FEN parses");
    auto pos = *posOpt;

    search::SearchLimits limits{.depth = 1};
    auto res = search::search(pos, limits);
    require(res.bestMove.has_value(), "Search finds mate in one move");
    require(res.bestMove->from() == Square::D1 && res.bestMove->to() == Square::D8,
            "White plays Qd1-d8#");
    require(res.score == search::kMateScore - 1, "Mate in 1 score is kMateScore - 1");
    require(search::isMateScore(res.score), "isMateScore recognizes mate in 1");
}

void testColourSymmetry() {
    // White to move mate-in-1:
    auto whiteOpt = Position::fromFen("7k/5ppp/8/8/8/8/8/3QK3 w - - 0 1");
    require(whiteOpt.hasValue(), "White mate in 1 FEN parses");
    auto whitePos = *whiteOpt;

    // Mirrored Black to move mate-in-1:
    // Black: Ke8, Qd8; White: Kh1, pawns f2, g2, h2
    auto blackOpt = Position::fromFen("3qk3/8/8/8/8/8/5PPP/7K b - - 0 1");
    require(blackOpt.hasValue(), "Black mate in 1 FEN parses");
    auto blackPos = *blackOpt;

    search::SearchLimits limits{.depth = 1};
    auto whiteRes = search::search(whitePos, limits);
    auto blackRes = search::search(blackPos, limits);

    require(whiteRes.score == blackRes.score, "Scores must be symmetrical from side-to-move perspective");
    require(whiteRes.score == search::kMateScore - 1, "Both scores equal kMateScore - 1");

    require(whiteRes.bestMove.has_value() && blackRes.bestMove.has_value(), "Both find best move");
    require(whiteRes.bestMove->from() == Square::D1 && whiteRes.bestMove->to() == Square::D8,
            "White plays Qd1-d8#");
    require(blackRes.bestMove->from() == Square::D8 && blackRes.bestMove->to() == Square::D1,
            "Black plays Qd8-d1#");
}

void testGreedyWeaknessAvoidanceAtDepthTwo() {
    // White queen on d1, Black pawn on d5 defended by pawn on c6:
    // FEN: 4k3/8/2p5/3p4/8/8/8/3QK3 w - - 0 1
    // At depth 1, 1. Qxd5 looks tempting (+100 material).
    // At depth 2, Negamax sees 1. Qxd5 cxd5, losing the queen for a net -800 centipawns!
    // Negamax must therefore REJECT Qxd5 at depth 2.
    auto posOpt = Position::fromFen("4k3/8/2p5/3p4/8/8/8/3QK3 w - - 0 1");
    require(posOpt.hasValue(), "Defended pawn trap FEN parses");
    auto pos = *posOpt;

    search::SearchLimits limitsDepth1{.depth = 1};
    auto res1 = search::search(pos, limitsDepth1);
    require(res1.bestMove.has_value(), "Depth 1 finds a move");
    // At depth 1, queen takes pawn for +100
    require(res1.bestMove->from() == Square::D1 && res1.bestMove->to() == Square::D5,
            "Depth 1 greedily takes defended pawn");

    search::SearchLimits limitsDepth2{.depth = 2};
    auto res2 = search::search(pos, limitsDepth2);
    require(res2.bestMove.has_value(), "Depth 2 finds a move");
    // At depth 2, Qxd5 is avoided because of the recapture
    const bool tookDefendedPawn = (res2.bestMove->from() == Square::D1 && res2.bestMove->to() == Square::D5);
    require(!tookDefendedPawn, "Depth 2 Negamax lookahead avoids hanging the Queen on d5");
}

void testStatePreservationInvariant() {
    auto pos = Position::startPosition();
    const std::string originalFen = pos.toFen();

    search::SearchLimits limits{.depth = 2};
    auto res = search::search(pos, limits);

    require(res.bestMove.has_value(), "Start position returns legal move");
    require(res.stats.nodes > 20, "Depth 2 searched multiple nodes");
    require(res.stats.maxDepth == 2, "Max depth reached 2 plies");
    require(pos.toFen() == originalFen, "Position FEN is strictly preserved after search");
}

void testReferenceMinimaxEquivalence() {
    // Compare Negamax against Reference Minimax across multiple test positions at depth 1 and 2
    const char* testFens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "4k3/8/8/4q3/4Q3/8/8/4K3 w - - 0 1",
        "4k3/8/2p5/3p4/8/8/8/3QK3 w - - 0 1",
        "3qk3/8/8/8/8/8/5PPP/7K b - - 0 1"
    };

    for (const char* fen : testFens) {
        auto posOpt = Position::fromFen(fen);
        require(posOpt.hasValue(), "FEN parses");
        auto pos = *posOpt;

        for (std::uint32_t depth = 1; depth <= 2; ++depth) {
            search::SearchLimits limits{.depth = depth};
            auto negamaxRes = search::search(pos, limits);

            const bool isWhite = (pos.sideToMove() == Color::White);
            const std::int32_t refScore = referenceMinimax(pos, depth, 0, isWhite);

            const std::int32_t expectedNegamaxScore = isWhite ? refScore : -refScore;
            require(negamaxRes.score == expectedNegamaxScore,
                    "Negamax score strictly matches Reference Minimax score");
        }
    }
}

void testNegamaxAgentWrapper() {
    NegamaxAgent agent(/*depth=*/2);
    require(agent.name() == "NegamaxAgent", "Agent name matches");
    require(agent.depth() == 2, "Agent initial depth is 2");

    auto pos = Position::startPosition();
    MoveList legalMoves;
    movegen::generateLegalMoves(pos, legalMoves);

    const auto move = agent.selectMove(pos, legalMoves);
    require(move.has_value(), "NegamaxAgent selects a move");
    require(legalMoves.contains(*move), "Selected move is strictly legal");
}

}  // namespace

int main() {
    testTerminalScoresAndDraws();
    testMateInOneFixtures();
    testColourSymmetry();
    testGreedyWeaknessAvoidanceAtDepthTwo();
    testStatePreservationInvariant();
    testReferenceMinimaxEquivalence();
    testNegamaxAgentWrapper();

    std::cout << "All Minimax and Negamax search tests passed successfully.\n";
    return EXIT_SUCCESS;
}
