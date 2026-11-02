#include "rooksac/tuning.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

#include "rooksac/evaluation.hpp"
#include "rooksac/position.hpp"

namespace rooksac {
namespace tuning {

double computeMeanSquaredError(
    const std::vector<TuningEntry>& dataset,
    const eval::EvalParameters& params,
    double K) {
    if (dataset.empty()) {
        return 0.0;
    }

    double totalSquaredError = 0.0;
    std::size_t validPositions = 0;

    for (const auto& entry : dataset) {
        auto posOpt = Position::fromFen(entry.fen);
        if (!posOpt.hasValue()) {
            continue;
        }

        // evaluate() returns score from White's perspective in centipawns
        const double evalScore = static_cast<double>(eval::evaluate(*posOpt, params));
        const double predictedProb = sigmoid(evalScore, K);
        const double diff = entry.result - predictedProb;
        totalSquaredError += diff * diff;
        ++validPositions;
    }

    return (validPositions > 0) ? (totalSquaredError / static_cast<double>(validPositions)) : 0.0;
}

std::string serializeParametersToJson(const eval::EvalParameters& params, std::string_view version) {
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"schemaVersion\": \"" << version << "\",\n";
    ss << "  \"bishopPair\": {\"mg\": " << params.bishopPair.mg << ", \"eg\": " << params.bishopPair.eg << "},\n";
    ss << "  \"isolatedPawn\": {\"mg\": " << params.isolatedPawn.mg << ", \"eg\": " << params.isolatedPawn.eg << "},\n";
    ss << "  \"doubledPawn\": {\"mg\": " << params.doubledPawn.mg << ", \"eg\": " << params.doubledPawn.eg << "},\n";
    ss << "  \"rookOnOpenFile\": {\"mg\": " << params.rookOnOpenFile.mg << ", \"eg\": " << params.rookOnOpenFile.eg << "},\n";
    ss << "  \"rookOnSemiOpenFile\": {\"mg\": " << params.rookOnSemiOpenFile.mg << ", \"eg\": " << params.rookOnSemiOpenFile.eg << "},\n";
    ss << "  \"kingMissingShieldPawn\": {\"mg\": " << params.kingMissingShieldPawn.mg << ", \"eg\": " << params.kingMissingShieldPawn.eg << "},\n";
    ss << "  \"kingSemiOpenFile\": {\"mg\": " << params.kingSemiOpenFile.mg << ", \"eg\": " << params.kingSemiOpenFile.eg << "},\n";
    ss << "  \"knightMobility\": {\"mg\": " << params.knightMobility.mg << ", \"eg\": " << params.knightMobility.eg << "},\n";
    ss << "  \"bishopMobility\": {\"mg\": " << params.bishopMobility.mg << ", \"eg\": " << params.bishopMobility.eg << "},\n";
    ss << "  \"rookMobility\": {\"mg\": " << params.rookMobility.mg << ", \"eg\": " << params.rookMobility.eg << "},\n";
    ss << "  \"queenMobility\": {\"mg\": " << params.queenMobility.mg << ", \"eg\": " << params.queenMobility.eg << "}\n";
    ss << "}\n";
    return ss.str();
}

namespace {

std::int32_t extractScoreField(std::string_view json, std::string_view key, std::string_view subkey, std::int32_t fallback) {
    const auto keyPos = json.find(key);
    if (keyPos == std::string_view::npos) return fallback;
    const auto subPos = json.find(subkey, keyPos);
    if (subPos == std::string_view::npos) return fallback;
    const auto colonPos = json.find(':', subPos);
    if (colonPos == std::string_view::npos) return fallback;

    std::size_t start = colonPos + 1;
    while (start < json.size() && (json[start] == ' ' || json[start] == '\t')) ++start;
    std::size_t end = start;
    while (end < json.size() && (json[end] == '-' || (json[end] >= '0' && json[end] <= '9'))) ++end;

    if (start < end) {
        std::string valStr(json.substr(start, end - start));
        try {
            return std::stoi(valStr);
        } catch (...) {
            return fallback;
        }
    }
    return fallback;
}

}  // namespace

bool deserializeParametersFromJson(std::string_view jsonStr, eval::EvalParameters& outParams) {
    if (jsonStr.empty() || jsonStr.find("schemaVersion") == std::string_view::npos) {
        return false;
    }

    outParams.bishopPair.mg = extractScoreField(jsonStr, "\"bishopPair\"", "\"mg\"", outParams.bishopPair.mg);
    outParams.bishopPair.eg = extractScoreField(jsonStr, "\"bishopPair\"", "\"eg\"", outParams.bishopPair.eg);
    outParams.isolatedPawn.mg = extractScoreField(jsonStr, "\"isolatedPawn\"", "\"mg\"", outParams.isolatedPawn.mg);
    outParams.isolatedPawn.eg = extractScoreField(jsonStr, "\"isolatedPawn\"", "\"eg\"", outParams.isolatedPawn.eg);
    outParams.doubledPawn.mg = extractScoreField(jsonStr, "\"doubledPawn\"", "\"mg\"", outParams.doubledPawn.mg);
    outParams.doubledPawn.eg = extractScoreField(jsonStr, "\"doubledPawn\"", "\"eg\"", outParams.doubledPawn.eg);
    outParams.rookOnOpenFile.mg = extractScoreField(jsonStr, "\"rookOnOpenFile\"", "\"mg\"", outParams.rookOnOpenFile.mg);
    outParams.rookOnOpenFile.eg = extractScoreField(jsonStr, "\"rookOnOpenFile\"", "\"eg\"", outParams.rookOnOpenFile.eg);
    outParams.rookOnSemiOpenFile.mg = extractScoreField(jsonStr, "\"rookOnSemiOpenFile\"", "\"mg\"", outParams.rookOnSemiOpenFile.mg);
    outParams.rookOnSemiOpenFile.eg = extractScoreField(jsonStr, "\"rookOnSemiOpenFile\"", "\"eg\"", outParams.rookOnSemiOpenFile.eg);

    return true;
}

SprtResult evaluateSprt(std::uint32_t wins, std::uint32_t losses, std::uint32_t draws, const SprtConfig& config) {
    SprtResult result;
    result.wins = wins;
    result.losses = losses;
    result.draws = draws;
    result.totalGames = wins + losses + draws;

    // Wald's SPRT boundary limits
    result.lowerBound = std::log(config.beta / (1.0 - config.alpha));
    result.upperBound = std::log((1.0 - config.beta) / config.alpha);

    if (result.totalGames == 0) {
        result.decision = SprtDecision::Continue;
        result.logLikelihoodRatio = 0.0;
        return result;
    }

    // Convert Elo bounds to expected score probabilities: P = 1 / (1 + 10^(-elo/400))
    const double p0 = 1.0 / (1.0 + std::pow(10.0, -config.elo0 / 400.0));
    const double p1 = 1.0 / (1.0 + std::pow(10.0, -config.elo1 / 400.0));

    // Empirical score and variance estimate
    const double score = (static_cast<double>(wins) + 0.5 * static_cast<double>(draws)) / static_cast<double>(result.totalGames);
    const double wProb = static_cast<double>(wins) / static_cast<double>(result.totalGames);
    const double dProb = static_cast<double>(draws) / static_cast<double>(result.totalGames);
    const double var = (wProb * std::pow(1.0 - score, 2.0)) +
                       (dProb * std::pow(0.5 - score, 2.0)) +
                       ((1.0 - wProb - dProb) * std::pow(0.0 - score, 2.0));

    const double s2 = std::max(var, 0.01);  // Avoid division by zero
    const double deltaP = p1 - p0;

    // Normal approximation of Log-Likelihood Ratio for trinomial chess matches:
    // LLR = (p1 - p0) * sum(xi - (p0 + p1)/2) / s^2
    const double sumX = static_cast<double>(wins) + 0.5 * static_cast<double>(draws);
    result.logLikelihoodRatio = (deltaP / s2) * (sumX - static_cast<double>(result.totalGames) * (p0 + p1) / 2.0);

    if (result.logLikelihoodRatio >= result.upperBound) {
        result.decision = SprtDecision::AcceptH1;
    } else if (result.logLikelihoodRatio <= result.lowerBound) {
        result.decision = SprtDecision::AcceptH0;
    } else {
        result.decision = SprtDecision::Continue;
    }

    return result;
}

}  // namespace tuning
}  // namespace rooksac
