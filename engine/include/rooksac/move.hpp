#pragma once

#include <cstdint>
#include <optional>

#include "rooksac/types.hpp"

namespace rooksac {

class Move {
public:
    // Bits 0-5 encode from, 6-11 encode to, and 12-14 encode promotion: N=1, B=2, R=3, Q=4.
    [[nodiscard]] static constexpr std::optional<Move> create(
        Square from, Square to, PieceType promotion = PieceType::None) noexcept {
        if (!isValidSquare(from) || !isValidSquare(to) || from == to ||
            (promotion != PieceType::None && !isPromotionPieceType(promotion))) {
            return std::nullopt;
        }

        return Move(static_cast<std::uint16_t>(squareIndex(from)) |
                    (static_cast<std::uint16_t>(squareIndex(to)) << kToShift) |
                    (promotionCode(promotion) << kPromotionShift));
    }

    [[nodiscard]] constexpr Square from() const noexcept {
        return static_cast<Square>(encoded_ & kSquareMask);
    }

    [[nodiscard]] constexpr Square to() const noexcept {
        return static_cast<Square>((encoded_ >> kToShift) & kSquareMask);
    }

    [[nodiscard]] constexpr PieceType promotion() const noexcept {
        return promotionFromCode((encoded_ >> kPromotionShift) & kPromotionMask);
    }

    [[nodiscard]] constexpr std::uint16_t encoded() const noexcept {
        return encoded_;
    }

private:
    static inline constexpr std::uint16_t kSquareMask = 0x003FU;
    static inline constexpr std::uint16_t kPromotionMask = 0x0007U;
    static inline constexpr std::uint16_t kToShift = 6U;
    static inline constexpr std::uint16_t kPromotionShift = 12U;

    explicit constexpr Move(std::uint16_t encoded) noexcept : encoded_(encoded) {}

    [[nodiscard]] static constexpr std::uint16_t promotionCode(PieceType promotion) noexcept {
        switch (promotion) {
            case PieceType::Knight:
                return 1U;
            case PieceType::Bishop:
                return 2U;
            case PieceType::Rook:
                return 3U;
            case PieceType::Queen:
                return 4U;
            default:
                return 0U;
        }
    }

    [[nodiscard]] static constexpr PieceType promotionFromCode(std::uint16_t code) noexcept {
        switch (code) {
            case 1U:
                return PieceType::Knight;
            case 2U:
                return PieceType::Bishop;
            case 3U:
                return PieceType::Rook;
            case 4U:
                return PieceType::Queen;
            default:
                return PieceType::None;
        }
    }

    std::uint16_t encoded_;
};

}  // namespace rooksac
