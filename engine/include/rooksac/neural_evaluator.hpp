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
inline constexpr std::size_t kActionVocabSize = 4096;                           // 64 * 64 from/to moves

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

/// \brief Encodes a Move into the 0..4095 action vocabulary index.
[[nodiscard]] constexpr std::uint32_t encodeMove(Move move) noexcept {
    const auto fromIdx = static_cast<std::uint32_t>(squareIndex(move.from()));
    const auto toIdx = static_cast<std::uint32_t>(squareIndex(move.to()));
    return fromIdx * 64U + toIdx;
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
    std::vector<float> policyFcW_;   // [4096, 16 * 64]
    std::vector<float> policyFcB_;   // [4096]

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
