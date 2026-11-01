#pragma once

#include <array>
#include <cstdint>

#include "rooksac/bitboard.hpp"
#include "rooksac/types.hpp"

namespace rooksac {

/// \brief Pure functions and precomputed attack lookup tables for chess pieces.
///
/// Attack queries model which squares a piece attacks regardless of king check or pins.
/// A piece attacks a square even if moving there would expose its own king to check.
namespace attacks {

namespace detail {

constexpr std::array<std::array<Bitboard, kSquareCount>, kColorCount> initPawnAttacks() noexcept {
    std::array<std::array<Bitboard, kSquareCount>, kColorCount> table{};
    for (std::uint8_t sq = 0; sq < kSquareCount; ++sq) {
        const Bitboard b = Bitboard{1} << sq;
        // White attacks diagonally up-left (file != A) and up-right (file != H)
        // fileOf(sq) > 0 -> not File A -> sq + 7
        // fileOf(sq) < 7 -> not File H -> sq + 9
        Bitboard whiteAttacks = 0;
        if ((b & ~kFileA) != 0) {
            whiteAttacks |= (b << 7U);
        }
        if ((b & ~kFileH) != 0) {
            whiteAttacks |= (b << 9U);
        }
        table[static_cast<std::size_t>(Color::White)][sq] = whiteAttacks;

        // Black attacks diagonally down-left (file != A) and down-right (file != H)
        // fileOf(sq) > 0 -> not File A -> sq - 9
        // fileOf(sq) < 7 -> not File H -> sq - 7
        Bitboard blackAttacks = 0;
        if ((b & ~kFileA) != 0) {
            blackAttacks |= (b >> 9U);
        }
        if ((b & ~kFileH) != 0) {
            blackAttacks |= (b >> 7U);
        }
        table[static_cast<std::size_t>(Color::Black)][sq] = blackAttacks;
    }
    return table;
}

constexpr std::array<Bitboard, kSquareCount> initKnightAttacks() noexcept {
    std::array<Bitboard, kSquareCount> table{};
    for (std::uint8_t sq = 0; sq < kSquareCount; ++sq) {
        const int f = static_cast<int>(sq % 8U);
        const int r = static_cast<int>(sq / 8U);
        Bitboard att = 0;
        constexpr int df[8] = {1, 2, 2, 1, -1, -2, -2, -1};
        constexpr int dr[8] = {2, 1, -1, -2, -2, -1, 1, 2};
        for (int i = 0; i < 8; ++i) {
            const int nf = f + df[i];
            const int nr = r + dr[i];
            if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) {
                att |= Bitboard{1} << (nr * 8 + nf);
            }
        }
        table[sq] = att;
    }
    return table;
}

constexpr std::array<Bitboard, kSquareCount> initKingAttacks() noexcept {
    std::array<Bitboard, kSquareCount> table{};
    for (std::uint8_t sq = 0; sq < kSquareCount; ++sq) {
        const int f = static_cast<int>(sq % 8U);
        const int r = static_cast<int>(sq / 8U);
        Bitboard att = 0;
        constexpr int df[8] = {-1, -1, -1, 0, 0, 1, 1, 1};
        constexpr int dr[8] = {-1, 0, 1, -1, 1, -1, 0, 1};
        for (int i = 0; i < 8; ++i) {
            const int nf = f + df[i];
            const int nr = r + dr[i];
            if (nf >= 0 && nf < 8 && nr >= 0 && nr < 8) {
                att |= Bitboard{1} << (nr * 8 + nf);
            }
        }
        table[sq] = att;
    }
    return table;
}

}  // namespace detail

inline constexpr auto kPawnAttacks = detail::initPawnAttacks();
inline constexpr auto kKnightAttacks = detail::initKnightAttacks();
inline constexpr auto kKingAttacks = detail::initKingAttacks();

[[nodiscard]] constexpr Bitboard pawnAttacks(Square square, Color color) noexcept {
    if (!isValidSquare(square)) return kEmptyBitboard;
    return kPawnAttacks[static_cast<std::size_t>(color)][squareIndex(square)];
}

[[nodiscard]] constexpr Bitboard knightAttacks(Square square) noexcept {
    if (!isValidSquare(square)) return kEmptyBitboard;
    return kKnightAttacks[squareIndex(square)];
}

[[nodiscard]] constexpr Bitboard kingAttacks(Square square) noexcept {
    if (!isValidSquare(square)) return kEmptyBitboard;
    return kKingAttacks[squareIndex(square)];
}

/// \brief Computes bishop attacks along 4 diagonal rays until blocked.
/// Includes the first blocker encountered in each direction (friendly or enemy).
[[nodiscard]] constexpr Bitboard bishopAttacks(Square square, Bitboard occupancy) noexcept {
    if (!isValidSquare(square)) return kEmptyBitboard;

    Bitboard attacks = kEmptyBitboard;
    const int sqF = static_cast<int>(fileOf(square));
    const int sqR = static_cast<int>(rankOf(square));

    constexpr int df[4] = {1, -1, 1, -1};
    constexpr int dr[4] = {1, 1, -1, -1};

    for (int dir = 0; dir < 4; ++dir) {
        int f = sqF + df[dir];
        int r = sqR + dr[dir];
        while (f >= 0 && f < 8 && r >= 0 && r < 8) {
            const auto targetSq = static_cast<Square>(r * 8 + f);
            attacks |= bit(targetSq);
            if (testBit(occupancy, targetSq)) {
                break;
            }
            f += df[dir];
            r += dr[dir];
        }
    }
    return attacks;
}

/// \brief Computes rook attacks along 4 cardinal rays until blocked.
/// Includes the first blocker encountered in each direction (friendly or enemy).
[[nodiscard]] constexpr Bitboard rookAttacks(Square square, Bitboard occupancy) noexcept {
    if (!isValidSquare(square)) return kEmptyBitboard;

    Bitboard attacks = kEmptyBitboard;
    const int sqF = static_cast<int>(fileOf(square));
    const int sqR = static_cast<int>(rankOf(square));

    constexpr int df[4] = {0, 0, 1, -1};
    constexpr int dr[4] = {1, -1, 0, 0};

    for (int dir = 0; dir < 4; ++dir) {
        int f = sqF + df[dir];
        int r = sqR + dr[dir];
        while (f >= 0 && f < 8 && r >= 0 && r < 8) {
            const auto targetSq = static_cast<Square>(r * 8 + f);
            attacks |= bit(targetSq);
            if (testBit(occupancy, targetSq)) {
                break;
            }
            f += df[dir];
            r += dr[dir];
        }
    }
    return attacks;
}

/// \brief Computes queen attacks as union of bishop and rook rays.
[[nodiscard]] constexpr Bitboard queenAttacks(Square square, Bitboard occupancy) noexcept {
    return bishopAttacks(square, occupancy) | rookAttacks(square, occupancy);
}

/// \brief Computes all squares attacked by a given side given the full position occupancies.
[[nodiscard]] Bitboard attacksByColor(
    Color attackingColor,
    Bitboard occupancy,
    Bitboard pawns,
    Bitboard knights,
    Bitboard bishops,
    Bitboard rooks,
    Bitboard queens,
    Bitboard kings) noexcept;

/// \brief Tests whether a square is attacked by the specified side.
[[nodiscard]] bool isSquareAttacked(
    Square targetSquare,
    Color attackingColor,
    Bitboard occupancy,
    Bitboard pawns,
    Bitboard knights,
    Bitboard bishops,
    Bitboard rooks,
    Bitboard queens,
    Bitboard kings) noexcept;

}  // namespace attacks
}  // namespace rooksac
