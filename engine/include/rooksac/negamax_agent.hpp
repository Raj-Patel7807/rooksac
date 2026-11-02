#pragma once

#include <cstdint>
#include <optional>
#include <string_view>

#include "rooksac/agent.hpp"
#include "rooksac/material.hpp"
#include "rooksac/search.hpp"

namespace rooksac {

/// \brief Move selection agent using depth-limited Negamax tree search.
class NegamaxAgent : public Agent {
public:
    explicit NegamaxAgent(
        std::uint32_t depth = 2,
        eval::MaterialWeights weights = eval::kDefaultMaterialWeights) noexcept
        : depth_(depth), weights_(weights) {}

    [[nodiscard]] std::string_view name() const noexcept override {
        return "NegamaxAgent";
    }

    [[nodiscard]] std::optional<Move> selectMove(Position& position, const MoveList& legalMoves) override {
        if (legalMoves.empty()) {
            return std::nullopt;
        }

        const search::SearchLimits limits(depth_);
        const auto result = search::search(position, limits, [this](const Position& pos) {
            return eval::evaluateMaterialSideToMove(pos, weights_);
        });

        return result.bestMove;
    }

    [[nodiscard]] std::uint32_t depth() const noexcept { return depth_; }
    void setDepth(std::uint32_t depth) noexcept { depth_ = depth; }

    [[nodiscard]] const eval::MaterialWeights& weights() const noexcept { return weights_; }
    void setWeights(const eval::MaterialWeights& weights) noexcept { weights_ = weights; }

private:
    std::uint32_t depth_{2};
    eval::MaterialWeights weights_{};
};

}  // namespace rooksac

