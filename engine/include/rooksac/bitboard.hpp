#pragma once

#include <bit>
#include <cstdint>
#include <iterator>

#include "rooksac/types.hpp"

namespace rooksac {

using Bitboard = std::uint64_t;

inline constexpr Bitboard kEmptyBitboard = 0;
inline constexpr Bitboard kFullBitboard = ~Bitboard{0};

inline constexpr Bitboard kFileA = 0x0101010101010101ULL;
inline constexpr Bitboard kFileB = kFileA << 1U;
inline constexpr Bitboard kFileC = kFileA << 2U;
inline constexpr Bitboard kFileD = kFileA << 3U;
inline constexpr Bitboard kFileE = kFileA << 4U;
inline constexpr Bitboard kFileF = kFileA << 5U;
inline constexpr Bitboard kFileG = kFileA << 6U;
inline constexpr Bitboard kFileH = kFileA << 7U;

inline constexpr Bitboard kRank1 = 0x00000000000000FFULL;
inline constexpr Bitboard kRank2 = kRank1 << 8U;
inline constexpr Bitboard kRank3 = kRank1 << 16U;
inline constexpr Bitboard kRank4 = kRank1 << 24U;
inline constexpr Bitboard kRank5 = kRank1 << 32U;
inline constexpr Bitboard kRank6 = kRank1 << 40U;
inline constexpr Bitboard kRank7 = kRank1 << 48U;
inline constexpr Bitboard kRank8 = kRank1 << 56U;

inline constexpr Bitboard kQueenside = kFileA | kFileB | kFileC | kFileD;
inline constexpr Bitboard kKingside = kFileE | kFileF | kFileG | kFileH;
inline constexpr Bitboard kBoardEdge = kFileA | kFileH | kRank1 | kRank8;
inline constexpr Bitboard kDarkSquares = 0xAA55AA55AA55AA55ULL;
inline constexpr Bitboard kLightSquares = ~kDarkSquares;

[[nodiscard]] constexpr Bitboard bit(Square square) noexcept {
    return isValidSquare(square) ? Bitboard{1} << squareIndex(square) : kEmptyBitboard;
}

[[nodiscard]] constexpr bool testBit(Bitboard board, Square square) noexcept {
    return (board & bit(square)) != 0;
}

constexpr void setBit(Bitboard& board, Square square) noexcept {
    board |= bit(square);
}

constexpr void clearBit(Bitboard& board, Square square) noexcept {
    board &= ~bit(square);
}

[[nodiscard]] constexpr int popCount(Bitboard board) noexcept {
    return std::popcount(board);
}

[[nodiscard]] constexpr Square leastSignificantSquare(Bitboard board) noexcept {
    if (board == kEmptyBitboard) {
        return Square::None;
    }

    return static_cast<Square>(std::countr_zero(board));
}

// The empty-board check ensures countr_zero is never invoked with an invalid precondition.
constexpr Square popLeastSignificantBit(Bitboard& board) noexcept {
    const Square square = leastSignificantSquare(board);
    if (square != Square::None) {
        board &= board - 1U;
    }
    return square;
}

class BitboardIterator {
public:
    explicit constexpr BitboardIterator(Bitboard remaining) noexcept : remaining_(remaining) {}

    [[nodiscard]] constexpr Square operator*() const noexcept {
        return leastSignificantSquare(remaining_);
    }

    constexpr BitboardIterator& operator++() noexcept {
        popLeastSignificantBit(remaining_);
        return *this;
    }

    [[nodiscard]] constexpr bool operator==(std::default_sentinel_t) const noexcept {
        return remaining_ == kEmptyBitboard;
    }

private:
    Bitboard remaining_;
};

class BitboardSquares {
public:
    explicit constexpr BitboardSquares(Bitboard board) noexcept : board_(board) {}

    [[nodiscard]] constexpr BitboardIterator begin() const noexcept {
        return BitboardIterator(board_);
    }

    [[nodiscard]] constexpr std::default_sentinel_t end() const noexcept {
        return {};
    }

private:
    Bitboard board_;
};

}  // namespace rooksac
