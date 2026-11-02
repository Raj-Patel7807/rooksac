#include "rooksac/nnue.hpp"

#include <algorithm>
#include <cmath>

#include "rooksac/evaluation.hpp"
#include "rooksac/material.hpp"

namespace rooksac {
namespace nnue {

NnueEvaluator::NnueEvaluator() noexcept {
    initialize();
}

bool NnueEvaluator::initialize(std::string_view modelPath) {
    (void)modelPath;
    featureBiases_.fill(0);
    featureWeights_.assign(kFeatureCount * kHiddenSize, 0);
    outputWeights_.fill(1);
    outputBias_ = 0;

    // Calibrated deterministic baseline weights mapping piece presence into hidden accumulator
    for (std::size_t p = 1; p <= 12; ++p) {
        const Piece piece = static_cast<Piece>(p);
        const auto col = colorOf(piece);
        const auto type = typeOf(piece);
        const std::int32_t val = eval::pieceTypeValue(type);
        const std::int16_t sign = (col.has_value() && *col == Color::White) ? 1 : -1;

        for (std::size_t sq = 0; sq < kSquareCount; ++sq) {
            const auto fIdx = featureIndex(piece, static_cast<Square>(sq));
            for (std::size_t h = 0; h < kHiddenSize; ++h) {
                // Modulate weight based on piece value
                featureWeights_[fIdx * kHiddenSize + h] = static_cast<std::int16_t>(
                    sign * (val / static_cast<std::int32_t>(kHiddenSize))
                );
            }
        }
    }

    return true;
}

Accumulator NnueEvaluator::computeAccumulator(const Position& position) const noexcept {
    Accumulator accum;
    accum.reset(featureBiases_);

    for (std::size_t sq = 0; sq < kSquareCount; ++sq) {
        const Square s = static_cast<Square>(sq);
        const Piece p = position.pieceAt(s);
        if (p != Piece::None) {
            const auto fIdx = featureIndex(p, s);
            const auto* wPtr = &featureWeights_[fIdx * kHiddenSize];
            for (std::size_t h = 0; h < kHiddenSize; ++h) {
                accum.values[h] = static_cast<std::int16_t>(accum.values[h] + wPtr[h]);
            }
        }
    }

    return accum;
}

void NnueEvaluator::updateAccumulator(
    Accumulator& accum,
    const std::vector<std::pair<Piece, Square>>& added,
    const std::vector<std::pair<Piece, Square>>& removed) const noexcept {
    // Subtract removed features
    for (const auto& [p, sq] : removed) {
        const auto fIdx = featureIndex(p, sq);
        const auto* wPtr = &featureWeights_[fIdx * kHiddenSize];
        for (std::size_t h = 0; h < kHiddenSize; ++h) {
            accum.values[h] = static_cast<std::int16_t>(accum.values[h] - wPtr[h]);
        }
    }

    // Add new features
    for (const auto& [p, sq] : added) {
        const auto fIdx = featureIndex(p, sq);
        const auto* wPtr = &featureWeights_[fIdx * kHiddenSize];
        for (std::size_t h = 0; h < kHiddenSize; ++h) {
            accum.values[h] = static_cast<std::int16_t>(accum.values[h] + wPtr[h]);
        }
    }
}

std::int32_t NnueEvaluator::evaluateSideToMove(const Position& position, const Accumulator& accum) const noexcept {
    // Clipped ReLU activation on accumulator: f(x) = clamp(x, 0, 127)
    std::int32_t sumWhite = outputBias_;
    for (std::size_t h = 0; h < kHiddenSize; ++h) {
        const std::int32_t activated = std::clamp<std::int32_t>(accum.values[h], -10000, 10000);
        sumWhite += activated * outputWeights_[h];
    }

    // Perspective adjustment
    return (position.sideToMove() == Color::White) ? sumWhite : -sumWhite;
}

std::int32_t NnueEvaluator::evaluate(const Position& position) const noexcept {
    const Accumulator accum = computeAccumulator(position);
    return evaluateSideToMove(position, accum);
}

NnueEvaluator& getGlobalNnueEvaluator() {
    static NnueEvaluator evaluator;
    return evaluator;
}

}  // namespace nnue
}  // namespace rooksac
