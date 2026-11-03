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

    search::SearchLimits limits(2);
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

    search::SearchLimits limits(1);
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

    search::SearchLimits limits(1);
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

    // Without quiescence (unpruned plain minimax at depth 1), White falls into the horizon trap
    search::SearchLimits limitsDepth1(1);
    auto resMinimax1 = search::searchMinimax(pos, limitsDepth1);
    require(resMinimax1.bestMove.has_value(), "Depth 1 minimax finds a move");
    require(resMinimax1.bestMove->from() == Square::D1 && resMinimax1.bestMove->to() == Square::D5,
            "Depth 1 unpruned minimax without quiescence greedily takes defended pawn");

    // With quiescence search (even at depth 1), the tactical recapture cxd5 is seen, avoiding the queen loss!
    auto resQ1 = search::search(pos, limitsDepth1);
    require(resQ1.bestMove.has_value(), "Depth 1 search finds a move");
    const bool q1TookDefendedPawn = (resQ1.bestMove->from() == Square::D1 && resQ1.bestMove->to() == Square::D5);
    require(!q1TookDefendedPawn, "Depth 1 with Quiescence search avoids hanging the Queen on d5");

    search::SearchLimits limitsDepth2(2);
    auto res2 = search::search(pos, limitsDepth2);
    require(res2.bestMove.has_value(), "Depth 2 finds a move");
    const bool tookDefendedPawn = (res2.bestMove->from() == Square::D1 && res2.bestMove->to() == Square::D5);
    require(!tookDefendedPawn, "Depth 2 lookahead avoids hanging the Queen on d5");
}

void testStatePreservationInvariant() {
    auto pos = Position::startPosition();
    const std::string originalFen = pos.toFen();

    search::SearchLimits limits(2);
    auto res = search::search(pos, limits);

    require(res.bestMove.has_value(), "Start position returns legal move");
    require(res.stats.nodes > 20, "Depth 2 searched multiple nodes");
    require(res.stats.maxDepth >= 2, "Max depth reached at least 2 plies");
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
    // Verify that Alpha-Beta search produces mathematically identical scores to plain unpruned Minimax
    // on quiet positions where no tactical horizon distortion occurs.
    const char* quietFens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "4k3/8/8/8/8/8/8/4K3 w - - 0 1"
    };

    for (const char* fen : quietFens) {
        auto posOpt = Position::fromFen(fen);
        require(posOpt.hasValue(), "FEN parses");
        auto pos = *posOpt;

        MoveList legalMoves;
        movegen::generateLegalMoves(pos, legalMoves);

        for (std::uint32_t depth = 1; depth <= 2; ++depth) {
            search::SearchLimits limits(depth);

            auto minimaxRes = search::searchMinimax(pos, limits);
            auto alphaBetaRes = search::search(pos, limits);

            require(minimaxRes.score == alphaBetaRes.score,
                    "Alpha-Beta score strictly matches Minimax score on quiet positions");

            if (alphaBetaRes.bestMove.has_value()) {
                require(legalMoves.contains(*alphaBetaRes.bestMove),
                        "Alpha-Beta best move is strictly legal");
            }
        }
    }
}

void testAlphaBetaCutoffsAndNodeReduction() {
    // In positions with multiple moves and captures, Alpha-Beta with move ordering
    // should achieve beta cutoffs and search strictly fewer nodes than Minimax.
    auto posOpt = Position::fromFen("r1bqk2r/pppp1ppp/2n5/4p3/1b1Pn3/2N2N2/PPP1PPPP/R1BQKB1R w KQkq - 2 6");
    require(posOpt.hasValue(), "Tactical FEN parses");
    auto pos = *posOpt;

    search::SearchLimits limits(3);
    auto minimaxRes = search::searchMinimax(pos, limits);
    auto alphaBetaRes = search::search(pos, limits);

    require(alphaBetaRes.bestMove.has_value(), "Alpha-Beta search finds a move");
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
    search::SearchLimits limits(2);
    auto exactRes = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateMaterialSideToMove(p);
    });
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

    search::SearchLimits limits(2);
    auto checkRes = search::search(checkPos, limits);
    require(checkRes.bestMove.has_value(), "Search finds check evasion");
    require(checkMoves.contains(*checkRes.bestMove), "Evasion move is strictly legal");

    // 2. Promotion: White pawn on a7 can promote to a8=Q
    auto promoOpt = Position::fromFen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    require(promoOpt.hasValue(), "Promotion FEN parses");
    auto promoPos = *promoOpt;

    auto promoRes = search::search(promoPos, limits, [](const Position& p) {
        return eval::evaluateMaterialSideToMove(p);
    });
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
            search::SearchLimits limits(depth);
            auto searchRes = search::searchMinimax(pos, limits, [](const Position& p) {
                return eval::evaluateMaterialSideToMove(p);
            });

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

void testQuiescenceTacticalHorizon() {
    // Tactical horizon fixture:
    // White queen on d1, Black pawn on d5 defended by pawn on c6:
    // FEN: 4k3/8/2p5/3p4/8/8/8/3QK3 w - - 0 1
    // If White plays 1. Qxd5, White is statically up a pawn (+100).
    // In Quiescence search, Black immediately plays 1... cxd5, capturing the Queen.
    // The resulting quiescence score is -800 (net queen loss), exposing the horizon blunder!
    auto posOpt = Position::fromFen("4k3/8/2p5/3p4/8/8/8/3QK3 w - - 0 1");
    require(posOpt.hasValue(), "Defended pawn FEN parses");
    auto pos = *posOpt;

    // Simulate position after White plays 1. Qxd5:
    auto qxd5 = Move::create(Square::D1, Square::D5);
    require(qxd5.has_value(), "Qxd5 created");

    UndoState undo;
    (void)pos.makeMove(*qxd5, undo);

    // Black is to move. Black has legal capture cxd5:
    search::SearchStats qstats;
    const std::int32_t qscore = search::quiescence(
        pos, -search::kInfinity, search::kInfinity, 1, qstats,
        [](const Position& p) { return eval::evaluateMaterialSideToMove(p); });

    pos.unmakeMove(*qxd5, undo);

    // From Black's perspective after 1. Qxd5, stand-pat is -800 (down a queen).
    // By capturing the queen (1... cxd5), Black ends up up a pawn (+100).
    require(qscore == 100, "Quiescence search correctly captures the hung queen on d5, scoring +100");
}

void testQuiescenceCheckHandling() {
    // Check in quiescence: King on e1 is in check by Black rook on e2.
    // FEN: 8/8/8/8/8/8/4r3/4K2k w - - 0 1
    // Statically White is down a rook (-500). Stand-pat would be -500.
    // Because White is in check, stand-pat is prohibited and White searches evasions,
    // finding Kxe2 which captures the checking rook and brings score to 0!
    auto checkOpt = Position::fromFen("8/8/8/8/8/8/4r3/4K2k w - - 0 1");
    require(checkOpt.hasValue(), "Check FEN parses");
    auto checkPos = *checkOpt;

    search::SearchStats qstats;
    const std::int32_t qscore = search::quiescence(
        checkPos, -search::kInfinity, search::kInfinity, 0, qstats,
        [](const Position& p) { return eval::evaluateMaterialSideToMove(p); });

    require(qscore == 0, "King captures checking rook in quiescence to restore material to 0");

    // Checkmate in quiescence: White king checkmated on h8
    // FEN: 7k/7Q/6K1/8/8/8/8/8 b - - 0 1
    auto mateOpt = Position::fromFen("7k/7Q/6K1/8/8/8/8/8 b - - 0 1");
    require(mateOpt.hasValue(), "Mate FEN parses");
    auto matePos = *mateOpt;

    search::SearchStats mateStats;
    const std::int32_t mateScore = search::quiescence(
        matePos, -search::kInfinity, search::kInfinity, 0, mateStats,
        [](const Position& p) { return eval::evaluateMaterialSideToMove(p); });

    require(mateScore == -search::kMateScore, "Checkmate in quiescence returns -kMateScore");
}

void testIterativeDeepeningAndFallbackMove() {
    auto pos = Position::startPosition();
    MoveList legalMoves;
    movegen::generateLegalMoves(pos, legalMoves);

    // Iterative deepening to depth 3
    search::SearchLimits limits(3);
    auto res = search::search(pos, limits);

    require(res.completedDepth == 3, "Iterative deepening reached requested depth 3");
    require(res.bestMove.has_value(), "Returns a best move");
    require(legalMoves.contains(*res.bestMove), "Returned move is strictly legal");
    require(!res.interrupted, "Search was not interrupted");
}

void testCooperativeStopToken() {
    auto pos = Position::startPosition();
    MoveList legalMoves;
    movegen::generateLegalMoves(pos, legalMoves);

    search::SearchStopToken stopToken;
    stopToken.stop();  // Pre-stop the token

    search::SearchLimits limits(10);
    auto res = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateMaterialSideToMove(p);
    }, &stopToken);

    require(res.interrupted, "Search flagged as interrupted");
    require(res.completedDepth == 0, "No iterations completed under immediate stop");
    require(res.bestMove.has_value(), "Fallback move is guaranteed");
    require(legalMoves.contains(*res.bestMove), "Fallback move is strictly legal");
}

void testTimeManagerAllocation() {
    // 1. Explicit move time
    TimeControl tcMoveTime;
    tcMoveTime.moveTimeMs = 500;
    TimeAllocation alloc1 = TimeManager::allocate(tcMoveTime);
    require(alloc1.optimalTimeMs == 500 && alloc1.maximumTimeMs == 500, "Fixed move time allocated");

    // 2. Explicit move time clamped to kMinMoveTimeMs
    TimeControl tcZeroMoveTime;
    tcZeroMoveTime.moveTimeMs = 0;
    TimeAllocation allocZero = TimeManager::allocate(tcZeroMoveTime);
    require(allocZero.optimalTimeMs == TimeManager::kMinMoveTimeMs, "Zero moveTime clamped to min");
    require(allocZero.maximumTimeMs == TimeManager::kMinMoveTimeMs, "Zero moveTime max clamped to min");

    // 3. Unbounded by clock (no remainingTimeMs and no moveTimeMs)
    TimeControl tcUnbounded;
    TimeAllocation allocUnbounded = TimeManager::allocate(tcUnbounded);
    require(allocUnbounded.optimalTimeMs == 0 && allocUnbounded.maximumTimeMs == 0, "Unbounded returns zero allocation");

    // 4. Normal clock with remaining time, increment, and movesToGo
    TimeControl tcClock;
    tcClock.remainingTimeMs = 60'000;
    tcClock.incrementMs = 1'000;
    tcClock.movesToGo = 30;

    TimeAllocation alloc2 = TimeManager::allocate(tcClock);
    require(alloc2.optimalTimeMs > 0, "Optimal time is positive");
    require(alloc2.maximumTimeMs >= alloc2.optimalTimeMs, "Maximum time >= optimal time");
    require(alloc2.maximumTimeMs < 60'000, "Maximum time does not exceed remaining clock");
    require(alloc2.maximumTimeMs + TimeManager::kHardReserveMs <= 60'000, "Hard reserve maintained");

    // 5. Emergency threshold reachability & behavior
    // For clock <= 1500 ms (emergency threshold), panic allocation executes without increment bonus.
    {
        TimeControl tcEmerg;
        tcEmerg.remainingTimeMs = 1'000;
        tcEmerg.incrementMs = 2'000;
        const TimeAllocation aEmerg = TimeManager::allocate(tcEmerg);
        require(aEmerg.optimalTimeMs > 0, "Emergency optimal > 0");
        require(aEmerg.maximumTimeMs >= aEmerg.optimalTimeMs, "Emergency max >= optimal");
        // Usable clock is 1000 - 100 = 900 ms. 30% is 270 ms.
        require(aEmerg.maximumTimeMs == 270, "Emergency uses exactly 30% of usable clock");
        require(aEmerg.maximumTimeMs + TimeManager::kHardReserveMs <= 1'000, "Emergency preserves hard reserve");
    }

    // 6. Hard reserve floor boundary: clock <= 100 ms returns kMinMoveTimeMs
    {
        for (std::uint32_t rem : {100U, 50U, 10U, 1U, 0U}) {
            TimeControl tcFloor;
            tcFloor.remainingTimeMs = rem;
            tcFloor.incrementMs = 2'000;
            const TimeAllocation aFloor = TimeManager::allocate(tcFloor);
            require(aFloor.optimalTimeMs == TimeManager::kMinMoveTimeMs, "Floor optimal == kMinMoveTimeMs");
            require(aFloor.maximumTimeMs == TimeManager::kMinMoveTimeMs, "Floor maximum == kMinMoveTimeMs");
        }
    }

    // 7. Full 3+2 blitz schedule audit matrix:
    //    180s, 60s, 30s, 10s, 5s, 2s, 1s, 500ms, 100ms, 50ms, 1ms with 2s increment
    {
        const struct Case {
            std::uint32_t rem;
            std::uint32_t inc;
        } cases[] = {
            {180'000, 2'000},
            {60'000,  2'000},
            {30'000,  2'000},
            {10'000,  2'000},
            {5'000,   2'000},
            {2'000,   2'000},
            {1'000,   2'000},
            {500,     2'000},
            {100,     2'000},
            {50,      2'000},
            {1,       2'000},
        };

        for (const auto& c : cases) {
            TimeControl tc;
            tc.remainingTimeMs = c.rem;
            tc.incrementMs = c.inc;
            const TimeAllocation a = TimeManager::allocate(tc);

            require(a.optimalTimeMs >= TimeManager::kMinMoveTimeMs, "Optimal is always positive (>= 1ms)");
            require(a.maximumTimeMs >= a.optimalTimeMs, "Invariant max >= optimal holds across 3+2 matrix");
            require(a.maximumTimeMs <= c.rem, "Maximum never exceeds remaining clock");

            if (c.rem > TimeManager::kHardReserveMs) {
                require(a.maximumTimeMs + TimeManager::kHardReserveMs <= c.rem,
                        "Hard reserve (100ms) strictly maintained for clocks > 100ms");
            }
            if (c.rem >= 2'000) {
                // In standard mode, maximum must strictly honor the 20% remaining clock cap
                require(a.maximumTimeMs <= c.rem / 5U, "Standard mode honors 20% max-time cap");
            }
        }
    }

    // 8. Zero-increment and alternate-increment tests
    {
        for (std::uint32_t inc : {0U, 500U, 1'000U, 5'000U}) {
            for (std::uint32_t rem : {180'000U, 60'000U, 5'000U, 1'000U}) {
                TimeControl tc;
                tc.remainingTimeMs = rem;
                tc.incrementMs = inc;
                const TimeAllocation a = TimeManager::allocate(tc);
                require(a.maximumTimeMs >= a.optimalTimeMs, "Max >= optimal for variable increments");
                if (rem > TimeManager::kHardReserveMs) {
                    require(a.maximumTimeMs + TimeManager::kHardReserveMs <= rem,
                            "Hard reserve preserved for variable increments");
                }
            }
        }
    }

    // 9. movesToGo scaling behavior
    {
        TimeControl tc;
        tc.remainingTimeMs = 60'000;
        tc.incrementMs = 0;

        tc.movesToGo = 10;
        const auto a10 = TimeManager::allocate(tc);

        tc.movesToGo = 40;
        const auto a40 = TimeManager::allocate(tc);

        require(a10.optimalTimeMs > a40.optimalTimeMs, "Fewer movesToGo allocates more time per move");
        require(a10.maximumTimeMs <= 60'000 / 5U, "20% cap respected even with 10 movesToGo");
    }

    // 10. Large clock values (prevent overflow)
    {
        TimeControl tc;
        tc.remainingTimeMs = 3'600'000; // 1 hour
        tc.incrementMs = 30'000;       // 30 seconds
        const TimeAllocation a = TimeManager::allocate(tc);
        require(a.optimalTimeMs > 0, "Large clock produces positive allocation");
        require(a.maximumTimeMs >= a.optimalTimeMs, "Large clock max >= optimal");
        require(a.maximumTimeMs <= 3'600'000 / 5U, "Large clock honors 20% cap");
        require(a.maximumTimeMs + TimeManager::kHardReserveMs <= 3'600'000, "Large clock honors hard reserve");
    }
}

void testSearchLimitsControls() {
    auto pos = Position::startPosition();
    MoveList legalMoves;
    movegen::generateLegalMoves(pos, legalMoves);

    // 1. Fixed maxNodes limit
    search::SearchLimits nodeLimits;
    nodeLimits.depth = 64;
    nodeLimits.maxNodes = 200;

    auto nodeRes = search::search(pos, nodeLimits);
    require(nodeRes.bestMove.has_value(), "Node-limited search returns a move");
    require(legalMoves.contains(*nodeRes.bestMove), "Node-limited move is legal");

    // 2. Fixed moveTimeMs limit
    search::SearchLimits timeLimits;
    timeLimits.depth = 64;
    timeLimits.moveTimeMs = 25;

    auto timeRes = search::search(pos, timeLimits);
    require(timeRes.bestMove.has_value(), "Time-limited search returns a move");
    require(legalMoves.contains(*timeRes.bestMove), "Time-limited move is legal");
}

void testIterativeDeepeningDeterminism() {
    auto pos = Position::startPosition();
    search::SearchLimits limits(3);

    auto res1 = search::search(pos, limits);
    auto res2 = search::search(pos, limits);

    require(res1.bestMove.has_value() && res2.bestMove.has_value(), "Both return best moves");
    require(res1.bestMove->encoded() == res2.bestMove->encoded(), "Best moves match deterministically");
    require(res1.score == res2.score, "Scores match deterministically");
    require(res1.completedDepth == res2.completedDepth, "Completed depths match");
}

void testSearchOptionsIsolation() {
    auto posOpt = Position::fromFen("r1bqk2r/pppp1ppp/2n5/4p3/1b1Pn3/2N2N2/PPP1PPPP/R1BQKB1R w KQkq - 2 6");
    require(posOpt.hasValue(), "Tactical FEN parses");
    auto pos = *posOpt;

    search::SearchLimits limits(3);

    // 1. Vanilla (plain alpha-beta)
    auto vanillaOpts = search::SearchOptions::vanilla();
    auto vanillaRes = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateSideToMove(p);
    }, nullptr, nullptr, vanillaOpts);

    // 2. Full options (all enhancements enabled)
    search::SearchOptions fullOpts;
    auto fullRes = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateSideToMove(p);
    }, nullptr, nullptr, fullOpts);

    require(vanillaRes.bestMove.has_value(), "Vanilla search returns best move");
    require(fullRes.bestMove.has_value(), "Enhanced search returns best move");
    // Enhanced search with PVS/NMP/LMR/killers explores a more pruned tree
    require(fullRes.stats.nodes <= vanillaRes.stats.nodes, "Enhanced search prunes effectively");

    std::cout << "Search options isolation: Vanilla nodes = " << vanillaRes.stats.nodes
              << ", Enhanced nodes = " << fullRes.stats.nodes << '\n';
}

void testAspirationWindowsFailSoftWidening() {
    auto pos = Position::startPosition();
    search::SearchLimits limits(3);

    // With aspiration window enabled, result must match or be closely consistent with vanilla
    search::SearchOptions aspOnly = search::SearchOptions::vanilla();
    aspOnly.aspirationWindows = true;
    aspOnly.aspirationDelta = 25;  // narrow delta to ensure widening path is exercised

    auto aspRes = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateSideToMove(p);
    }, nullptr, nullptr, aspOnly);

    require(aspRes.bestMove.has_value(), "Aspiration window search finds best move");
    require(aspRes.completedDepth == 3, "Aspiration search completes requested depth");
}

void testNullMovePruningSafety() {
    // Position with sufficient pieces where null-move pruning can trigger
    auto posOpt = Position::fromFen("r1bqk2r/pppp1ppp/2n5/4p3/1b1Pn3/2N2N2/PPP1PPPP/R1BQKB1R w KQkq - 2 6");
    require(posOpt.hasValue(), "FEN parses");
    auto pos = *posOpt;

    search::SearchLimits limits(4);

    search::SearchOptions nmpOpts = search::SearchOptions::vanilla();
    nmpOpts.nullMovePruning = true;

    auto nmpRes = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateSideToMove(p);
    }, nullptr, nullptr, nmpOpts);

    require(nmpRes.bestMove.has_value(), "NMP search finds best move");
    require(nmpRes.completedDepth == 4, "NMP search completes requested depth");
}

void testKillerMoveOrdering() {
    // Quiet tactical position
    auto posOpt = Position::fromFen("r1bq1rk1/ppp2ppp/2np1n2/4p3/2B1P3/2NP1N2/PPP2PPP/R1BQ1RK1 w - - 0 7");
    require(posOpt.hasValue(), "Quiet tactical FEN parses");
    auto pos = *posOpt;

    search::SearchLimits limits(3);

    search::SearchOptions killerOpts = search::SearchOptions::vanilla();
    killerOpts.killerMoves = true;

    auto killerRes = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateSideToMove(p);
    }, nullptr, nullptr, killerOpts);

    require(killerRes.bestMove.has_value(), "Killer heuristic search finds best move");
    require(killerRes.completedDepth == 3, "Killer heuristic search completes requested depth");
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
    testQuiescenceTacticalHorizon();
    testQuiescenceCheckHandling();
    testIterativeDeepeningAndFallbackMove();
    testCooperativeStopToken();
    testTimeManagerAllocation();
    testSearchLimitsControls();
    testIterativeDeepeningDeterminism();
    testSearchOptionsIsolation();
    testAspirationWindowsFailSoftWidening();
    testNullMovePruningSafety();
    testKillerMoveOrdering();

    std::cout << "All Iterative Deepening, Quiescence, Time Control, and Search tests passed successfully.\n";
    return EXIT_SUCCESS;
}
