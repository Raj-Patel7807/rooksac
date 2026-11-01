#pragma once

#include <cstdint>

namespace rooksac {

enum class CastlingRights : std::uint8_t {
    None = 0,
    WhiteKingside = 1U << 0,
    WhiteQueenside = 1U << 1,
    BlackKingside = 1U << 2,
    BlackQueenside = 1U << 3,
    WhiteBoth = WhiteKingside | WhiteQueenside,
    BlackBoth = BlackKingside | BlackQueenside,
    All = WhiteBoth | BlackBoth,
};

[[nodiscard]] constexpr CastlingRights operator|(CastlingRights a, CastlingRights b) noexcept {
    return static_cast<CastlingRights>(static_cast<std::uint8_t>(a) | static_cast<std::uint8_t>(b));
}

[[nodiscard]] constexpr CastlingRights operator&(CastlingRights a, CastlingRights b) noexcept {
    return static_cast<CastlingRights>(static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(b));
}

[[nodiscard]] constexpr CastlingRights operator~(CastlingRights a) noexcept {
    return static_cast<CastlingRights>(~static_cast<std::uint8_t>(a) & static_cast<std::uint8_t>(CastlingRights::All));
}

constexpr CastlingRights& operator|=(CastlingRights& a, CastlingRights b) noexcept {
    a = a | b;
    return a;
}

constexpr CastlingRights& operator&=(CastlingRights& a, CastlingRights b) noexcept {
    a = a & b;
    return a;
}

[[nodiscard]] constexpr bool hasCastlingRight(CastlingRights rights, CastlingRights query) noexcept {
    return (rights & query) == query;
}

}  // namespace rooksac
