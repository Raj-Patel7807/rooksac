#pragma once

#include <array>
#include <cstdint>

#include "rooksac/types.hpp"

namespace rooksac {
namespace eval {

/// \brief Pair of (Middlegame, Endgame) score components in centipawns.
struct Score {
    std::int32_t mg = 0;
    std::int32_t eg = 0;

    constexpr Score() noexcept = default;
    constexpr Score(std::int32_t mgVal, std::int32_t egVal) noexcept : mg(mgVal), eg(egVal) {}

    constexpr Score operator+(const Score& other) const noexcept {
        return Score{mg + other.mg, eg + other.eg};
    }

    constexpr Score operator-(const Score& other) const noexcept {
        return Score{mg - other.mg, eg - other.eg};
    }

    constexpr Score& operator+=(const Score& other) noexcept {
        mg += other.mg;
        eg += other.eg;
        return *this;
    }

    constexpr Score& operator-=(const Score& other) noexcept {
        mg -= other.mg;
        eg -= other.eg;
        return *this;
    }

    constexpr Score operator-() const noexcept {
        return Score{-mg, -eg};
    }

    constexpr Score operator*(std::int32_t scalar) const noexcept {
        return Score{mg * scalar, eg * scalar};
    }
};

/// \brief Full set of tunable classical evaluation parameters.
struct EvalParameters {
    // 1. Piece base values: [Pawn, Knight, Bishop, Rook, Queen]
    Score pieceValues[5] = {
        Score{100, 120},   // Pawn
        Score{320, 310},   // Knight
        Score{330, 340},   // Bishop
        Score{500, 530},   // Rook
        Score{900, 950}    // Queen
    };

    // 2. Bishop pair bonus
    Score bishopPair = Score{30, 50};

    // 3. Pawn structure terms
    Score isolatedPawn = Score{-15, -20};
    Score doubledPawn = Score{-15, -25};
    // Passed pawn bonus by rank (ranks 1..8, index 0..7)
    Score passedPawnByRank[8] = {
        Score{0, 0},
        Score{5, 10},
        Score{10, 20},
        Score{20, 40},
        Score{40, 70},
        Score{70, 120},
        Score{110, 180},
        Score{0, 0}
    };

    // 4. Rook activity
    Score rookOnOpenFile = Score{30, 20};
    Score rookOnSemiOpenFile = Score{15, 10};

    // 5. Piece-Square Tables (White perspective, rank 1=0..rank 8=7; Black is mirrored)
    std::array<Score, kSquareCount> pstPawn{};
    std::array<Score, kSquareCount> pstKnight{};
    std::array<Score, kSquareCount> pstBishop{};
    std::array<Score, kSquareCount> pstRook{};
    std::array<Score, kSquareCount> pstQueen{};
    std::array<Score, kSquareCount> pstKing{};

    // 6. King safety: penalty per open/missing file shield pawn
    Score kingMissingShieldPawn = Score{-20, 0};
    Score kingSemiOpenFile = Score{-15, 0};

    // 7. Mobility bonus per available pseudo-legal square (not attacked by enemy pawns)
    Score knightMobility = Score{4, 4};
    Score bishopMobility = Score{4, 4};
    Score rookMobility = Score{2, 3};
    Score queenMobility = Score{1, 2};
};

/// \brief Returns the default tuned evaluation parameter set.
[[nodiscard]] const EvalParameters& defaultParameters() noexcept;

}  // namespace eval
}  // namespace rooksac
