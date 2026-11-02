#pragma once

#include <array>
#include <cstdint>

#include "rooksac/castling.hpp"
#include "rooksac/types.hpp"

namespace rooksac {

class Position;

namespace zobrist {

/// \brief Seed used to initialize the deterministic pseudo-random key tables.
inline constexpr std::uint64_t kDefaultSeed = 0x180f42b3917a26c5ULL;

/// \brief Precomputed deterministic 64-bit random values for Zobrist hashing.
struct ZobristTable {
    // [Piece][Square] where Piece is 0..12
    std::array<std::array<std::uint64_t, kSquareCount>, 13> pieceSquare{};
    std::uint64_t sideToMove{};
    // [CastlingRights mask (0..15)]
    std::array<std::uint64_t, 16> castling{};
    // [File (0..7)] for en-passant target files
    std::array<std::uint64_t, 8> enPassantFile{};
};

/// \brief Returns the singleton global Zobrist random table.
[[nodiscard]] const ZobristTable& getTable() noexcept;

/// \brief Computes a complete 64-bit Zobrist key from scratch from a Position.
/// Used for initial state evaluation, verification, and debugging.
[[nodiscard]] std::uint64_t computeHash(const Position& position) noexcept;

}  // namespace zobrist
}  // namespace rooksac
