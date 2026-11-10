#include "rooksac/neural_evaluator.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include "rooksac/evaluation.hpp"

namespace rooksac {
namespace neural {

namespace {

// Magic header bytes for RookSac native model format
constexpr char kMagicHeader[8] = {'R', 'O', 'O', 'K', 'S', 'A', 'C', '1'};
constexpr std::uint32_t kSupportedFormatVersion = 1;

void conv2d(
    const float* in,
    std::size_t inChannels,
    std::size_t height,
    std::size_t width,
    const float* weights,
    const float* biases,
    std::size_t outChannels,
    std::size_t kernelSize,
    float* out,
    bool applyRelu = true) noexcept {
    const int pad = static_cast<int>(kernelSize / 2);
    const int H = static_cast<int>(height);
    const int W = static_cast<int>(width);

    for (std::size_t oc = 0; oc < outChannels; ++oc) {
        const float bias = biases ? biases[oc] : 0.0f;
        const float* kWeights = weights + oc * (inChannels * kernelSize * kernelSize);

        for (int r = 0; r < H; ++r) {
            for (int c = 0; c < W; ++c) {
                float sum = bias;

                for (std::size_t ic = 0; ic < inChannels; ++ic) {
                    const float* inChannel = in + ic * (height * width);
                    const float* kw = kWeights + ic * (kernelSize * kernelSize);

                    for (int kr = 0; kr < static_cast<int>(kernelSize); ++kr) {
                        const int inR = r + kr - pad;
                        if (inR < 0 || inR >= H) continue;

                        for (int kc = 0; kc < static_cast<int>(kernelSize); ++kc) {
                            const int inC = c + kc - pad;
                            if (inC < 0 || inC >= W) continue;

                            sum += inChannel[inR * W + inC] * kw[kr * static_cast<int>(kernelSize) + kc];
                        }
                    }
                }

                if (applyRelu && sum < 0.0f) {
                    sum = 0.0f;
                }
                out[oc * (height * width) + (static_cast<std::size_t>(r) * width + static_cast<std::size_t>(c))] = sum;
            }
        }
    }
}

void dense(
    const float* in,
    std::size_t inFeatures,
    const float* weights,
    const float* biases,
    std::size_t outFeatures,
    float* out,
    bool applyRelu = false) noexcept {
    for (std::size_t of = 0; of < outFeatures; ++of) {
        float sum = biases ? biases[of] : 0.0f;
        const float* wRow = weights + of * inFeatures;
        for (std::size_t ifeat = 0; ifeat < inFeatures; ++ifeat) {
            sum += in[ifeat] * wRow[ifeat];
        }
        if (applyRelu && sum < 0.0f) {
            sum = 0.0f;
        }
        out[of] = sum;
    }
}

}  // namespace

FeatureTensor encodePosition(const Position& position) noexcept {
    FeatureTensor tensor{};
    tensor.fill(0.0f);

    // Planes 0-5: White P, N, B, R, Q, K
    // Planes 6-11: Black p, n, b, r, q, k
    for (std::size_t c = 0; c < 2; ++c) {
        const Color col = static_cast<Color>(c);
        const std::size_t basePlane = c * 6;
        for (std::size_t p = 1; p <= 6; ++p) {
            const PieceType pt = static_cast<PieceType>(p);
            Bitboard bb = position.pieces(col, pt);
            const std::size_t plane = basePlane + (p - 1);
            while (bb != 0) {
                const Square sq = popLeastSignificantBit(bb);
                const auto sqIdx = static_cast<std::size_t>(squareIndex(sq));
                tensor[plane * kBoardSquares + sqIdx] = 1.0f;
            }
        }
    }

    // Plane 12: Side to move (1.0 for White, 0.0 for Black)
    if (position.sideToMove() == Color::White) {
        std::fill_n(tensor.begin() + 12 * kBoardSquares, kBoardSquares, 1.0f);
    }

    // Planes 13-16: Castling rights (White K, Q; Black k, q)
    const CastlingRights cr = position.castlingRights();
    if (hasCastlingRight(cr, CastlingRights::WhiteKingside)) {
        std::fill_n(tensor.begin() + 13 * kBoardSquares, kBoardSquares, 1.0f);
    }
    if (hasCastlingRight(cr, CastlingRights::WhiteQueenside)) {
        std::fill_n(tensor.begin() + 14 * kBoardSquares, kBoardSquares, 1.0f);
    }
    if (hasCastlingRight(cr, CastlingRights::BlackKingside)) {
        std::fill_n(tensor.begin() + 15 * kBoardSquares, kBoardSquares, 1.0f);
    }
    if (hasCastlingRight(cr, CastlingRights::BlackQueenside)) {
        std::fill_n(tensor.begin() + 16 * kBoardSquares, kBoardSquares, 1.0f);
    }

    // Plane 17: En-passant target square file
    const Square epSq = position.enPassantSquare();
    if (isValidSquare(epSq)) {
        const auto epFile = static_cast<std::size_t>(fileOf(epSq));
        for (std::size_t r = 0; r < 8; ++r) {
            tensor[17 * kBoardSquares + (r * 8 + epFile)] = 1.0f;
        }
    }

    return tensor;
}

std::int32_t valueToCentipawns(float value, float K) noexcept {
    const float vClamped = std::clamp(value, -0.999f, 0.999f);
    const float ratio = (1.0f - vClamped) / (1.0f + vClamped);
    const float cpFloat = -(400.0f / K) * std::log10(ratio);
    return static_cast<std::int32_t>(std::round(cpFloat));
}

bool NeuralEvaluator::loadModel(std::string_view modelPath, bool allowFallback) {
    if (modelPath.empty()) {
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    std::ifstream file(std::string(modelPath), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "NeuralEvaluator error: Cannot open model file: " << modelPath << '\n';
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    char magic[8];
    if (!file.read(magic, 8) || std::memcmp(magic, kMagicHeader, 8) != 0) {
        std::cerr << "NeuralEvaluator error: Invalid model file header in: " << modelPath << '\n';
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    std::uint32_t version = 0;
    std::uint32_t numBlocks = 0;
    std::uint32_t channels = 0;
    std::uint32_t numActions = 0;

    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.read(reinterpret_cast<char*>(&numBlocks), sizeof(numBlocks));
    file.read(reinterpret_cast<char*>(&channels), sizeof(channels));
    file.read(reinterpret_cast<char*>(&numActions), sizeof(numActions));

    if (!file.good() || version != kSupportedFormatVersion || numActions != kActionVocabSize) {
        std::cerr << "NeuralEvaluator error: Incompatible model format or action count\n";
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    metadata_.name = "RookSac-NativeCNN";
    metadata_.version = "1.0.0";
    metadata_.schemaVersion = "18x8x8-v1";
    metadata_.numBlocks = numBlocks;
    metadata_.channels = channels;
    metadata_.sourcePath = std::string(modelPath);

    // 1. Read Conv In
    convInW_.resize(channels * kInputChannels * 3 * 3);
    convInB_.resize(channels);
    file.read(reinterpret_cast<char*>(convInW_.data()), static_cast<std::streamsize>(convInW_.size() * sizeof(float)));
    file.read(reinterpret_cast<char*>(convInB_.data()), static_cast<std::streamsize>(convInB_.size() * sizeof(float)));

    // 2. Read ResBlocks
    blocks_.resize(numBlocks);
    for (std::size_t b = 0; b < numBlocks; ++b) {
        blocks_[b].w1.resize(channels * channels * 3 * 3);
        blocks_[b].b1.resize(channels);
        blocks_[b].w2.resize(channels * channels * 3 * 3);
        blocks_[b].b2.resize(channels);

        file.read(reinterpret_cast<char*>(blocks_[b].w1.data()), static_cast<std::streamsize>(blocks_[b].w1.size() * sizeof(float)));
        file.read(reinterpret_cast<char*>(blocks_[b].b1.data()), static_cast<std::streamsize>(blocks_[b].b1.size() * sizeof(float)));
        file.read(reinterpret_cast<char*>(blocks_[b].w2.data()), static_cast<std::streamsize>(blocks_[b].w2.size() * sizeof(float)));
        file.read(reinterpret_cast<char*>(blocks_[b].b2.data()), static_cast<std::streamsize>(blocks_[b].b2.size() * sizeof(float)));
    }

    // 3. Read Policy Head
    policyConvW_.resize(16 * channels * 1 * 1);
    policyConvB_.resize(16);
    policyFcW_.resize(numActions * 16 * 64);
    policyFcB_.resize(numActions);

    file.read(reinterpret_cast<char*>(policyConvW_.data()), static_cast<std::streamsize>(policyConvW_.size() * sizeof(float)));
    file.read(reinterpret_cast<char*>(policyConvB_.data()), static_cast<std::streamsize>(policyConvB_.size() * sizeof(float)));
    file.read(reinterpret_cast<char*>(policyFcW_.data()), static_cast<std::streamsize>(policyFcW_.size() * sizeof(float)));
    file.read(reinterpret_cast<char*>(policyFcB_.data()), static_cast<std::streamsize>(policyFcB_.size() * sizeof(float)));

    // 4. Read Value Head
    valueConvW_.resize(4 * channels * 1 * 1);
    valueConvB_.resize(4);
    valueFc1W_.resize(64 * 4 * 64);
    valueFc1B_.resize(64);
    valueFc2W_.resize(1 * 64);
    valueFc2B_.resize(1);

    file.read(reinterpret_cast<char*>(valueConvW_.data()), static_cast<std::streamsize>(valueConvW_.size() * sizeof(float)));
    file.read(reinterpret_cast<char*>(valueConvB_.data()), static_cast<std::streamsize>(valueConvB_.size() * sizeof(float)));
    file.read(reinterpret_cast<char*>(valueFc1W_.data()), static_cast<std::streamsize>(valueFc1W_.size() * sizeof(float)));
    file.read(reinterpret_cast<char*>(valueFc1B_.data()), static_cast<std::streamsize>(valueFc1B_.size() * sizeof(float)));
    file.read(reinterpret_cast<char*>(valueFc2W_.data()), static_cast<std::streamsize>(valueFc2W_.size() * sizeof(float)));
    file.read(reinterpret_cast<char*>(valueFc2B_.data()), static_cast<std::streamsize>(valueFc2B_.size() * sizeof(float)));

    if (!file.good()) {
        std::cerr << "NeuralEvaluator error: Incomplete weights payload in: " << modelPath << '\n';
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    metadata_.isLoaded = true;
    fallbackMode_ = false;
    return true;
}

std::pair<float, std::vector<float>> NeuralEvaluator::forwardPass(const FeatureTensor& input) const {
    if (!metadata_.isLoaded) {
        return {0.0f, std::vector<float>(kActionVocabSize, 0.0f)};
    }

    const std::size_t C = metadata_.channels;
    std::vector<float> buf1(C * 64);
    std::vector<float> buf2(C * 64);
    std::vector<float> residual(C * 64);

    // Initial conv
    conv2d(input.data(), kInputChannels, 8, 8, convInW_.data(), convInB_.data(), C, 3, buf1.data(), /*applyRelu=*/true);

    // Residual blocks
    for (std::size_t b = 0; b < metadata_.numBlocks; ++b) {
        std::memcpy(residual.data(), buf1.data(), C * 64 * sizeof(float));

        // Conv 1
        conv2d(buf1.data(), C, 8, 8, blocks_[b].w1.data(), blocks_[b].b1.data(), C, 3, buf2.data(), /*applyRelu=*/true);
        // Conv 2 (no relu before residual addition)
        conv2d(buf2.data(), C, 8, 8, blocks_[b].w2.data(), blocks_[b].b2.data(), C, 3, buf1.data(), /*applyRelu=*/false);

        // Add residual and relu
        for (std::size_t i = 0; i < C * 64; ++i) {
            const float val = buf1[i] + residual[i];
            buf1[i] = (val > 0.0f) ? val : 0.0f;
        }
    }

    // Policy head: conv 1x1 (16 channels) -> dense numActions
    std::vector<float> pConvOut(16 * 64);
    conv2d(buf1.data(), C, 8, 8, policyConvW_.data(), policyConvB_.data(), 16, 1, pConvOut.data(), /*applyRelu=*/true);

    std::vector<float> policyLogits(kActionVocabSize);
    dense(pConvOut.data(), 16 * 64, policyFcW_.data(), policyFcB_.data(), kActionVocabSize, policyLogits.data(), /*applyRelu=*/false);

    // Value head: conv 1x1 (4 channels) -> dense 64 -> dense 1 -> tanh
    std::vector<float> vConvOut(4 * 64);
    conv2d(buf1.data(), C, 8, 8, valueConvW_.data(), valueConvB_.data(), 4, 1, vConvOut.data(), /*applyRelu=*/true);

    std::vector<float> vFc1Out(64);
    dense(vConvOut.data(), 4 * 64, valueFc1W_.data(), valueFc1B_.data(), 64, vFc1Out.data(), /*applyRelu=*/true);

    float rawValue = 0.0f;
    dense(vFc1Out.data(), 64, valueFc2W_.data(), valueFc2B_.data(), 1, &rawValue, /*applyRelu=*/false);
    const float whiteValue = std::tanh(rawValue);

    return {whiteValue, policyLogits};
}

std::pair<std::int32_t, std::vector<float>> NeuralEvaluator::evaluate(const Position& position) const {
    if (!metadata_.isLoaded) {
        return {eval::evaluateSideToMove(position), std::vector<float>(kActionVocabSize, 0.0f)};
    }

    const FeatureTensor tensor = encodePosition(position);
    const auto [vWhite, logits] = forwardPass(tensor);

    const float vStm = (position.sideToMove() == Color::White) ? vWhite : -vWhite;
    const std::int32_t scoreStm = valueToCentipawns(vStm);
    return {scoreStm, logits};
}

std::int32_t NeuralEvaluator::evaluateSideToMove(const Position& position) const {
    if (!metadata_.isLoaded) {
        return eval::evaluateSideToMove(position);
    }
    const FeatureTensor tensor = encodePosition(position);
    const auto [vWhite, _] = forwardPass(tensor);
    (void)_;
    const float vStm = (position.sideToMove() == Color::White) ? vWhite : -vWhite;
    return valueToCentipawns(vStm);
}

void NeuralEvaluator::orderMovesByPolicy(const Position& position, MoveList& moves) const {
    if (moves.size() <= 1) {
        return;
    }

    const auto [score, logits] = evaluate(position);
    (void)score;

    struct ScoredLegalMove {
        Move move;
        float policyLogit = 0.0f;
    };

    std::vector<ScoredLegalMove> scored;
    scored.reserve(moves.size());
    for (std::size_t i = 0; i < moves.size(); ++i) {
        const auto moveIdx = encodeMove(moves[i]);
        const float logit = (moveIdx < logits.size()) ? logits[moveIdx] : 0.0f;
        scored.push_back({moves[i], logit});
    }

    std::stable_sort(scored.begin(), scored.end(), [](const auto& a, const auto& b) noexcept {
        return a.policyLogit > b.policyLogit;
    });

    for (std::size_t i = 0; i < moves.size(); ++i) {
        moves[i] = scored[i].move;
    }
}

NeuralEvaluator& getGlobalNeuralEvaluator() {
    static NeuralEvaluator evaluator;
    return evaluator;
}

}  // namespace neural
}  // namespace rooksac
