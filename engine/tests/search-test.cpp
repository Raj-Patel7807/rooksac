#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>

#include "rooksac/legal_movegen.hpp"
#include "rooksac/material.hpp"
#include "rooksac/move_order.hpp"
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
            return maximizingPlayer
                ? (-search::kMateScore + static_cast<std::int32_t>(ply))
                : (search::kMateScore - static_cast<std::int32_t>(ply));
        }
        return search::kDrawScore;
    }

    if (depth == 0) {
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
    auto mateOpt = Position::fromFen("7k/7Q/6K1/8/8/8/8/8 b - - 0 1");
    require(mateOpt.hasValue(), "Checkmate FEN parses");
    auto matePos = *mateOpt;
    auto mateRes = search::search(matePos, limits);
    require(!mateRes.bestMove.has_value(), "Checkmated position at root returns nullopt move");
    require(mateRes.score == -search::kMateScore, "Checkmated position at root scores -kMateScore");
}

void testMateInOneFixtures() {
    // Back-rank mate in one for White: Qd1 to d8#
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
    auto whiteOpt = Position::fromFen("7k/5ppp/8/8/8/8/8/3QK3 w - - 0 1");
    require(whiteOpt.hasValue(), "White mate in 1 FEN parses");
    auto whitePos = *whiteOpt;

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
    auto posOpt = Position::fromFen("4k3/8/2p5/3p4/8/8/8/3QK3 w - - 0 1");
    require(posOpt.hasValue(), "Defended pawn trap FEN parses");
    auto pos = *posOpt;

    search::SearchLimits limitsDepth1{.depth = 1};
    auto res1 = search::search(pos, limitsDepth1);
    require(res1.bestMove.has_value(), "Depth 1 finds a move");
    require(res1.bestMove->from() == Square::D1 && res1.bestMove->to() == Square::D5,
            "Depth 1 greedily takes defended pawn");

    search::SearchLimits limitsDepth2{.depth = 2};
    auto res2 = search::search(pos, limitsDepth2);
    require(res2.bestMove.has_value(), "Depth 2 finds a move");
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

void testMoveOrderingPriorities() {
    // White pawn on e7 can promote to Queen (quiet promotion) or capture black rook on d8 with promotion
    // White knight on f3 can capture black queen on d4
    // FEN: 3r4/4P3/8/8/3q4/5N2/8/4K2k w - - 0 1
    auto posOpt = Position::fromFen("3r4/4P3/8/8/3q4/5N2/8/4K2k w - - 0 1");
    require(posOpt.hasValue(), "Move ordering FEN parses");
    auto pos = *posOpt;

    MoveList legalMoves;
    movegen::generateLegalMoves(pos, legalMoves);
    require(legalMoves.size() > 5, "Has multiple legal moves");

    // Capture with promotion: e7xd8=Q
    auto capPromo = Move::create(Square::E7, Square::D8, PieceType::Queen);
    require(capPromo.has_value(), "CapPromo created");
    const std::int32_t capPromoScore = search::scoreMove(pos, *capPromo);
    require(capPromoScore >= search::kCapturePromoBaseScore, "Capture + promo has highest priority base");

    // Quiet promotion: e7-e8=Q
    auto quietPromo = Move::create(Square::E7, Square::E8, PieceType::Queen);
    require(quietPromo.has_value(), "QuietPromo created");
    const std::int32_t quietPromoScore = search::scoreMove(pos, *quietPromo);
    require(quietPromoScore >= search::kPromoBaseScore && quietPromoScore < search::kCapturePromoBaseScore,
            "Quiet promo has promo base score");

    // MVV-LVA Capture: Nf3xd4 (Knight takes Queen)
    auto nTakesQ = Move::create(Square::F3, Square::D4);
    require(nTakesQ.has_value(), "Nxd4 created");
    const std::int32_t nTakesQScore = search::scoreMove(pos, *nTakesQ);
    require(nTakesQScore >= search::kCaptureBaseScore && nTakesQScore < search::kPromoBaseScore,
            "Capture has capture base score");

    // Order moves and verify staging order
    search::orderMoves(pos, legalMoves);
    require(legalMoves[0].encoded() == capPromo->encoded(),
            "Move ordering places best capture-with-promotion first");

    // All capture-promotions should come first (scores >= 30,000)
    // Then quiet promotions (scores 20,000 - 29,999)
    // Then captures (scores 10,000 - 19,999)
    // Then quiet moves (score 0)
    std::int32_t prevScore = search::kInfinity;
    for (std::size_t i = 0; i < legalMoves.size(); ++i) {
        const std::int32_t score = search::scoreMove(pos, legalMoves[i]);
        require(score <= prevScore, "Moves must be in non-ascending priority score order");
        prevScore = score;
    }

    // Verify quiet promotion e7-e8=Q is ahead of Nxd4
    std::size_t quietPromoIdx = legalMoves.size();
    std::size_t nTakesQIdx = legalMoves.size();
    for (std::size_t i = 0; i < legalMoves.size(); ++i) {
        if (legalMoves[i].encoded() == quietPromo->encoded()) quietPromoIdx = i;
        if (legalMoves[i].encoded() == nTakesQ->encoded()) nTakesQIdx = i;
    }
    require(quietPromoIdx < nTakesQIdx, "Quiet queen promotion is ordered ahead of ordinary capture");
}

void testDifferentialMinimaxVsAlphaBeta() {
    // Verify that Alpha-Beta produces mathematically identical scores to plain unpruned Minimax
    // across diverse positions and depths (1, 2, 3).
    const char* fens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "4k3/8/8/4q3/4Q3/8/8/4K3 w - - 0 1",
        "4k3/8/2p5/3p4/8/8/8/3QK3 w - - 0 1",
        "7k/5ppp/8/8/8/8/8/3QK3 w - - 0 1",
        "3qk3/8/8/8/8/8/5PPP/7K b - - 0 1"
    };

    for (const char* fen : fens) {
        auto posOpt = Position::fromFen(fen);
        require(posOpt.hasValue(), "FEN parses");
        auto pos = *posOpt;

        MoveList legalMoves;
        movegen::generateLegalMoves(pos, legalMoves);

        for (std::uint32_t depth = 1; depth <= 3; ++depth) {
            search::SearchLimits limits{.depth = depth};

            auto minimaxRes = search::searchMinimax(pos, limits);
            auto alphaBetaRes = search::search(pos, limits);

            require(minimaxRes.score == alphaBetaRes.score,
                    "Alpha-Beta score strictly matches Minimax score at all depths");

            if (alphaBetaRes.bestMove.has_value()) {
                require(legalMoves.contains(*alphaBetaRes.bestMove),
                        "Alpha-Beta best move is strictly legal");
            }

            // Alpha-Beta must visit no more nodes than plain Minimax
            require(alphaBetaRes.stats.nodes <= minimaxRes.stats.nodes,
                    "Alpha-Beta visits <= nodes than unpruned Minimax");
        }
    }
}

void testAlphaBetaCutoffsAndNodeReduction() {
    // In positions with multiple moves and captures, Alpha-Beta with move ordering
    // should achieve beta cutoffs and search strictly fewer nodes than Minimax.
    auto posOpt = Position::fromFen("r1bqk2r/pppp1ppp/2n5/4p3/1b1Pn3/2N2N2/PPP1PPPP/R1BQKB1R w KQkq - 2 6");
    require(posOpt.hasValue(), "Tactical FEN parses");
    auto pos = *posOpt;

    search::SearchLimits limits{.depth = 3};
    auto minimaxRes = search::searchMinimax(pos, limits);
    auto alphaBetaRes = search::search(pos, limits);

    require(minimaxRes.score == alphaBetaRes.score, "Scores match at depth 3");
    require(alphaBetaRes.stats.nodes < minimaxRes.stats.nodes,
            "Alpha-Beta visits strictly fewer nodes than plain Minimax");
    require(alphaBetaRes.stats.cutoffs > 0, "Alpha-Beta achieved beta cutoffs");

    std::cout << "Node reduction at depth 3: Minimax = " << minimaxRes.stats.nodes
              << ", Alpha-Beta = " << alphaBetaRes.stats.nodes
              << " (" << alphaBetaRes.stats.cutoffs << " beta cutoffs)\n";
}

void testAlphaBetaFailLowFailHighExact() {
    auto posOpt = Position::fromFen("4k3/8/8/4q3/4Q3/8/8/4K3 w - - 0 1");
    require(posOpt.hasValue(), "FEN parses");
    auto pos = *posOpt;

    // First obtain exact root score via infinite window [-infinity, +infinity]
    search::SearchLimits limits{.depth = 2};
    auto exactRes = search::search(pos, limits);
    const std::int32_t trueScore = exactRes.score;

    // 1. Exact window: [trueScore - 100, trueScore + 100]
    search::SearchStats statsExact;
    const std::int32_t exactScore = search::alphaBeta(
        pos, trueScore - 100, trueScore + 100, 2, 0, statsExact,
        [](const Position& p) { return eval::evaluateMaterialSideToMove(p); });
    require(exactScore == trueScore, "Exact window returns exact score");

    // 2. Fail-high window: [trueScore - 200, trueScore - 50]
    search::SearchStats statsFailHigh;
    const std::int32_t failHighScore = search::alphaBeta(
        pos, trueScore - 200, trueScore - 50, 2, 0, statsFailHigh,
        [](const Position& p) { return eval::evaluateMaterialSideToMove(p); });
    require(failHighScore >= trueScore - 50, "Fail-high score >= beta");
    require(statsFailHigh.cutoffs > 0, "Fail-high produced beta cutoffs");

    // 3. Fail-low window: [trueScore + 50, trueScore + 200]
    search::SearchStats statsFailLow;
    const std::int32_t failLowScore = search::alphaBeta(
        pos, trueScore + 50, trueScore + 200, 2, 0, statsFailLow,
        [](const Position& p) { return eval::evaluateMaterialSideToMove(p); });
    require(failLowScore <= trueScore + 50, "Fail-low score <= alpha");
}

void testCheckEvasionsAndPromotions() {
    // 1. Check evasion: White king on e1 is in check by Black rook on e8.
    // FEN: 4r3/8/8/8/8/8/8/4K3 w - - 0 1
    // King must move to d1, d2, f1, or f2.
    auto checkOpt = Position::fromFen("4r3/8/8/8/8/8/8/4K3 w - - 0 1");
    require(checkOpt.hasValue(), "Check evasion FEN parses");
    auto checkPos = *checkOpt;

    MoveList checkMoves;
    movegen::generateLegalMoves(checkPos, checkMoves);
    require(!checkMoves.empty(), "King has evasions");

    search::SearchLimits limits{.depth = 2};
    auto checkRes = search::search(checkPos, limits);
    require(checkRes.bestMove.has_value(), "Search finds check evasion");
    require(checkMoves.contains(*checkRes.bestMove), "Evasion move is strictly legal");

    // 2. Promotion: White pawn on a7 can promote to a8=Q
    auto promoOpt = Position::fromFen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    require(promoOpt.hasValue(), "Promotion FEN parses");
    auto promoPos = *promoOpt;

    auto promoRes = search::search(promoPos, limits);
    require(promoRes.bestMove.has_value(), "Search finds promotion");
    require(promoRes.bestMove->from() == Square::A7 && promoRes.bestMove->to() == Square::A8,
            "Pawn promotes on a8");
    require(promoRes.bestMove->promotion() == PieceType::Queen, "Pawn promotes to Queen");
}

void testReferenceMinimaxEquivalence() {
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
            auto searchRes = search::search(pos, limits);

            const bool isWhite = (pos.sideToMove() == Color::White);
            const std::int32_t refScore = referenceMinimax(pos, depth, 0, isWhite);

            const std::int32_t expectedScore = isWhite ? refScore : -refScore;
            require(searchRes.score == expectedScore,
                    "Alpha-Beta search score strictly matches Reference Minimax score");
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
    testMoveOrderingPriorities();
    testDifferentialMinimaxVsAlphaBeta();
    testAlphaBetaCutoffsAndNodeReduction();
    testAlphaBetaFailLowFailHighExact();
    testCheckEvasionsAndPromotions();
    testReferenceMinimaxEquivalence();
    testNegamaxAgentWrapper();

    std::cout << "All Alpha-Beta pruning, move ordering, and search tests passed successfully.\n";
    return EXIT_SUCCESS;
}
