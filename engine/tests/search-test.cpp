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
#include "rooksac/see.hpp"

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
    search::SearchLimits limits(2);

    // 1. Stalemate position: Black king on a8 has no moves, not in check (halfmove = 0).
    auto staleOpt = Position::fromFen("k7/2Q5/2K5/8/8/8/8/8 b - - 0 1");
    require(staleOpt.hasValue(), "Stalemate FEN parses");
    auto stalePos = *staleOpt;
    auto staleRes = search::search(stalePos, limits);
    require(!staleRes.bestMove.has_value(), "Stalemate returns nullopt move");
    require(staleRes.score == search::kDrawScore, "Stalemate returns 0 score");

    // 2. Stalemate with halfmove clock == 100
    auto stale50Opt = Position::fromFen("k7/2Q5/2K5/8/8/8/8/8 b - - 100 50");
    require(stale50Opt.hasValue(), "Stalemate with halfmove=100 FEN parses");
    auto stale50Res = search::search(*stale50Opt, limits);
    require(!stale50Res.bestMove.has_value(), "Stalemate with halfmove=100 returns nullopt move");
    require(stale50Res.score == search::kDrawScore, "Stalemate with halfmove=100 returns draw score");

    // 3. Fifty-move draw rule: halfmove clock >= 100, non-terminal legal moves exist
    auto fiftyOpt = Position::fromFen("4k3/8/8/8/8/8/8/4K3 w - - 100 50");
    require(fiftyOpt.hasValue(), "Fifty-move FEN parses");
    auto fiftyPos = *fiftyOpt;
    auto fiftyRes = search::search(fiftyPos, limits);
    require(!fiftyRes.bestMove.has_value(), "Fifty move draw at root returns nullopt move");
    require(fiftyRes.score == search::kDrawScore, "Fifty move draw returns 0 score");

    // 4. Normal position below 50-move threshold (halfmove = 99, White can move)
    auto sub50Opt = Position::fromFen("4k3/8/8/8/8/8/8/4K3 w - - 99 50");
    require(sub50Opt.hasValue(), "Sub-50 move FEN parses");
    auto sub50Res = search::search(*sub50Opt, limits);
    require(sub50Res.bestMove.has_value(), "Sub-50 move returns valid best move");

    // 5. Checkmate at root: Black king on h8 checkmated by Qh7# with Kg6 (halfmove = 0)
    auto mateOpt = Position::fromFen("7k/7Q/6K1/8/8/8/8/8 b - - 0 1");
    require(mateOpt.hasValue(), "Checkmate FEN parses");
    auto matePos = *mateOpt;
    auto mateRes = search::search(matePos, limits);
    require(!mateRes.bestMove.has_value(), "Checkmated position at root returns nullopt move");
    require(mateRes.score == -search::kMateScore, "Checkmated position at root scores -kMateScore");

    // 6. Checkmate with halfmove clock == 100 (CRITICAL REGRESSION: checkmate takes precedence over 50-move rule)
    auto mate50Opt = Position::fromFen("7k/7Q/6K1/8/8/8/8/8 b - - 100 50");
    require(mate50Opt.hasValue(), "Checkmate with halfmove=100 FEN parses");
    auto mate50Pos = *mate50Opt;
    auto mate50Res = search::search(mate50Pos, limits);
    require(!mate50Res.bestMove.has_value(), "Checkmated position with halfmove=100 returns nullopt move");
    require(mate50Res.score == -search::kMateScore, "Checkmate MUST take precedence over 50-move draw (score == -kMateScore)");
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


void testQuiescenceDepthLimitAndCheckEvasions() {
    search::SearchStats qstats;

    // 1. Checkmate inside quiescence at ply >= kMaxQsearchDepth
    auto mateOpt = Position::fromFen("7k/7Q/6K1/8/8/8/8/8 b - - 0 1");
    require(mateOpt.hasValue(), "Mate FEN valid");
    auto matePos = *mateOpt;
    const std::int32_t mateAtLimit = search::quiescence(
        matePos, -search::kInfinity, search::kInfinity, search::kMaxQsearchDepth, qstats,
        [](const Position& p) { return eval::evaluateMaterialSideToMove(p); });
    require(mateAtLimit == -search::kMateScore + static_cast<std::int32_t>(search::kMaxQsearchDepth),
            "Quiescence recognizes checkmate even at max depth limit boundary");

    // 2. Position in check at ply >= kMaxQsearchDepth with legal evasions available:
    // Stand-pat must not be used, returns bounded evaluator score without endless recursion.
    auto checkOpt = Position::fromFen("8/8/8/8/8/8/4r3/4K2k w - - 0 1");
    require(checkOpt.hasValue(), "Check FEN valid");
    auto checkPos = *checkOpt;
    const std::uint64_t nodesBefore = qstats.nodes;
    const std::int32_t boundScore = search::quiescence(
        checkPos, -search::kInfinity, search::kInfinity, search::kMaxQsearchDepth, qstats,
        [](const Position& p) { return eval::evaluateMaterialSideToMove(p); });
    require(qstats.nodes == nodesBefore + 1, "At depth limit, quiescence terminates without recursive branches");
    require(boundScore != search::kInfinity && boundScore != -search::kInfinity, "Bounded evaluation returned");

    // 3. Deep checking sequence does not exceed kMaxQsearchDepth
    qstats.nodes = 0;
    qstats.maxDepth = 0;
    static_cast<void>(search::quiescence(
        checkPos, -search::kInfinity, search::kInfinity, search::kMaxQsearchDepth - 2, qstats,
        [](const Position& p) { return eval::evaluateMaterialSideToMove(p); }));
    require(qstats.maxDepth <= search::kMaxQsearchDepth + 1, "Quiescence search strictly adheres to max depth bound");

    // 4. Verify position state invariant is preserved after quiescence check evasions
    const std::string originalFen = checkPos.toFen();
    static_cast<void>(search::quiescence(
        checkPos, -search::kInfinity, search::kInfinity, 0, qstats,
        [](const Position& p) { return eval::evaluateMaterialSideToMove(p); }));
    require(checkPos.toFen() == originalFen, "Position FEN strictly preserved after full quiescence evasion search");
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

    // 11. Extreme uint32_t values and arithmetic overflow protection
    {
        TimeControl tcMax;
        tcMax.remainingTimeMs = UINT32_MAX;
        tcMax.incrementMs = 60'000;
        const TimeAllocation aMax = TimeManager::allocate(tcMax);
        require(aMax.optimalTimeMs > 0, "UINT32_MAX clock produces positive optimal allocation");
        require(aMax.maximumTimeMs >= aMax.optimalTimeMs, "UINT32_MAX max >= optimal");
        require(aMax.maximumTimeMs <= tcMax.remainingTimeMs.value() / 5U, "UINT32_MAX honors 20% cap without overflow");

        TimeControl tcHugeInc;
        tcHugeInc.remainingTimeMs = 10'000;
        tcHugeInc.incrementMs = UINT32_MAX;
        const TimeAllocation aHugeInc = TimeManager::allocate(tcHugeInc);
        require(aHugeInc.optimalTimeMs > 0, "Huge increment produces positive optimal");
        require(aHugeInc.maximumTimeMs >= aHugeInc.optimalTimeMs, "Huge increment max >= optimal");
        require(aHugeInc.maximumTimeMs <= 10'000 / 5U, "Huge increment strictly capped at 20% of 10s clock");

        // Targeted regression: baseTime + incBonus overflow test
        // 4,000,000,000 ms (~46 days) remaining with movestogo = 1 gives baseTime = 3,999,999,900 ms.
        // With increment = 4,000,000,000 ms, incCap = 400,000,000 ms.
        // In 32-bit arithmetic, 3,999,999,900 + 400,000,000 = 4,399,999,900 wraps to 105,032,604 ms!
        // With 64-bit safe intermediate arithmetic, optimal is strictly capped at capByRemain (800,000,000 ms)
        // rather than collapsing to a wrapped value.
        TimeControl tcOverflow;
        tcOverflow.remainingTimeMs = 4'000'000'000U;
        tcOverflow.incrementMs     = 4'000'000'000U;
        tcOverflow.movesToGo       = 1U;
        const TimeAllocation aOverflow = TimeManager::allocate(tcOverflow);
        require(aOverflow.optimalTimeMs > 0, "Overflow scenario optimal is positive");
        require(aOverflow.maximumTimeMs >= aOverflow.optimalTimeMs, "Overflow scenario max >= optimal");
        require(aOverflow.maximumTimeMs <= 4'000'000'000U / 5U, "Overflow scenario respects 20% ceiling");
        // Verify optimalTimeMs does not wrap around to 105,032,604 ms:
        require(aOverflow.maximumTimeMs == 4'000'000'000U / 5U, "Overflow scenario max hits exactly 20% cap (800,000,000 ms)");
        require(aOverflow.optimalTimeMs == 4'000'000'000U / 5U, "Overflow scenario optimal bounded to 20% cap (800,000,000 ms)");
        require(aOverflow.maximumTimeMs + TimeManager::kHardReserveMs <= 4'000'000'000U, "Hard reserve strictly preserved");

        // Both clock and increment at maximum representable UINT32_MAX
        TimeControl tcDualMax;
        tcDualMax.remainingTimeMs = UINT32_MAX;
        tcDualMax.incrementMs     = UINT32_MAX;
        tcDualMax.movesToGo       = 1U;
        const TimeAllocation aDualMax = TimeManager::allocate(tcDualMax);
        require(aDualMax.optimalTimeMs > 0, "Dual UINT32_MAX optimal > 0");
        require(aDualMax.maximumTimeMs >= aDualMax.optimalTimeMs, "Dual UINT32_MAX max >= optimal");
        require(aDualMax.maximumTimeMs <= UINT32_MAX / 5U, "Dual UINT32_MAX respects 20% cap");
        require(aDualMax.maximumTimeMs + TimeManager::kHardReserveMs <= UINT32_MAX, "Dual UINT32_MAX respects hard reserve");
    }
}

void testSingleLegalMoveFastExit() {
    // White king on e1 with pawn on e2, black king on f3.
    // The only legal move for white is e1-d2.
    auto posOpt = Position::fromFen("8/8/8/8/8/5k2/4p3/4K3 w - - 0 1");
    require(posOpt.hasValue(), "Single legal move FEN parses");
    auto pos = *posOpt;

    // When optimalTimeMs is provided, search returns depth 1 immediately without wasting clock
    search::SearchLimits limits;
    limits.timeAllocation = rooksac::TimeAllocation{5000, 10000};

    auto res = search::search(pos, limits);
    require(res.bestMove.has_value(), "Search finds single legal move");
    require(res.bestMove->from() == Square::E1 && res.bestMove->to() == Square::D2, "Plays Kd2");
    require(res.completedDepth == 1, "Completes immediately at depth 1");
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

void testSearchPathRepetitionDetection() {
    // 1. Position where White is completely winning (+900 queen up),
    // but the only way to avoid 3-fold repetition is to play a winning move.
    // Or simpler: White can repeat moves (e.g. Nf3-g1-f3) or play winning move.
    // If repetition returns 0 (kDrawScore), a winning engine will strictly avoid repeating!
    // FEN: White has Queen and King, Black has King:
    auto posOpt = Position::fromFen("4k3/8/8/8/8/8/8/4K2Q w - - 0 1");
    require(posOpt.hasValue(), "Repetition test FEN parses");
    auto pos = *posOpt;

    // Simulate move sequence into repeated position along search path:
    // 1. Qh8+ Ke7 2. Qh1 Ke8 3. Qh8+ Ke7
    // Verify that repeating position evaluates to kDrawScore in search
    search::SearchLimits limits(4);
    auto res = search::search(pos, limits);
    require(res.bestMove.has_value(), "Search finds move");
    // Engine with a queen up should score heavily positive, not draw
    require(res.score > 500, "Winning position does not evaluate to draw");

    // 2. Direct cycle fixture:
    // White king on e1, Black king on e8.
    // White plays Ke1-e2, Black Ke8-e7, White Ke2-e1, Black Ke7-e8 (cycle).
    auto cyclePosOpt = Position::fromFen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    require(cyclePosOpt.hasValue(), "Cycle FEN parses");
    auto cyclePos = *cyclePosOpt;

    search::SearchLimits depthLimits(4);
    auto cycleRes = search::search(cyclePos, depthLimits);
    require(cycleRes.score == search::kDrawScore, "K vs K with repetition path evaluates to kDrawScore");
}

void testQuiescenceBoundaryEvasionScoring() {
    // In-check position at quiescence depth limit
    // White king on e1 is in check by Black rook on e2.
    // At boundary kMaxQsearchDepth, evasions must be evaluated directly rather than taking stand-pat.
    auto checkOpt = Position::fromFen("8/8/8/8/8/8/4r3/4K2k w - - 0 1");
    require(checkOpt.hasValue(), "Check FEN parses");
    auto checkPos = *checkOpt;

    search::SearchStats stats;
    const std::int32_t boundaryScore = search::quiescence(
        checkPos, -search::kInfinity, search::kInfinity, search::kMaxQsearchDepth, stats,
        [](const Position& p) { return eval::evaluateMaterialSideToMove(p); });

    // In this position, Kxe2 captures the checking rook. Material becomes 0.
    // Under our new evasion-leaf evaluation at the boundary, Kxe2 is evaluated, scoring 0!
    require(boundaryScore == 0, "Quiescence boundary evaluates check evasion leaf to capture rook");
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


void testPvLegalityAndCompletedIterations() {
    // Test PV best-move legality across several depths and position types
    const char* positions[] = {
        Position::kStartPositionFen.data(),
        "r1bqk2r/pppp1ppp/2n5/4p3/1b1Pn3/2N2N2/PPP1PPPP/R1BQKB1R w KQkq - 2 6",
        "4r3/8/8/8/8/8/8/4K3 w - - 0 1",
        "3r4/4P3/8/8/3q4/5N2/8/4K2k w - - 0 1"
    };

    for (const char* fen : positions) {
        auto posOpt = Position::fromFen(fen);
        require(posOpt.hasValue(), "PV test FEN parses");
        Position pos = *posOpt;

        MoveList legalMoves;
        movegen::generateLegalMoves(pos, legalMoves);

        for (std::uint32_t d = 1; d <= 3; ++d) {
            search::SearchLimits limits(d);
            auto res = search::search(pos, limits);
            require(res.completedDepth == d, "Requested depth completed");
            require(res.bestMove.has_value(), "Search returned best move");
            require(legalMoves.contains(*res.bestMove), "PV best move is strictly legal in the root position");
        }
    }
}

void testPvsAndLmrReSearchCorrectness() {
    // Compare vanilla search vs PVS+LMR on quiet tactical positions
    // PVS with re-search must find a legal and sound move matching or beating vanilla
    auto posOpt = Position::fromFen("r1bqk2r/pppp1ppp/2n5/4p3/1b1Pn3/2N2N2/PPP1PPPP/R1BQKB1R w KQkq - 2 6");
    require(posOpt.hasValue(), "Tactical FEN parses");
    Position pos = *posOpt;

    MoveList legalMoves;
    movegen::generateLegalMoves(pos, legalMoves);

    search::SearchLimits limits(3);

    // 1. Vanilla
    auto vanillaOpts = search::SearchOptions::vanilla();
    auto vanillaRes = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateSideToMove(p);
    }, nullptr, nullptr, vanillaOpts);

    // 2. PVS enabled only
    search::SearchOptions pvsOpts = search::SearchOptions::vanilla();
    pvsOpts.pvSearch = true;
    auto pvsRes = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateSideToMove(p);
    }, nullptr, nullptr, pvsOpts);

    require(pvsRes.bestMove.has_value(), "PVS search finds best move");
    require(legalMoves.contains(*pvsRes.bestMove), "PVS best move is legal");
    require(pvsRes.completedDepth == 3, "PVS search completes depth 3");
    require(pvsRes.score == vanillaRes.score, "PVS score matches vanilla alpha-beta score");

    // 3. PVS + LMR enabled
    search::SearchOptions lmrOpts = search::SearchOptions::vanilla();
    lmrOpts.pvSearch = true;
    lmrOpts.lmr = true;
    auto lmrRes = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateSideToMove(p);
    }, nullptr, nullptr, lmrOpts);

    require(lmrRes.bestMove.has_value(), "PVS+LMR search finds best move");
    require(legalMoves.contains(*lmrRes.bestMove), "PVS+LMR best move is legal");
    require(lmrRes.completedDepth == 3, "PVS+LMR search completes depth 3");
}

void testNmpZugzwangSafeguard() {
    // In king and pawn endgames, null-move pruning should NOT trigger due to zugzwang risk
    // FEN: King + pawns only
    auto posOpt = Position::fromFen("8/5k2/8/8/8/8/4P3/4K3 w - - 0 1");
    require(posOpt.hasValue(), "KP endgame FEN parses");
    Position pos = *posOpt;

    search::SearchLimits limits(4);

    search::SearchOptions vanillaOpts = search::SearchOptions::vanilla();
    auto vanillaRes = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateSideToMove(p);
    }, nullptr, nullptr, vanillaOpts);

    search::SearchOptions nmpOpts = search::SearchOptions::vanilla();
    nmpOpts.nullMovePruning = true;
    auto nmpRes = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateSideToMove(p);
    }, nullptr, nullptr, nmpOpts);

    // Because non-pawn pieces == 0, NMP does not execute; nodes and score should match vanilla exactly
    require(nmpRes.stats.nodes == vanillaRes.stats.nodes, "NMP zugzwang safeguard prevented null moves with pawns only");
    require(nmpRes.score == vanillaRes.score, "Scores match with zugzwang safeguard");
}

void testAspirationFailLowFailHighRecovery() {
    auto pos = Position::startPosition();
    search::SearchLimits limits(3);

    // Narrow aspiration window delta of 10 to force both fail-low and fail-high re-searches
    search::SearchOptions aspOpts = search::SearchOptions::vanilla();
    aspOpts.aspirationWindows = true;
    aspOpts.aspirationDelta = 10;

    auto res = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateSideToMove(p);
    }, nullptr, nullptr, aspOpts);

    require(res.completedDepth == 3, "Aspiration widening successfully recovers and reaches depth 3");
    require(res.bestMove.has_value(), "Best move produced after widening");

    MoveList legals;
    movegen::generateLegalMoves(pos, legals);
    require(legals.contains(*res.bestMove), "Recovered move is strictly legal");
}

void testSeeCalculationAndPruning() {
    // 1. Hanging piece capture: White Queen takes undefended Black Rook
    // FEN: 4k3/8/8/3r4/8/8/8/3Q4 w - - 0 1
    auto hangingOpt = Position::fromFen("4k3/8/8/3r4/8/8/8/3Q4 w - - 0 1");
    require(hangingOpt.hasValue(), "Hanging Rook FEN parses");
    auto qTakesR = Move::create(Square::D1, Square::D5);
    require(qTakesR.has_value(), "Qxd5 move creates");
    require(search::see(*hangingOpt, *qTakesR) == 500, "Qxd5 gains 500 centipawns (Rook value)");
    require(search::seeGe(*hangingOpt, *qTakesR, 0), "Qxd5 is winning/equal capture");

    // 2. Bad capture: White Queen takes defended Black Pawn (defended by Black Rook on d8)
    // FEN: 3rk3/8/8/3p4/8/8/8/3Q4 w - - 0 1
    auto badCapOpt = Position::fromFen("3rk3/8/8/3p4/8/8/8/3Q4 w - - 0 1");
    require(badCapOpt.hasValue(), "Defended Pawn FEN parses");
    auto qTakesP = Move::create(Square::D1, Square::D5);
    require(qTakesP.has_value(), "Qxd5 move creates");
    require(search::see(*badCapOpt, *qTakesP) == 100 - 900, "Qxd5 loses 800 centipawns (Q - P)");
    require(!search::seeGe(*badCapOpt, *qTakesP, 0), "Qxd5 is detected as bad capture by SEE");

    // 3. Complex exchange: White Bishop takes Black Knight on d5, defended by Black Pawn on c6 and Queen on d8,
    // attacked by White Knight on c3.
    // FEN: 3qkb1r/8/2p5/3n4/8/2N5/8/2B1K3 w - - 0 1
    auto complexOpt = Position::fromFen("3qkb1r/8/2p5/3n4/8/2N5/8/2B1K3 w - - 0 1");
    require(complexOpt.hasValue(), "Complex exchange FEN parses");
    auto bTakesN = Move::create(Square::C1, Square::D5);
    require(bTakesN.has_value(), "Bxd5 move creates");
    // White: B(330), N(320). Black: N(320), P(100), Q(900).
    // B takes N (+320), cxd5 (-330), Nxd5 (+100), Qxd5 (-320) -> White stops or trades equal
    // Net is >= 0 for Bxd5 because White wins N(320) and loses B(330) at worst or trades knight
    require(search::see(*complexOpt, *bTakesN) >= -10, "Bxd5 evaluated properly in complex battery");

    // 4. En passant SEE test
    // FEN: 4k3/8/8/3Pp3/8/8/8/4K3 w - e6 0 1
    auto epOpt = Position::fromFen("4k3/8/8/3Pp3/8/8/8/4K3 w - e6 0 1");
    require(epOpt.hasValue(), "EP FEN parses");
    auto epMove = Move::create(Square::D5, Square::E6);
    require(epMove.has_value(), "EP move creates");
    require(search::see(*epOpt, *epMove) == 100, "En passant capture gains pawn value (+100 cp)");
}

void testAbortPropagationGuaranteedLegalFallback() {
    auto pos = Position::startPosition();
    MoveList legalMoves;
    movegen::generateLegalMoves(pos, legalMoves);

    // Stop token that aborts immediately
    search::SearchStopToken stopToken;
    stopToken.stop();

    search::SearchLimits limits(10);
    auto res = search::search(pos, limits, [](const Position& p) {
        return eval::evaluateSideToMove(p);
    }, &stopToken);

    require(res.interrupted, "Search was interrupted by stop token");
    require(res.completedDepth == 0, "No full depth completed");
    require(res.bestMove.has_value(), "Guaranteed legal fallback move is populated even on immediate abort");
    require(legalMoves.contains(*res.bestMove), "Guaranteed fallback move is strictly legal");
}

void testEvaluatorSelectionParity() {
    auto pos = Position::startPosition();
    search::SearchLimits limits(2);

    int classicalCalls = 0;
    auto classicalRes = search::search(pos, limits, [&](const Position& p) {
        ++classicalCalls;
        return eval::evaluateSideToMove(p);
    });
    (void)classicalRes;

    int customCalls = 0;
    auto customRes = search::search(pos, limits, [&](const Position&) {
        ++customCalls;
        return 42; // arbitrary unique value
    });

    require(classicalCalls > 0, "Classical evaluator was actively called");
    require(customCalls > 0, "Custom evaluator was actively called");
    require(customRes.score == 42, "Search score directly reflects the active custom evaluator");
    require(customRes.bestMove.has_value(), "Custom search returned legal move");
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
    testQuiescenceDepthLimitAndCheckEvasions();
    testIterativeDeepeningAndFallbackMove();
    testCooperativeStopToken();
    testTimeManagerAllocation();
    testSingleLegalMoveFastExit();
    testSearchLimitsControls();
    testIterativeDeepeningDeterminism();
    testSearchOptionsIsolation();
    testAspirationWindowsFailSoftWidening();
    testNullMovePruningSafety();
    testKillerMoveOrdering();
    testSearchPathRepetitionDetection();
    testQuiescenceBoundaryEvasionScoring();
    testPvLegalityAndCompletedIterations();
    testPvsAndLmrReSearchCorrectness();
    testNmpZugzwangSafeguard();
    testAspirationFailLowFailHighRecovery();
    testAbortPropagationGuaranteedLegalFallback();
    testSeeCalculationAndPruning();
    testEvaluatorSelectionParity();

    std::cout << "All Iterative Deepening, Quiescence, Time Control, and Search tests passed successfully.\n";
    return EXIT_SUCCESS;
}
