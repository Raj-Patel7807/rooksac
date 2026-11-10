#include "rooksac/neural_evaluator.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

#include "rooksac/evaluation.hpp"
#include "rooksac/sha256.hpp"

namespace rooksac {
namespace neural {

namespace {

// Magic header bytes for RookSac native model format v2
constexpr char kMagicHeader[8] = {'R', 'O', 'O', 'K', 'S', 'A', 'C', '2'};
constexpr std::uint32_t kSupportedFormatVersion = 2;
constexpr std::size_t kHeaderSize = 64;
constexpr std::size_t kSha256DigestSize = 32;

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

    std::error_code ec;
    const auto fileSize = std::filesystem::file_size(modelPath, ec);
    if (ec) {
        std::cerr << "NeuralEvaluator error: Cannot access model file: " << modelPath << '\n';
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    if (fileSize < kHeaderSize + kSha256DigestSize) {
        std::cerr << "NeuralEvaluator error: File too small for header and checksum: " << modelPath << '\n';
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

    // Initialize SHA-256 hasher to stream and verify integrity
    crypto::Sha256 hasher;

    // 1. Read 64-byte Header
    std::array<std::uint8_t, kHeaderSize> headerBytes{};
    if (!file.read(reinterpret_cast<char*>(headerBytes.data()), static_cast<std::streamsize>(headerBytes.size()))) {
        std::cerr << "NeuralEvaluator error: Cannot read header: " << modelPath << '\n';
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }
    hasher.update(headerBytes.data(), headerBytes.size());

    if (std::memcmp(headerBytes.data(), kMagicHeader, 8) != 0) {
        std::cerr << "NeuralEvaluator error: Invalid model file magic in: " << modelPath << '\n';
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    std::uint32_t version = 0;
    std::uint32_t inputChannels = 0;
    std::uint32_t numBlocks = 0;
    std::uint32_t channels = 0;
    std::uint32_t numActions = 0;
    std::uint32_t metaBytes = 0;
    std::uint64_t payloadBytes = 0;

    std::memcpy(&version, headerBytes.data() + 8, sizeof(version));
    std::memcpy(&inputChannels, headerBytes.data() + 12, sizeof(inputChannels));
    std::memcpy(&numBlocks, headerBytes.data() + 16, sizeof(numBlocks));
    std::memcpy(&channels, headerBytes.data() + 20, sizeof(channels));
    std::memcpy(&numActions, headerBytes.data() + 24, sizeof(numActions));
    std::memcpy(&metaBytes, headerBytes.data() + 28, sizeof(metaBytes));
    std::memcpy(&payloadBytes, headerBytes.data() + 32, sizeof(payloadBytes));

    // Bounds and safety verification
    if (version != kSupportedFormatVersion ||
        inputChannels != kInputChannels ||
        numBlocks < 1 || numBlocks > 32 ||
        channels < 8 || channels > 512 ||
        numActions != kActionVocabSize) {
        std::cerr << "NeuralEvaluator error: Incompatible model format or bounds exceeded (version="
                  << version << ", blocks=" << numBlocks << ", channels=" << channels
                  << ", actions=" << numActions << ")\n";
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    // Calculate expected payload size
    const std::size_t convInFloats = channels * kInputChannels * 3 * 3 + channels;
    const std::size_t resBlockFloats = numBlocks * (2 * (channels * channels * 3 * 3) + 2 * channels);
    const std::size_t policyFloats = (16 * channels * 1 * 1 + 16) + (numActions * 16 * 64 + numActions);
    const std::size_t valueFloats = (4 * channels * 1 * 1 + 4) + (64 * 4 * 64 + 64) + (1 * 64 + 1);
    const std::uint64_t expectedPayloadBytes = static_cast<std::uint64_t>(
        (convInFloats + resBlockFloats + policyFloats + valueFloats) * sizeof(float));

    if (payloadBytes != expectedPayloadBytes) {
        std::cerr << "NeuralEvaluator error: Payload byte count mismatch in header (got "
                  << payloadBytes << ", expected " << expectedPayloadBytes << ")\n";
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    const std::uint64_t expectedTotalSize = kHeaderSize + metaBytes + payloadBytes + kSha256DigestSize;
    if (fileSize != expectedTotalSize) {
        std::cerr << "NeuralEvaluator error: File size mismatch (actual=" << fileSize
                  << ", expected=" << expectedTotalSize << ")\n";
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    // 2. Read JSON Metadata
    std::string jsonMeta(metaBytes, '\0');
    if (metaBytes > 0) {
        if (!file.read(jsonMeta.data(), static_cast<std::streamsize>(metaBytes))) {
            std::cerr << "NeuralEvaluator error: Incomplete metadata payload\n";
            if (allowFallback) {
                metadata_.isLoaded = false;
                fallbackMode_ = true;
                return true;
            }
            return false;
        }
        hasher.update(jsonMeta.data(), jsonMeta.size());
    }

    // Helper lambda to read a float buffer, update hasher, and check for NaN/Inf
    auto readWeights = [&](std::vector<float>& buf, std::size_t count) -> bool {
        buf.resize(count);
        const std::streamsize bytesToRead = static_cast<std::streamsize>(count * sizeof(float));
        if (!file.read(reinterpret_cast<char*>(buf.data()), bytesToRead)) {
            return false;
        }
        hasher.update(buf.data(), static_cast<std::size_t>(bytesToRead));
        for (std::size_t i = 0; i < count; ++i) {
            if (std::isnan(buf[i]) || std::isinf(buf[i])) {
                std::cerr << "NeuralEvaluator error: NaN or Inf weight detected\n";
                return false;
            }
        }
        return true;
    };

    // 3. Read Weights Payload
    // 3.1 Conv In
    if (!readWeights(convInW_, channels * kInputChannels * 3 * 3) ||
        !readWeights(convInB_, channels)) {
        std::cerr << "NeuralEvaluator error: Failed reading Conv In weights\n";
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    // 3.2 ResBlocks
    blocks_.resize(numBlocks);
    for (std::size_t b = 0; b < numBlocks; ++b) {
        if (!readWeights(blocks_[b].w1, channels * channels * 3 * 3) ||
            !readWeights(blocks_[b].b1, channels) ||
            !readWeights(blocks_[b].w2, channels * channels * 3 * 3) ||
            !readWeights(blocks_[b].b2, channels)) {
            std::cerr << "NeuralEvaluator error: Failed reading ResBlock " << b << " weights\n";
            if (allowFallback) {
                metadata_.isLoaded = false;
                fallbackMode_ = true;
                return true;
            }
            return false;
        }
    }

    // 3.3 Policy Head
    if (!readWeights(policyConvW_, 16 * channels * 1 * 1) ||
        !readWeights(policyConvB_, 16) ||
        !readWeights(policyFcW_, numActions * 16 * 64) ||
        !readWeights(policyFcB_, numActions)) {
        std::cerr << "NeuralEvaluator error: Failed reading Policy Head weights\n";
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    // 3.4 Value Head
    if (!readWeights(valueConvW_, 4 * channels * 1 * 1) ||
        !readWeights(valueConvB_, 4) ||
        !readWeights(valueFc1W_, 64 * 4 * 64) ||
        !readWeights(valueFc1B_, 64) ||
        !readWeights(valueFc2W_, 1 * 64) ||
        !readWeights(valueFc2B_, 1)) {
        std::cerr << "NeuralEvaluator error: Failed reading Value Head weights\n";
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    // 4. Read & Verify SHA-256 Checksum
    std::array<std::uint8_t, kSha256DigestSize> fileDigest{};
    if (!file.read(reinterpret_cast<char*>(fileDigest.data()), kSha256DigestSize)) {
        std::cerr << "NeuralEvaluator error: Cannot read checksum trailer\n";
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    const auto computedDigest = hasher.finalize();
    if (std::memcmp(fileDigest.data(), computedDigest.data(), kSha256DigestSize) != 0) {
        std::cerr << "NeuralEvaluator error: SHA-256 checksum mismatch (corrupted model file)\n";
        if (allowFallback) {
            metadata_.isLoaded = false;
            fallbackMode_ = true;
            return true;
        }
        return false;
    }

    metadata_.name = "RookSac-NativeCNN";
    metadata_.version = "2.0.0";
    metadata_.schemaVersion = "18x8x8-v1";
    metadata_.numBlocks = numBlocks;
    metadata_.channels = channels;
    metadata_.numActions = numActions;
    metadata_.sourcePath = std::string(modelPath);
    metadata_.jsonMetadata = std::move(jsonMeta);
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
