#pragma once

#include <array>
#include <cstdint>
#include <optional>

namespace rooksac {

enum class Color : std::uint8_t {
    White = 0,
    Black = 1,
};

inline constexpr std::size_t kColorCount = 2;

[[nodiscard]] constexpr Color opposite(Color color) noexcept {
    return color == Color::White ? Color::Black : Color::White;
}

enum class PieceType : std::uint8_t {
    None = 0,
    Pawn = 1,
    Knight = 2,
    Bishop = 3,
    Rook = 4,
    Queen = 5,
    King = 6,
};

inline constexpr std::size_t kPieceTypeCount = 6;

enum class Piece : std::uint8_t {
    None = 0,
    WhitePawn = 1,
    WhiteKnight = 2,
    WhiteBishop = 3,
    WhiteRook = 4,
    WhiteQueen = 5,
    WhiteKing = 6,
    BlackPawn = 7,
    BlackKnight = 8,
    BlackBishop = 9,
    BlackRook = 10,
    BlackQueen = 11,
    BlackKing = 12,
};

[[nodiscard]] constexpr bool isValidPieceType(PieceType pieceType) noexcept {
    return pieceType >= PieceType::Pawn && pieceType <= PieceType::King;
}

[[nodiscard]] constexpr Piece makePiece(Color color, PieceType pieceType) noexcept {
    if (!isValidPieceType(pieceType)) {
        return Piece::None;
    }

    return static_cast<Piece>(static_cast<std::uint8_t>(pieceType) +
                              static_cast<std::uint8_t>(color) * kPieceTypeCount);
}

[[nodiscard]] constexpr bool isValidPiece(Piece piece) noexcept {
    return piece != Piece::None;
}

[[nodiscard]] constexpr std::optional<Color> colorOf(Piece piece) noexcept {
    if (!isValidPiece(piece)) {
        return std::nullopt;
    }

    return static_cast<std::uint8_t>(piece) <= kPieceTypeCount ? Color::White : Color::Black;
}

[[nodiscard]] constexpr PieceType typeOf(Piece piece) noexcept {
    if (!isValidPiece(piece)) {
        return PieceType::None;
    }

    const auto encoded = static_cast<std::uint8_t>(piece);
    const auto type = static_cast<std::uint8_t>((encoded - 1U) % kPieceTypeCount + 1U);
    return static_cast<PieceType>(type);
}

enum class File : std::uint8_t {
    A = 0,
    B = 1,
    C = 2,
    D = 3,
    E = 4,
    F = 5,
    G = 6,
    H = 7,
};

enum class Rank : std::uint8_t {
    One = 0,
    Two = 1,
    Three = 2,
    Four = 3,
    Five = 4,
    Six = 5,
    Seven = 6,
    Eight = 7,
};

// A1 is bit zero. Files increase from west to east; ranks increase from south to north.
enum class Square : std::uint8_t {
    A1 = 0,
    B1,
    C1,
    D1,
    E1,
    F1,
    G1,
    H1,
    A2,
    B2,
    C2,
    D2,
    E2,
    F2,
    G2,
    H2,
    A3,
    B3,
    C3,
    D3,
    E3,
    F3,
    G3,
    H3,
    A4,
    B4,
    C4,
    D4,
    E4,
    F4,
    G4,
    H4,
    A5,
    B5,
    C5,
    D5,
    E5,
    F5,
    G5,
    H5,
    A6,
    B6,
    C6,
    D6,
    E6,
    F6,
    G6,
    H6,
    A7,
    B7,
    C7,
    D7,
    E7,
    F7,
    G7,
    H7,
    A8,
    B8,
    C8,
    D8,
    E8,
    F8,
    G8,
    H8,
    None = 64,
};

inline constexpr std::size_t kSquareCount = 64;

[[nodiscard]] constexpr bool isValidSquare(Square square) noexcept {
    return static_cast<std::uint8_t>(square) < kSquareCount;
}

[[nodiscard]] constexpr std::optional<Square> tryMakeSquare(std::uint8_t file,
                                                               std::uint8_t rank) noexcept {
    if (file >= 8 || rank >= 8) {
        return std::nullopt;
    }

    return static_cast<Square>(rank * 8U + file);
}

[[nodiscard]] constexpr Square makeSquare(File file, Rank rank) noexcept {
    return static_cast<Square>(static_cast<std::uint8_t>(rank) * 8U +
                               static_cast<std::uint8_t>(file));
}

[[nodiscard]] constexpr std::uint8_t squareIndex(Square square) noexcept {
    return static_cast<std::uint8_t>(square);
}

// These helpers require a valid square. Call isValidSquare or use tryMakeSquare for untrusted input.
[[nodiscard]] constexpr File fileOf(Square square) noexcept {
    return static_cast<File>(squareIndex(square) % 8U);
}

[[nodiscard]] constexpr Rank rankOf(Square square) noexcept {
    return static_cast<Rank>(squareIndex(square) / 8U);
}

[[nodiscard]] constexpr std::array<char, 2> squareToAlgebraic(Square square) noexcept {
    if (!isValidSquare(square)) {
        return {'?', '?'};
    }

    return {static_cast<char>('a' + static_cast<char>(fileOf(square))),
            static_cast<char>('1' + static_cast<char>(rankOf(square)))};
}

[[nodiscard]] constexpr std::optional<Square> squareFromAlgebraic(char file, char rank) noexcept {
    if (file < 'a' || file > 'h' || rank < '1' || rank > '8') {
        return std::nullopt;
    }

    return tryMakeSquare(static_cast<std::uint8_t>(file - 'a'),
                         static_cast<std::uint8_t>(rank - '1'));
}

[[nodiscard]] constexpr bool isPromotionPieceType(PieceType pieceType) noexcept {
    return pieceType == PieceType::Knight || pieceType == PieceType::Bishop ||
           pieceType == PieceType::Rook || pieceType == PieceType::Queen;
}

}  // namespace rooksac
