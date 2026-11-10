#include <cassert>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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

void testGoldenPositionEncodingsMatchPython() {
    std::ifstream file("research/tests/golden_tensors.json");
    if (!file.is_open()) {
        file.open("../research/tests/golden_tensors.json");
    }
    require(file.is_open(), "golden_tensors.json file opens");

    std::stringstream buffer;
    buffer << file.rdbuf();
    const std::string content = buffer.str();

    // Parse simple JSON array containing name, fen, tensor_flat
    // Find each position block
    std::size_t pos = 0;
    std::size_t verifiedCount = 0;

    while ((pos = content.find("\"fen\": \"", pos)) != std::string::npos) {
        pos += 8;
        const std::size_t endFen = content.find('"', pos);
        require(endFen != std::string::npos, "Valid FEN closing quote");
        const std::string fen = content.substr(pos, endFen - pos);

        // Find tensor_flat: [ ... ]
        const std::size_t startArray = content.find("[", endFen);
        const std::size_t endArray = content.find("]", startArray);
        require(startArray != std::string::npos && endArray != std::string::npos, "Valid tensor array");

        const std::string arrayStr = content.substr(startArray + 1, endArray - startArray - 1);
        std::stringstream ss(arrayStr);
        std::vector<float> pythonTensor;
        pythonTensor.reserve(kInputTensorSize);
        std::string token;
        while (std::getline(ss, token, ',')) {
            // Trim whitespace
            token.erase(0, token.find_first_not_of(" \t\r\n"));
            token.erase(token.find_last_not_of(" \t\r\n") + 1);
            if (!token.empty()) {
                pythonTensor.push_back(std::stof(token));
            }
        }

        require(pythonTensor.size() == kInputTensorSize, "Python tensor has exactly 1152 elements");

        auto posOpt = Position::fromFen(fen);
        require(posOpt.hasValue(), "Golden FEN parses in C++");
        const FeatureTensor cppTensor = encodePosition(*posOpt);

        for (std::size_t i = 0; i < kInputTensorSize; ++i) {
            require(cppTensor[i] == pythonTensor[i], "C++ and Python tensors match exactly at index");
        }

        ++verifiedCount;
        pos = endArray + 1;
    }

    require(verifiedCount >= 15, "At least 15 golden positions verified against Python");
    std::cout << "Verified " << verifiedCount << " golden position encodings with 100% exact C++/Python match.\n";
}

void testActionEncodingBijectiveAndDistinctPromotions() {
    // 1. Check distinct promotion actions for White promotions (e7e8)
    const Square e7 = makeSquare(File::E, Rank::Seven);
    const Square e8 = makeSquare(File::E, Rank::Eight);
    const Square d8 = makeSquare(File::D, Rank::Eight);

    const auto moveQ = *Move::create(e7, e8, PieceType::Queen);
    const auto moveN = *Move::create(e7, e8, PieceType::Knight);
    const auto moveB = *Move::create(e7, e8, PieceType::Bishop);
    const auto moveR = *Move::create(e7, e8, PieceType::Rook);

    const auto idxQ = encodeMove(moveQ);
    const auto idxN = encodeMove(moveN);
    const auto idxB = encodeMove(moveB);
    const auto idxR = encodeMove(moveR);

    require(idxQ < kStandardActions, "Queen promotion uses standard action index");
    require(idxN >= kStandardActions, "Knight underpromotion uses underpromotion action index");
    require(idxB >= kStandardActions, "Bishop underpromotion uses underpromotion action index");
    require(idxR >= kStandardActions, "Rook underpromotion uses underpromotion action index");

    require(idxQ != idxN, "Queen and Knight promotions have distinct action indices");
    require(idxQ != idxB, "Queen and Bishop promotions have distinct action indices");
    require(idxQ != idxR, "Queen and Rook promotions have distinct action indices");
    require(idxN != idxB, "Knight and Bishop promotions have distinct action indices");
    require(idxN != idxR, "Knight and Rook promotions have distinct action indices");
    require(idxB != idxR, "Bishop and Rook promotions have distinct action indices");

    // 2. Capture promotions: e7d8 and e7f8
    const auto moveCapN = *Move::create(e7, d8, PieceType::Knight);
    const auto idxCapN = encodeMove(moveCapN);
    require(idxCapN != idxN, "Straight and capture knight promotions have distinct action indices");

    // 3. Black promotions: e2e1
    const Square e2 = makeSquare(File::E, Rank::Two);
    const Square e1 = makeSquare(File::E, Rank::One);
    const auto bMoveQ = *Move::create(e2, e1, PieceType::Queen);
    const auto bMoveN = *Move::create(e2, e1, PieceType::Knight);
    const auto bMoveB = *Move::create(e2, e1, PieceType::Bishop);
    const auto bMoveR = *Move::create(e2, e1, PieceType::Rook);

    const auto bIdxQ = encodeMove(bMoveQ);
    const auto bIdxN = encodeMove(bMoveN);
    const auto bIdxB = encodeMove(bMoveB);
    const auto bIdxR = encodeMove(bMoveR);

    require(bIdxQ != bIdxN && bIdxQ != bIdxB && bIdxQ != bIdxR, "Black promo actions distinct from each other");
    require(bIdxN != bIdxB && bIdxN != bIdxR && bIdxB != bIdxR, "Black underpromos distinct");
    require(bIdxN != idxN, "White and Black underpromos have distinct indices");

    // 4. Decode round-trip for promotions
    const auto decQ = decodeMove(idxQ);
    const auto decN = decodeMove(idxN);
    const auto decB = decodeMove(idxB);
    const auto decR = decodeMove(idxR);

    require(decQ.has_value() && decQ->from() == e7 && decQ->to() == e8 && decQ->promotion() == PieceType::Queen, "Queen promo roundtrip");
    require(decN.has_value() && decN->from() == e7 && decN->to() == e8 && decN->promotion() == PieceType::Knight, "Knight promo roundtrip");
    require(decB.has_value() && decB->from() == e7 && decB->to() == e8 && decB->promotion() == PieceType::Bishop, "Bishop promo roundtrip");
    require(decR.has_value() && decR->from() == e7 && decR->to() == e8 && decR->promotion() == PieceType::Rook, "Rook promo roundtrip");

    // 5. Decode round-trip for Black promotions
    const auto decBN = decodeMove(bIdxN);
    require(decBN.has_value() && decBN->from() == e2 && decBN->to() == e1 && decBN->promotion() == PieceType::Knight, "Black Knight promo roundtrip");

    // 6. Non-promotion standard move round-trip
    const Square e4 = makeSquare(File::E, Rank::Four);
    const auto moveStd = *Move::create(e2, e4, PieceType::None);
    const auto idxStd = encodeMove(moveStd);
    const auto decStd = decodeMove(idxStd);
    require(decStd.has_value() && decStd->from() == e2 && decStd->to() == e4 && decStd->promotion() == PieceType::None, "Standard move roundtrip");
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

void testModelLoadingAndFallback() {
    NeuralEvaluator evaluator;

    // 1. Missing model with allowFallback=false should fail
    const bool strictFail = evaluator.loadModel("non_existent_model.rks", /*allowFallback=*/false);
    require(!strictFail, "Loading missing model without fallback fails strictly");
    require(!evaluator.isLoaded(), "Evaluator is not loaded after failed load");

    // 2. Missing model with allowFallback=true falls back cleanly to Classical
    const bool fallbackOk = evaluator.loadModel("non_existent_model.rks", /*allowFallback=*/true);
    require(fallbackOk, "Loading missing model with fallback returns true");
    require(!evaluator.isLoaded(), "Evaluator is marked not loaded (fallback active)");

    // 3. Fallback evaluation produces valid classical centipawn score
    auto pos = Position::startPosition();
    const auto [score, logits] = evaluator.evaluate(pos);
    require(score >= -100 && score <= 100, "Fallback score is within reasonable starting bounds");
    require(logits.size() == kActionVocabSize, "Logits vector allocated to vocabulary size");
}

void testModelFormatRejectionAndIntegrityChecks() {
    NeuralEvaluator evaluator;

    // 1. Test truncated file rejection
    {
        std::ofstream out("build/test_truncated.rks", std::ios::binary);
        out.write("ROOKSAC2", 8);
        out.close();
        require(!evaluator.loadModel("build/test_truncated.rks", false), "Truncated header rejected");
    }

    // 2. Test corrupted magic bytes rejection
    {
        std::ifstream src("research/checkpoints/model_test.rks", std::ios::binary);
        if (src.is_open()) {
            std::vector<char> buffer((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
            src.close();
            buffer[0] = 'B';
            buffer[1] = 'A';
            buffer[2] = 'D';
            buffer[3] = '!';
            std::ofstream out("build/test_bad_magic.rks", std::ios::binary);
            out.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            out.close();
            require(!evaluator.loadModel("build/test_bad_magic.rks", false), "Bad magic rejected");
        }
    }

    // 3. Test corrupted SHA-256 checksum rejection
    {
        std::ifstream src("research/checkpoints/model_test.rks", std::ios::binary);
        if (src.is_open()) {
            std::vector<char> buffer((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
            src.close();
            // Corrupt a weight byte
            if (buffer.size() > 100) {
                buffer[80] ^= 0x55;
            }
            std::ofstream out("build/test_corrupt_weight.rks", std::ios::binary);
            out.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            out.close();
            require(!evaluator.loadModel("build/test_corrupt_weight.rks", false), "Corrupted weight checksum mismatch rejected");
        }
    }

    // 4. Test trailing unexpected payload rejection
    {
        std::ifstream src("research/checkpoints/model_test.rks", std::ios::binary);
        if (src.is_open()) {
            std::vector<char> buffer((std::istreambuf_iterator<char>(src)), std::istreambuf_iterator<char>());
            src.close();
            buffer.push_back('X');
            buffer.push_back('Y');
            buffer.push_back('Z');
            std::ofstream out("build/test_trailing_bytes.rks", std::ios::binary);
            out.write(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            out.close();
            require(!evaluator.loadModel("build/test_trailing_bytes.rks", false), "Trailing payload rejected");
        }
    }
}

void testRealModelInference() {
    const std::string modelPath = "research/checkpoints/model_test.rks";
    std::ifstream f(modelPath, std::ios::binary);
    if (!f.is_open()) {
        std::cout << "Skipping real model inference test: " << modelPath << " not found.\n";
        return;
    }

    NeuralEvaluator evaluator;
    const bool loaded = evaluator.loadModel(modelPath, /*allowFallback=*/false);
    require(loaded, "Loads real exported .rks model");
    require(evaluator.isLoaded(), "Evaluator isLoaded() is true");
    require(evaluator.metadata().numBlocks == 2, "Model has 2 blocks");
    require(evaluator.metadata().channels == 32, "Model has 32 channels");

    auto pos = Position::startPosition();
    const auto [score, logits] = evaluator.evaluate(pos);
    require(logits.size() == kActionVocabSize, "Logits size is 4240");

    // Symmetry check: mirror positions
    auto whiteOpt = Position::fromFen("7k/5ppp/8/8/8/8/8/3QK3 w - - 0 1");
    auto blackOpt = Position::fromFen("3qk3/8/8/8/8/8/5PPP/7K b - - 0 1");
    require(whiteOpt.hasValue() && blackOpt.hasValue(), "Positions parse");

    const std::int32_t whiteScore = evaluator.evaluateSideToMove(*whiteOpt);
    const std::int32_t blackScore = evaluator.evaluateSideToMove(*blackOpt);
    // Symmetry difference should be small
    require(std::abs(whiteScore - blackScore) <= 50, "Side-to-move evaluation is approximately symmetric");
}

void testSearchRunsWithNeuralEvaluator() {
    auto pos = Position::startPosition();
    NeuralEvaluator evaluator;
    evaluator.loadModel("", /*allowFallback=*/true);

    search::SearchLimits limits(2);
    // Search using leaf evaluator (runs cleanly with fallback or loaded model)
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
    evaluator.loadModel("", /*allowFallback=*/true);
    evaluator.orderMovesByPolicy(pos, legalMoves);

    require(legalMoves.size() == countBefore, "Move count strictly preserved after policy ordering");
    for (std::size_t i = 0; i < legalMoves.size(); ++i) {
        require(movegen::isLegalMove(pos, legalMoves[i]), "Every move remains strictly legal");
    }
}

}  // namespace

int main() {
    testFeatureEncodingDimensionsAndPlanes();
    testGoldenPositionEncodingsMatchPython();
    testActionEncodingBijectiveAndDistinctPromotions();
    testValueCalibrationRoundtrip();
    testModelLoadingAndFallback();
    testModelFormatRejectionAndIntegrityChecks();
    testRealModelInference();
    testSearchRunsWithNeuralEvaluator();
    testPolicyOrderingOnlyRanksLegalMoves();

    std::cout << "All native neural evaluator tests passed successfully.\n";
    return EXIT_SUCCESS;
}
