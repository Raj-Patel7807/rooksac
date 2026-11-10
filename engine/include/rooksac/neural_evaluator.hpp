#pragma once

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "rooksac/move.hpp"
#include "rooksac/move_list.hpp"
#include "rooksac/position.hpp"

namespace rooksac {
namespace neural {

/// \brief Number of input channels in the feature tensor (matching research/nn/features.py).
inline constexpr std::size_t kInputChannels = 18;
inline constexpr std::size_t kBoardSquares = 64;
inline constexpr std::size_t kInputTensorSize = kInputChannels * kBoardSquares; // 18 * 8 * 8 = 1152 floats

// Action Vocabulary (Version 2.0.0):
// 0..4095: Standard moves and Queen promotions (from * 64 + to)
// 4096..4167: White underpromotions (Knight, Bishop, Rook)
// 4168..4239: Black underpromotions (Knight, Bishop, Rook)
inline constexpr std::size_t kStandardActions = 4096;
inline constexpr std::size_t kUnderpromotionActionsPerColor = 72;
inline constexpr std::size_t kUnderpromotionActions = 144;
inline constexpr std::size_t kActionVocabSize = kStandardActions + kUnderpromotionActions; // 4240
inline constexpr std::uint32_t kWhiteUnderpromoStart = 4096U;
inline constexpr std::uint32_t kBlackUnderpromoStart = 4168U;

/// \brief Represents an 18x8x8 planar float representation of the position.
using FeatureTensor = std::array<float, kInputTensorSize>;

/// \brief Evaluation mode selection.
enum class EvalMode {
    Classical,      ///< Classical tapered evaluation (PSTs, pawns, mobility, king safety)
    NeuralValue,    ///< Neural network value output calibrated to centipawns
    Blended         ///< Calibrated blend of classical and neural evaluations
};

/// \brief Encodes a Position into an 18x8x8 planar float tensor (C-major order: channel, rank, file).
/// Mirrors the exact feature mapping defined in research/nn/features.py.
[[nodiscard]] FeatureTensor encodePosition(const Position& position) noexcept;

/// \brief Encodes a Move into the 0..4167 action vocabulary index (version 2.0.0).
/// Standard moves and Queen promotions use from * 64 + to (0..4095).
/// Underpromotions to Knight, Bishop, or Rook map to unique indices (4096..4167).
[[nodiscard]] constexpr std::uint32_t encodeMove(Move move) noexcept {
    const auto fromSq = move.from();
    const auto toSq = move.to();
    const auto fromIdx = static_cast<std::uint32_t>(squareIndex(fromSq));
    const auto toIdx = static_cast<std::uint32_t>(squareIndex(toSq));
    const auto promo = move.promotion();

    if (promo == PieceType::Knight || promo == PieceType::Bishop || promo == PieceType::Rook) {
        const auto fromRank = static_cast<std::uint32_t>(rankOf(fromSq));
        const auto fromFile = static_cast<std::uint32_t>(fileOf(fromSq));
        const auto toRank = static_cast<std::uint32_t>(rankOf(toSq));
        const auto toFile = static_cast<std::uint32_t>(fileOf(toSq));

        const std::uint32_t pieceIdx = (promo == PieceType::Knight) ? 0U :
                                       (promo == PieceType::Bishop) ? 1U : 2U;
        const std::int32_t fileDiff = static_cast<std::int32_t>(toFile) - static_cast<std::int32_t>(fromFile);
        const std::uint32_t dirIdx = static_cast<std::uint32_t>(fileDiff + 1); // 0: left, 1: straight, 2: right

        if (fromRank == 6U && toRank == 7U) {
            // White underpromotion
            return kWhiteUnderpromoStart + fromFile * 9U + pieceIdx * 3U + dirIdx;
        }
        if (fromRank == 1U && toRank == 0U) {
            // Black underpromotion
            return kBlackUnderpromoStart + fromFile * 9U + pieceIdx * 3U + dirIdx;
        }
    }

    return fromIdx * 64U + toIdx;
}

/// \brief Decodes a 0..4167 action vocabulary index into a Move.
/// If move is in 0..4095, promotion defaults to PieceType::None (or Queen if on promotion rank).
[[nodiscard]] constexpr std::optional<Move> decodeMove(std::uint32_t moveIdx) noexcept {
    if (moveIdx < kStandardActions) {
        const auto fromIdx = static_cast<std::uint8_t>(moveIdx / 64U);
        const auto toIdx = static_cast<std::uint8_t>(moveIdx % 64U);
        if (fromIdx >= 64U || toIdx >= 64U || fromIdx == toIdx) {
            return std::nullopt;
        }
        const auto fromSq = static_cast<Square>(fromIdx);
        const auto toSq = static_cast<Square>(toIdx);
        const auto fromRank = static_cast<std::uint8_t>(rankOf(fromSq));
        const auto toRank = static_cast<std::uint8_t>(rankOf(toSq));
        const auto fileDiff = std::abs(static_cast<int>(fileOf(toSq)) - static_cast<int>(fileOf(fromSq)));

        // If move is a pawn advance to the 8th or 1st rank, standard index indicates Queen promotion
        if (fileDiff <= 1 && ((fromRank == 6U && toRank == 7U) || (fromRank == 1U && toRank == 0U))) {
            return Move::create(fromSq, toSq, PieceType::Queen);
        }
        return Move::create(fromSq, toSq, PieceType::None);
    }

    if (moveIdx < kActionVocabSize) {
        if (moveIdx < kBlackUnderpromoStart) {
            // White underpromotion
            const std::uint32_t offset = moveIdx - kWhiteUnderpromoStart;
            const std::uint32_t fromFile = offset / 9U;
            const std::uint32_t rem = offset % 9U;
            const std::uint32_t pieceIdx = rem / 3U;
            const std::uint32_t dirIdx = rem % 3U;
            const std::int32_t toFile = static_cast<std::int32_t>(fromFile) + (static_cast<std::int32_t>(dirIdx) - 1);
            if (fromFile >= 8U || toFile < 0 || toFile >= 8) {
                return std::nullopt;
            }
            const Square fromSq = makeSquare(static_cast<File>(fromFile), Rank::Seven);
            const Square toSq = makeSquare(static_cast<File>(toFile), Rank::Eight);
            const PieceType promo = (pieceIdx == 0U) ? PieceType::Knight :
                                    (pieceIdx == 1U) ? PieceType::Bishop : PieceType::Rook;
            return Move::create(fromSq, toSq, promo);
        } else {
            // Black underpromotion
            const std::uint32_t offset = moveIdx - kBlackUnderpromoStart;
            const std::uint32_t fromFile = offset / 9U;
            const std::uint32_t rem = offset % 9U;
            const std::uint32_t pieceIdx = rem / 3U;
            const std::uint32_t dirIdx = rem % 3U;
            const std::int32_t toFile = static_cast<std::int32_t>(fromFile) + (static_cast<std::int32_t>(dirIdx) - 1);
            if (fromFile >= 8U || toFile < 0 || toFile >= 8) {
                return std::nullopt;
            }
            const Square fromSq = makeSquare(static_cast<File>(fromFile), Rank::Two);
            const Square toSq = makeSquare(static_cast<File>(toFile), Rank::One);
            const PieceType promo = (pieceIdx == 0U) ? PieceType::Knight :
                                    (pieceIdx == 1U) ? PieceType::Bishop : PieceType::Rook;
            return Move::create(fromSq, toSq, promo);
        }
    }

    return std::nullopt;
}

/// \brief Converts a neural value prediction v in [-1.0, 1.0] from White's perspective
/// into an integer centipawn score calibrated to the engine's score convention.
/// Formula: centipawns = -400 * log10((1 - v) / (1 + v)) / K  (inverse sigmoid)
/// Clamped safely to avoid division by zero or infinities.
[[nodiscard]] std::int32_t valueToCentipawns(float value, float K = 1.0f) noexcept;

/// \brief Neural evaluator model metadata.
struct ModelMetadata {
    std::string name = "RookSac-PolicyValueNet";
    std::string version = "1.0.0";
    std::string schemaVersion = "18x8x8-v1";
    std::uint32_t numBlocks = 0;
    std::uint32_t channels = 0;
    std::string sourcePath;
    bool isLoaded = false;
};

/// \brief ResNet block weights for native forward pass.
struct NativeResBlock {
    std::vector<float> w1; // [channels, channels, 3, 3]
    std::vector<float> b1; // [channels]
    std::vector<float> w2; // [channels, channels, 3, 3]
    std::vector<float> b2; // [channels]
};

/// \brief Standalone native lightweight forward-pass neural inference engine.
/// Executes inference purely in C++ without Python runtime dependencies.
class NeuralEvaluator {
public:
    NeuralEvaluator() noexcept = default;

    /// \brief Loads a native model from a binary .rks file.
    /// If allowFallback is true and loading fails, reverts cleanly to Classical evaluation.
    bool loadModel(std::string_view modelPath = "", bool allowFallback = true);

    /// \brief Returns whether a real neural model is currently loaded.
    [[nodiscard]] bool isLoaded() const noexcept { return metadata_.isLoaded; }

    /// \brief Returns model metadata.
    [[nodiscard]] const ModelMetadata& metadata() const noexcept { return metadata_; }

    /// \brief Evaluates the position and returns (valueScoreCentipawns, policyLogits).
    /// Value is returned from the perspective of the side to move (matching search convention).
    [[nodiscard]] std::pair<std::int32_t, std::vector<float>> evaluate(const Position& position) const;

    /// \brief Convenience method returning the centipawn score for the side to move.
    [[nodiscard]] std::int32_t evaluateSideToMove(const Position& position) const;

    /// \brief Re-orders the provided legal moves in-place based on descending policy logits.
    /// Evaluates policy solely on the given legal moves; never creates or accepts non-legal moves.
    void orderMovesByPolicy(const Position& position, MoveList& moves) const;

    /// \brief Directly runs forward pass on a feature tensor, returning raw (whiteValueInMinus1To1, policyLogits).
    [[nodiscard]] std::pair<float, std::vector<float>> forwardPass(const FeatureTensor& input) const;

private:
    ModelMetadata metadata_{};
    bool fallbackMode_ = false;

    // Weights
    std::vector<float> convInW_; // [channels, 18, 3, 3]
    std::vector<float> convInB_; // [channels]
    std::vector<NativeResBlock> blocks_;

    // Policy head
    std::vector<float> policyConvW_; // [16, channels, 1, 1]
    std::vector<float> policyConvB_; // [16]
    std::vector<float> policyFcW_;   // [kActionVocabSize, 16 * 64]
    std::vector<float> policyFcB_;   // [kActionVocabSize]

    // Value head
    std::vector<float> valueConvW_; // [4, channels, 1, 1]
    std::vector<float> valueConvB_; // [4]
    std::vector<float> valueFc1W_;  // [64, 4 * 64]
    std::vector<float> valueFc1B_;  // [64]
    std::vector<float> valueFc2W_;  // [1, 64]
    std::vector<float> valueFc2B_;  // [1]
};

/// \brief Returns the global neural evaluator singleton.
[[nodiscard]] NeuralEvaluator& getGlobalNeuralEvaluator();

}  // namespace neural
}  // namespace rooksac
