#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>

#include "rooksac/legal_movegen.hpp"
#include "rooksac/neural_evaluator.hpp"
#include "rooksac/position.hpp"
#include "rooksac/search.hpp"

namespace {

using namespace rooksac;
using namespace rooksac::neural;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void testFeatureEncodingDimensionsAndPlanes() {
    auto posOpt = Position::fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    require(posOpt.hasValue(), "Startpos parses");
    const auto pos = *posOpt;

    const FeatureTensor tensor = encodePosition(pos);
    require(tensor.size() == kInputTensorSize, "Tensor size matches 18 * 64");

    // White pawn on a2 (index 8): should be 1.0 in plane 0
    require(tensor[0 * 64 + 8] == 1.0f, "White pawn plane has a2 active");
    // Black pawn on a7 (index 48): should be 1.0 in plane 6
    require(tensor[6 * 64 + 48] == 1.0f, "Black pawn plane has a7 active");
    // Plane 12 (Side to move) should be 1.0 for White
    require(tensor[12 * 64 + 0] == 1.0f, "Side to move plane is 1.0 for White");
}

void testValueCalibrationRoundtrip() {
    // Value = 0.0 -> Centipawns = 0
    require(valueToCentipawns(0.0f) == 0, "v=0.0 maps to 0 cp");

    // Symmetry: valueToCentipawns(v) == -valueToCentipawns(-v)
    const std::int32_t cpPos = valueToCentipawns(0.5f);
    const std::int32_t cpNeg = valueToCentipawns(-0.5f);
    require(cpPos == -cpNeg, "valueToCentipawns is symmetric around 0");
    require(cpPos > 0, "Positive value maps to positive centipawns");
}

void testNeuralEvaluatorSideToMovePerspective() {
    NeuralEvaluator evaluator;
    evaluator.loadModel();
    require(evaluator.isLoaded(), "Evaluator loads model");

    auto whiteOpt = Position::fromFen("7k/5ppp/8/8/8/8/8/3QK3 w - - 0 1");
    auto blackOpt = Position::fromFen("3qk3/8/8/8/8/8/5PPP/7K b - - 0 1");
    require(whiteOpt.hasValue() && blackOpt.hasValue(), "Positions parse");

    const std::int32_t whiteScore = evaluator.evaluateSideToMove(*whiteOpt);
    const std::int32_t blackScore = evaluator.evaluateSideToMove(*blackOpt);

    require(whiteScore == blackScore, "Evaluation is symmetric from side-to-move perspective");
}

void testSearchRunsWithNeuralEvaluator() {
    auto pos = Position::startPosition();
    NeuralEvaluator evaluator;
    evaluator.loadModel();

    search::SearchLimits limits(2);
    // Search using the native neural leaf evaluator
    const auto res = search::search(pos, limits, [&](const Position& p) {
        return evaluator.evaluateSideToMove(p);
    });

    require(res.bestMove.has_value(), "Search with neural evaluator returns a best move");
    require(res.completedDepth == 2, "Search completes requested depth 2");
}

void testPolicyOrderingOnlyRanksLegalMoves() {
    auto pos = Position::startPosition();
    MoveList legalMoves;
    movegen::generateLegalMoves(pos, legalMoves);
    const std::size_t countBefore = legalMoves.size();

    NeuralEvaluator evaluator;
    evaluator.loadModel();
    evaluator.orderMovesByPolicy(pos, legalMoves);

    require(legalMoves.size() == countBefore, "Move count strictly preserved after policy ordering");
    for (std::size_t i = 0; i < legalMoves.size(); ++i) {
        require(movegen::isLegalMove(pos, legalMoves[i]), "Every move remains strictly legal");
    }
}

}  // namespace

int main() {
    testFeatureEncodingDimensionsAndPlanes();
    testValueCalibrationRoundtrip();
    testNeuralEvaluatorSideToMovePerspective();
    testSearchRunsWithNeuralEvaluator();
    testPolicyOrderingOnlyRanksLegalMoves();

    std::cout << "All native neural evaluator tests passed successfully.\n";
    return EXIT_SUCCESS;
}
