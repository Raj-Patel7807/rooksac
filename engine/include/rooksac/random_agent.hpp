#pragma once

#include <cstdint>
#include <random>

#include "rooksac/agent.hpp"

namespace rooksac {

/// \brief Agent policy that selects uniformly at random from available legal moves.
/// Uses an injectable 64-bit seed to ensure reproducible test runs.
class RandomAgent final : public Agent {
public:
    explicit RandomAgent(std::uint64_t seed = 42) noexcept : rng_(seed) {}

    [[nodiscard]] std::string_view name() const noexcept override {
        return "RandomAgent";
    }

    void setSeed(std::uint64_t seed) noexcept {
        rng_.seed(seed);
    }

    [[nodiscard]] std::optional<Move> selectMove(Position& position, const MoveList& legalMoves) override {
        (void)position;
        if (legalMoves.empty()) {
            return std::nullopt;
        }

        std::uniform_int_distribution<std::size_t> dist(0, legalMoves.size() - 1);
        const std::size_t chosenIndex = dist(rng_);
        return legalMoves[chosenIndex];
    }

private:
    std::mt19937_64 rng_;
};

}  // namespace rooksac
