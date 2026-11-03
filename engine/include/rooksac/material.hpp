#pragma once

#include <cstdint>

#include "rooksac/position.hpp"
#include "rooksac/types.hpp"

namespace rooksac {
namespace eval {

struct MaterialWeights {
    std::int32_t pawn = 100;
    std::int32_t knight = 320;
    std::int32_t bishop = 330;
    std::int32_t rook = 500;
    std::int32_t queen = 900;
};

inline constexpr MaterialWeights kDefaultMaterialWeights{};

/// \brief Returns the material value of a single piece type in centipawns.
[[nodiscard]] constexpr std::int32_t pieceTypeValue(PieceType type, const MaterialWeights& weights = kDefaultMaterialWeights) noexcept {
    switch (type) {
        case PieceType::Pawn: return weights.pawn;
        case PieceType::Knight: return weights.knight;
        case PieceType::Bishop: return weights.bishop;
        case PieceType::Rook: return weights.rook;
        case PieceType::Queen: return weights.queen;
        case PieceType::King:
        case PieceType::None:
        default:
            return 0;
    }
}

/// \brief Computes net material balance in centipawns from the perspective of White (positive favors White).
[[nodiscard]] std::int32_t evaluateMaterial(const Position& position, const MaterialWeights& weights = kDefaultMaterialWeights) noexcept;

/// \brief Computes net material balance from the perspective of the side to move (positive favors side to move).
[[nodiscard]] std::int32_t evaluateMaterialSideToMove(const Position& position, const MaterialWeights& weights = kDefaultMaterialWeights) noexcept;

/// \brief Checks whether the position has insufficient material for checkmate by either side.
/// Specifically detects:
/// - K vs K
/// - K+B vs K
/// - K+N vs K
/// - K+B vs K+B where both bishops are on the same square color
[[nodiscard]] bool hasInsufficientMaterial(const Position& position) noexcept;

}  // namespace eval
}  // namespace rooksac

