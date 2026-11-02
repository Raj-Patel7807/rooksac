#include "rooksac/zobrist.hpp"

#include "rooksac/position.hpp"

namespace rooksac {
namespace zobrist {

namespace {

// Deterministic 64-bit SplitMix64 generator to produce pseudo-random numbers
class SplitMix64 {
public:
    constexpr explicit SplitMix64(std::uint64_t seed) noexcept : state_(seed) {}

    constexpr std::uint64_t next() noexcept {
        std::uint64_t z = (state_ += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }

private:
    std::uint64_t state_;
};

ZobristTable initTable() noexcept {
    ZobristTable table;
    SplitMix64 rng(kDefaultSeed);

    for (std::size_t piece = 1; piece <= 12; ++piece) {
        for (std::size_t sq = 0; sq < kSquareCount; ++sq) {
            table.pieceSquare[piece][sq] = rng.next();
        }
    }

    table.sideToMove = rng.next();

    for (std::size_t c = 0; c < 16; ++c) {
        table.castling[c] = rng.next();
    }

    for (std::size_t f = 0; f < 8; ++f) {
        table.enPassantFile[f] = rng.next();
    }

    return table;
}

}  // namespace

const ZobristTable& getTable() noexcept {
    static const ZobristTable kTable = initTable();
    return kTable;
}

std::uint64_t computeHash(const Position& position) noexcept {
    const auto& table = getTable();
    std::uint64_t hash = 0;

    // 1. Piece-square occupancy
    for (std::size_t sq = 0; sq < kSquareCount; ++sq) {
        const Piece piece = position.pieceAt(static_cast<Square>(sq));
        if (piece != Piece::None) {
            hash ^= table.pieceSquare[static_cast<std::size_t>(piece)][sq];
        }
    }

    // 2. Side to move (XOR if Black)
    if (position.sideToMove() == Color::Black) {
        hash ^= table.sideToMove;
    }

    // 3. Castling rights
    const auto castlingIndex = static_cast<std::size_t>(static_cast<std::uint8_t>(position.castlingRights()));
    hash ^= table.castling[castlingIndex & 0x0FU];

    // 4. En-passant square
    const Square ep = position.enPassantSquare();
    if (ep != Square::None) {
        const auto f = static_cast<std::size_t>(fileOf(ep));
        hash ^= table.enPassantFile[f];
    }

    return hash;
}

}  // namespace zobrist
}  // namespace rooksac
