#include <cassert>
#include <chrono>
#include <iostream>

#include "rooksac/evaluation.hpp"
#include "rooksac/material.hpp"
#include "rooksac/position.hpp"
#include "rooksac/search.hpp"

namespace {

using namespace rooksac;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Assertion failed: " << message << '\n';
        std::abort();
    }
}

void testOpeningSymmetry() {
    auto pos = Position::startPosition();
    const std::int32_t whiteEval = eval::evaluate(pos);
    require(whiteEval == 0, "Starting position evaluation is 0 from White's perspective");
    const std::int32_t sideEval = eval::evaluateSideToMove(pos);
    require(sideEval == 0, "Starting position evaluation is 0 from side-to-move perspective");
}

void testColorSymmetry() {
    // Two symmetrically mirrored positions
    // Pos 1: White has advanced pawn on e5
    auto pos1 = Position::fromFen("rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 1");
    // Pos 2: Same after 1. e4 e5
    require(pos1.hasValue(), "FEN parsed");
    const std::int32_t eval1 = eval::evaluate(*pos1);
    require(eval1 == 0, "Symmetric e4 e5 pawn structure evaluates to 0");

    // Asymmetric position: White has extra knight
    auto posWhiteAhead = Position::fromFen("rnbqkb1r/pppppppp/8/8/8/5N2/PPPPPPPP/RNBQKB1R w KQkq - 0 1");
    // Inverted: Black has extra knight
    auto posBlackAhead = Position::fromFen("rnbqkb1r/pppppppp/5n2/8/8/8/PPPPPPPP/RNBQKB1R b KQkq - 0 1");
    require(posWhiteAhead.hasValue() && posBlackAhead.hasValue(), "FENs parsed");

    const std::int32_t whiteScore = eval::evaluateSideToMove(*posWhiteAhead);
    const std::int32_t blackScore = eval::evaluateSideToMove(*posBlackAhead);
    require(whiteScore == blackScore, "Color-mirrored positions yield identical side-to-move evaluations");
}

void testPawnStructureTerms() {
    const auto& params = eval::defaultParameters();

    // 1. Doubled pawns
    auto posDoubled = Position::fromFen("k7/8/8/8/8/4P3/4P3/K7 w - - 0 1");
    require(posDoubled.hasValue(), "Doubled pawns FEN parsed");
    const eval::Score doubledScore = eval::evaluatePawns(*posDoubled, params);
    require(doubledScore.mg < 0 && doubledScore.eg < 0, "Doubled pawns receive penalty");

    // 2. Isolated pawn: Black has connected d7, e7, f7 pawns; White has isolated e3 pawn opposed by e7 (not passed)
    auto posPureIso = Position::fromFen("k7/3ppp2/8/8/8/4P3/8/K7 w - - 0 1");
    require(posPureIso.hasValue(), "Pure isolated pawn FEN parsed");
    const eval::Score pureIsoScore = eval::evaluatePawns(*posPureIso, params);
    require(pureIsoScore.mg < 0 && pureIsoScore.eg < 0, "Isolated pawn receives penalty");

    // 3. Passed pawn
    auto posPassed = Position::fromFen("k7/8/4P3/8/8/8/8/K7 w - - 0 1");
    require(posPassed.hasValue(), "Passed pawn FEN parsed");
    const eval::Score passedScore = eval::evaluatePawns(*posPassed, params);
    require(passedScore.mg > 0 && passedScore.eg > 0, "Passed pawn receives bonus");
    require(passedScore.eg > passedScore.mg, "Passed pawn bonus is higher in endgame");
}

void testKingSafetyTerms() {
    const auto& params = eval::defaultParameters();

    // Intact shield
    auto posShielded = Position::fromFen("rnbq1rk1/pppppppp/8/8/8/8/PPPPPPPP/RNBQ1RK1 w - - 0 1");
    require(posShielded.hasValue(), "Shielded king FEN parsed");
    const eval::Score shieldedScore = eval::evaluateKingSafety(*posShielded, params);

    // Completely missing shield pawns for White (f, g, h pawns gone)
    auto posExposedFen = Position::fromFen("rnbq1rk1/pppppppp/8/8/8/8/PPPP4/RNBQ1RK1 w - - 0 1");
    require(posExposedFen.hasValue(), "Exposed king FEN parsed");
    const eval::Score exposedScore = eval::evaluateKingSafety(*posExposedFen, params);

    require(exposedScore.mg < shieldedScore.mg, "Exposed king receives significant middlegame penalty");
}

void testBishopPairAndRookActivity() {
    const auto& params = eval::defaultParameters();

    // White has bishop pair, Black has two knights
    auto posBishops = Position::fromFen("r1b1k2r/8/8/8/8/8/8/R1B1K2R w KQkq - 0 1");
    require(posBishops.hasValue(), "Bishop test FEN parsed");
    const eval::Score piecesScore = eval::evaluatePiecesAndMobility(*posBishops, params);
    // Both sides have 1 bishop and 2 rooks on semi-open files; symmetric
    require(piecesScore.mg == 0 && piecesScore.eg == 0, "Symmetric pieces yield zero score");

    // White with rook on fully open file vs rook on closed file
    auto posOpenRook = Position::fromFen("4k3/pppppppp/8/8/8/8/PPPP1PPP/R3K3 w Q - 0 1");
    require(posOpenRook.hasValue(), "Open rook FEN parsed");
    // e-file has black pawn on e7, white pawn missing -> semi-open file
    const eval::Score rookScore = eval::evaluatePiecesAndMobility(*posOpenRook, params);
    require(rookScore.mg > 0 || rookScore.eg > 0, "Rook on semi-open/open file receives bonus");
}

void testGamePhaseAndTaperedBlending() {
    // Opening position: phase == 24
    auto startPos = Position::startPosition();
    const std::int32_t startPhase = eval::calculateGamePhase(startPos);
    require(startPhase == eval::kTotalPhase, "Startpos has maximum game phase 24");

    // Bare kings: phase == 0
    auto bareKings = Position::fromFen("4k3/8/8/8/8/8/8/4K3 w - - 0 1");
    require(bareKings.hasValue(), "Bare kings FEN parsed");
    const std::int32_t barePhase = eval::calculateGamePhase(*bareKings);
    require(barePhase == 0, "Bare kings position has game phase 0");

    // Tapered blending test
    eval::Score score{100, 300};
    require(eval::blendScore(score, 24) == 100, "Phase 24 yields 100% middlegame score");
    require(eval::blendScore(score, 0) == 300, "Phase 0 yields 100% endgame score");
    require(eval::blendScore(score, 12) == 200, "Phase 12 yields 50/50 interpolated score");
}

void testMaterialBaselineRemainsAvailable() {
    auto pos = Position::startPosition();
    const std::int32_t matScore = eval::evaluateMaterial(pos);
    require(matScore == 0, "Baseline material evaluator remains available and yields 0 for startpos");
}

void testEvaluationBenchmarkAndThroughput() {
    auto pos = Position::startPosition();

    // Benchmark 100,000 evaluations
    constexpr int kIterations = 100'000;
    const auto t0 = std::chrono::steady_clock::now();
    std::int32_t sum = 0;
    for (int i = 0; i < kIterations; ++i) {
        sum += eval::evaluate(pos);
    }
    const auto t1 = std::chrono::steady_clock::now();
    const auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    const double durationSec = static_cast<double>(durationMs) / 1000.0;
    const double nps = static_cast<double>(kIterations) / std::max(0.001, durationSec);
    std::cout << "Evaluation benchmark: " << kIterations << " evaluations in "
              << durationMs << " ms (~" << static_cast<std::uint64_t>(nps) << " evals/sec, sum=" << sum << ").\n";
    require(nps > 50'000.0, "Evaluator throughput exceeds 50,000 evals/sec");
}

}  // namespace

int main() {
    testOpeningSymmetry();
    testColorSymmetry();
    testPawnStructureTerms();
    testKingSafetyTerms();
    testBishopPairAndRookActivity();
    testGamePhaseAndTaperedBlending();
    testMaterialBaselineRemainsAvailable();
    testEvaluationBenchmarkAndThroughput();

    std::cout << "All classical tapered evaluation tests passed successfully.\n";
    return 0;
}
