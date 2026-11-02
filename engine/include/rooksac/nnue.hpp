#pragma once

#include <array>
#include <cstdint>
#include <string_view>
#include <vector>

#include "rooksac/position.hpp"
#include "rooksac/types.hpp"

namespace rooksac {
namespace nnue {

/// \brief Number of input features in the HalfKP-style feature representation.
/// 64 king squares * (10 non-king piece types * 64 board squares)
/// In our compact architectural prototype: 64 squares * 12 piece types = 768 sparse features.
inline constexpr std::size_t kFeatureCount = 13 * 64; // 13 piece codes (0..12) * 64 squares = 832
inline constexpr std::size_t kHiddenSize = 128;   // Layer 1 accumulator dimension

/// \brief Half-feature accumulator holding int16 hidden layer activation values.
struct Accumulator {
    std::array<std::int16_t, kHiddenSize> values{};

    void reset(const std::array<std::int16_t, kHiddenSize>& biases) noexcept {
        values = biases;
    }
};

/// \brief Maps (Piece, Square) pair to a unique sparse feature index in [0, 831].
[[nodiscard]] constexpr std::size_t featureIndex(Piece piece, Square square) noexcept {
    const auto pieceIdx = static_cast<std::size_t>(piece);
    const auto sqIdx = static_cast<std::size_t>(squareIndex(square));
    return pieceIdx * kSquareCount + sqIdx;
}

/// \brief Compact NNUE evaluation network with incremental accumulator update support.
class NnueEvaluator {
public:
    NnueEvaluator() noexcept;

    /// \brief Initializes weights deterministically or loads from a binary buffer.
    bool initialize(std::string_view modelPath = "");

    /// \brief Fully computes the accumulator from scratch for a given position.
    [[nodiscard]] Accumulator computeAccumulator(const Position& position) const noexcept;

    /// \brief Incrementally updates an accumulator: adds new pieces and subtracts removed pieces.
    void updateAccumulator(
        Accumulator& accum,
        const std::vector<std::pair<Piece, Square>>& added,
        const std::vector<std::pair<Piece, Square>>& removed) const noexcept;

    /// \brief Evaluates position using the precomputed accumulator.
    /// Returns score in centipawns from the perspective of the side to move.
    [[nodiscard]] std::int32_t evaluateSideToMove(const Position& position, const Accumulator& accum) const noexcept;

    /// \brief Full evaluation computing accumulator from scratch (for testing and leaf evaluation).
    [[nodiscard]] std::int32_t evaluate(const Position& position) const noexcept;

private:
    std::array<std::int16_t, kHiddenSize> featureBiases_{};
    std::vector<std::int16_t> featureWeights_{}; // [kFeatureCount * kHiddenSize]
    std::array<std::int16_t, kHiddenSize> outputWeights_{};
    std::int32_t outputBias_ = 0;
};

/// \brief Returns the global NNUE evaluator singleton.
[[nodiscard]] NnueEvaluator& getGlobalNnueEvaluator();

}  // namespace nnue
}  // namespace rooksac
