#pragma once

#include <cstdint>
#include <limits>
#include <random>
#include <vector>

#include "rooksac/agent.hpp"
#include "rooksac/material.hpp"
#include "rooksac/undo_state.hpp"

namespace rooksac {

/// \brief 1-ply greedy agent that chooses the move leading to the best immediate material balance.
///
/// Ties are broken deterministically by default (first best move), or using an injectable RNG seed.
/// Known limitation: completely blind to opponent responses, piece defense, positional harmony,
/// or tactics (e.g. captures a defended pawn with queen).
class GreedyAgent final : public Agent {
public:
    explicit GreedyAgent(eval::MaterialWeights weights = eval::kDefaultMaterialWeights,
                         bool randomizeTies = false,
                         std::uint64_t seed = 42) noexcept
        : weights_(weights), randomizeTies_(randomizeTies), rng_(seed) {}

    [[nodiscard]] std::string_view name() const noexcept override {
        return "GreedyAgent";
    }

    void setSeed(std::uint64_t seed) noexcept {
        rng_.seed(seed);
    }

    [[nodiscard]] const eval::MaterialWeights& weights() const noexcept {
        return weights_;
    }

    [[nodiscard]] std::optional<Move> selectMove(Position& position, const MoveList& legalMoves) override {
        if (legalMoves.empty()) {
            return std::nullopt;
        }

        const Color us = position.sideToMove();
        std::int32_t bestScore = std::numeric_limits<std::int32_t>::min();
        std::vector<Move> bestMoves;

        UndoState undo;
        for (const Move move : legalMoves) {
            const bool success = position.makeMove(move, undo);
            (void)success;

            // Evaluate resulting position from the perspective of 'us'
            const std::int32_t whiteEval = eval::evaluateMaterial(position, weights_);
            const std::int32_t ourScore = (us == Color::White) ? whiteEval : -whiteEval;

            position.unmakeMove(move, undo);

            if (ourScore > bestScore) {
                bestScore = ourScore;
                bestMoves.clear();
                bestMoves.push_back(move);
            } else if (ourScore == bestScore) {
                bestMoves.push_back(move);
            }
        }

        if (bestMoves.empty()) {
            return std::nullopt;
        }

        if (!randomizeTies_ || bestMoves.size() == 1) {
            return bestMoves.front();
        }

        std::uniform_int_distribution<std::size_t> dist(0, bestMoves.size() - 1);
        return bestMoves[dist(rng_)];
    }

private:
    eval::MaterialWeights weights_;
    bool randomizeTies_;
    std::mt19937_64 rng_;
};

}  // namespace rooksac

