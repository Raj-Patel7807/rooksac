#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "rooksac/eval_types.hpp"

namespace rooksac {
namespace tuning {

/// \brief A single position entry for tuning.
struct TuningEntry {
    std::string fen;
    /// \brief Game outcome from White's perspective: 1.0 = White win, 0.5 = Draw, 0.0 = Black win.
    double result = 0.5;
    /// \brief Game ID to enforce whole-game train/val/test splitting.
    std::string gameId;
};

/// \brief Computes the Texel sigmoid function: S(eval, K) = 1 / (1 + 10^(-K * eval / 400)).
/// \param evalCentipawns Static evaluation from White's perspective in centipawns.
/// \param K Scaling parameter (standard default ~ 1.0, fitted during tuning).
[[nodiscard]] inline double sigmoid(double evalCentipawns, double K = 1.0) noexcept {
    return 1.0 / (1.0 + std::pow(10.0, -K * evalCentipawns / 400.0));
}

/// \brief Computes the Mean Squared Error (MSE) between predicted win probability and actual game outcome.
/// MSE = (1 / N) * sum((result_i - sigmoid(eval_i, K))^2)
[[nodiscard]] double computeMeanSquaredError(
    const std::vector<TuningEntry>& dataset,
    const eval::EvalParameters& params,
    double K = 1.0);

/// \brief Serialization of EvalParameters to and from formatted JSON string.
[[nodiscard]] std::string serializeParametersToJson(const eval::EvalParameters& params, std::string_view version = "1.0.0");
[[nodiscard]] bool deserializeParametersFromJson(std::string_view jsonStr, eval::EvalParameters& outParams);

/// \brief Sequential Probability Ratio Test (SPRT) configuration and state.
struct SprtConfig {
    double elo0 = 0.0;     ///< H0: Null hypothesis Elo advantage
    double elo1 = 5.0;     ///< H1: Alternative hypothesis Elo advantage
    double alpha = 0.05;   ///< Type I error probability (false positive)
    double beta = 0.05;    ///< Type II error probability (false negative)
};

enum class SprtDecision {
    Continue,
    AcceptH0,  ///< Reject candidate (null hypothesis accepted)
    AcceptH1   ///< Accept candidate (alternative hypothesis accepted)
};

struct SprtResult {
    SprtDecision decision = SprtDecision::Continue;
    double logLikelihoodRatio = 0.0;
    double lowerBound = 0.0;
    double upperBound = 0.0;
    std::uint32_t wins = 0;
    std::uint32_t losses = 0;
    std::uint32_t draws = 0;
    std::uint32_t totalGames = 0;
};

/// \brief Evaluates SPRT state given win/loss/draw counts and hypotheses.
[[nodiscard]] SprtResult evaluateSprt(std::uint32_t wins, std::uint32_t losses, std::uint32_t draws, const SprtConfig& config = SprtConfig{});

}  // namespace tuning
}  // namespace rooksac
