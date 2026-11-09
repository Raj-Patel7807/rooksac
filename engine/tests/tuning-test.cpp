#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "rooksac/eval_types.hpp"
#include "rooksac/tuning.hpp"

namespace {

using namespace rooksac;
using namespace rooksac::tuning;

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "Test failure: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void testSigmoidProperties() {
    // 1. Equal evaluation (0 cp) should yield exactly 50% win probability
    const double pZero = sigmoid(0.0, 1.0);
    require(std::abs(pZero - 0.5) < 1e-6, "sigmoid(0) == 0.5");

    // 2. Symmetry: sigmoid(x) + sigmoid(-x) == 1.0
    const double pPos = sigmoid(200.0, 1.0);
    const double pNeg = sigmoid(-200.0, 1.0);
    require(std::abs((pPos + pNeg) - 1.0) < 1e-6, "sigmoid(x) + sigmoid(-x) == 1.0");

    // 3. Monotonicity: higher eval -> higher win probability
    require(pPos > pZero, "Positive eval yields > 50% win probability");
    require(pNeg < pZero, "Negative eval yields < 50% win probability");
}

void testParameterJsonSerialization() {
    eval::EvalParameters params = eval::defaultParameters();
    params.bishopPair.mg = 35;
    params.bishopPair.eg = 55;

    const std::string json = serializeParametersToJson(params, "1.0.0");
    require(json.find("\"schemaVersion\": \"1.0.0\"") != std::string::npos, "Schema version preserved");
    require(json.find("\"bishopPair\"") != std::string::npos, "Contains bishopPair key");

    eval::EvalParameters deserialized;
    const bool success = deserializeParametersFromJson(json, deserialized);
    require(success, "Deserialization succeeds");
    require(deserialized.bishopPair.mg == 35, "Deserialized bishopPair.mg matches");
    require(deserialized.bishopPair.eg == 55, "Deserialized bishopPair.eg matches");

    // Test file persistence round-trip
    const std::string tempFile = "eval_params_test.json";
    require(saveParametersToFile(tempFile, params, "1.0.0"), "saveParametersToFile succeeds");
    eval::EvalParameters fileLoaded;
    require(loadParametersFromFile(tempFile, fileLoaded), "loadParametersFromFile succeeds");
    require(fileLoaded.bishopPair.mg == 35, "Loaded bishopPair.mg matches");
    require(fileLoaded.bishopPair.eg == 55, "Loaded bishopPair.eg matches");
    std::remove(tempFile.c_str());
}

void testMseCalculationOnFixture() {
    // 3 simple positions
    std::vector<TuningEntry> fixture = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 0.5, "game1"},
        {"rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1", 0.5, "game1"},
        {"4k3/8/8/8/8/8/8/4K2R w - - 0 1", 1.0, "game2"}  // White up a rook
    };

    const eval::EvalParameters params = eval::defaultParameters();
    const double mse = computeMeanSquaredError(fixture, params, 1.0);

    require(mse >= 0.0, "MSE is non-negative");
    require(mse < 1.0, "MSE is bounded for reasonable predictions");
}

void testSprtBoundaries() {
    SprtConfig config;
    config.elo0 = 0.0;
    config.elo1 = 5.0;

    // 1. All wins for candidate -> should quickly accept H1
    SprtResult resWins = evaluateSprt(50, 0, 0, config);
    require(resWins.decision == SprtDecision::AcceptH1, "Dominant wins trigger AcceptH1");

    // 2. All losses for candidate -> should quickly accept H0
    SprtResult resLosses = evaluateSprt(0, 50, 0, config);
    require(resLosses.decision == SprtDecision::AcceptH0, "Dominant losses trigger AcceptH0");

    // 3. Small sample with balanced result -> should continue
    SprtResult resEarly = evaluateSprt(5, 5, 10, config);
    require(resEarly.decision == SprtDecision::Continue, "Balanced early sample continues");
}

}  // namespace

int main() {
    testSigmoidProperties();
    testParameterJsonSerialization();
    testMseCalculationOnFixture();
    testSprtBoundaries();

    std::cout << "All tuning and SPRT tests passed successfully.\n";
    return EXIT_SUCCESS;
}
