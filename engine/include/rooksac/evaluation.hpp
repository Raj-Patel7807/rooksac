#pragma once

#include <cstdint>

#include "rooksac/eval_types.hpp"
#include "rooksac/position.hpp"

namespace rooksac {
namespace eval {

/// \brief Total phase value in classical evaluation (standard 24 non-pawn phase sum).
inline constexpr std::int32_t kTotalPhase = 24;

/// \brief Returns the game phase of a position (0 = pure endgame, 24 = opening/full middlegame).
[[nodiscard]] std::int32_t calculateGamePhase(const Position& position) noexcept;

/// \brief Blends middlegame and endgame score components using game phase.
[[nodiscard]] constexpr std::int32_t blendScore(Score score, std::int32_t phase) noexcept {
    // Phase 24 -> 100% MG; Phase 0 -> 100% EG
    return ((score.mg * phase) + (score.eg * (kTotalPhase - phase))) / kTotalPhase;
}

/// \brief Detailed term-by-term evaluation breakdown for analysis, tuning, and debugging.
struct EvalBreakdown {
    Score materialAndPst{};
    Score pawns{};
    Score kingSafety{};
    Score piecesAndMobility{};
    std::int32_t phase = 0;
    std::int32_t blendedTotal = 0;
    std::int32_t sideToMoveScore = 0;
};

/// \brief Computes a full term-by-term evaluation breakdown without printing or polluting stdout.
[[nodiscard]] EvalBreakdown evaluateBreakdown(
    const Position& position,
    const EvalParameters& params = defaultParameters()) noexcept;

/// \brief Evaluates material and piece-square table scores for both sides.
[[nodiscard]] Score evaluateMaterialAndPST(const Position& position, const EvalParameters& params) noexcept;

/// \brief Evaluates pawn structure features (isolated, doubled, passed pawns).
[[nodiscard]] Score evaluatePawns(const Position& position, const EvalParameters& params) noexcept;

/// \brief Evaluates king safety terms (shield pawns, semi-open files near king).
[[nodiscard]] Score evaluateKingSafety(const Position& position, const EvalParameters& params) noexcept;

/// \brief Evaluates piece mobility and activity (bishop pair, open rooks, safe mobility).
[[nodiscard]] Score evaluatePiecesAndMobility(const Position& position, const EvalParameters& params) noexcept;

/// \brief Full tapered classical evaluation in centipawns from the perspective of White (positive favors White).
[[nodiscard]] std::int32_t evaluate(
    const Position& position,
    const EvalParameters& params = defaultParameters()) noexcept;

/// \brief Full tapered classical evaluation in centipawns relative to the side to move (positive favors side to move).
[[nodiscard]] std::int32_t evaluateSideToMove(
    const Position& position,
    const EvalParameters& params = defaultParameters()) noexcept;

}  // namespace eval
}  // namespace rooksac
