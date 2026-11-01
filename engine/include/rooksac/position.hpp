#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

#include "rooksac/bitboard.hpp"
#include "rooksac/castling.hpp"
#include "rooksac/fen.hpp"
#include "rooksac/move.hpp"
#include "rooksac/types.hpp"
#include "rooksac/undo_state.hpp"

namespace rooksac {

class Position {
public:
    static constexpr std::string_view kStartPositionFen =
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Position() noexcept;

    [[nodiscard]] static FenResult<Position> fromFen(std::string_view fen) noexcept;
    [[nodiscard]] static Position startPosition() noexcept;

    [[nodiscard]] std::string toFen() const;

    [[nodiscard]] Color sideToMove() const noexcept { return sideToMove_; }
    [[nodiscard]] CastlingRights castlingRights() const noexcept { return castlingRights_; }
    [[nodiscard]] Square enPassantSquare() const noexcept { return enPassantSquare_; }
    [[nodiscard]] std::uint32_t halfmoveClock() const noexcept { return halfmoveClock_; }
    [[nodiscard]] std::uint32_t fullmoveNumber() const noexcept { return fullmoveNumber_; }

    [[nodiscard]] Bitboard pieces(Color color, PieceType type) const noexcept {
        return pieceBitboards_[static_cast<std::size_t>(color)][static_cast<std::size_t>(type)];
    }

    [[nodiscard]] Bitboard pieces(Piece piece) const noexcept {
        if (!isValidPiece(piece)) {
            return kEmptyBitboard;
        }
        const auto color = *colorOf(piece);
        const auto type = typeOf(piece);
        return pieces(color, type);
    }

    [[nodiscard]] Bitboard colorOccupancy(Color color) const noexcept {
        return colorOccupancy_[static_cast<std::size_t>(color)];
    }

    [[nodiscard]] Bitboard totalOccupancy() const noexcept {
        return totalOccupancy_;
    }

    [[nodiscard]] Piece pieceAt(Square square) const noexcept {
        if (!isValidSquare(square)) {
            return Piece::None;
        }
        return mailbox_[squareIndex(square)];
    }

    [[nodiscard]] bool makeMove(Move move, UndoState& undoState) noexcept;
    void unmakeMove(Move move, const UndoState& undoState) noexcept;

    [[nodiscard]] bool checkInvariants() const noexcept;

private:
    void clear() noexcept;
    void putPiece(Square square, Piece piece) noexcept;
    void removePiece(Square square) noexcept;

    std::array<std::array<Bitboard, kPieceTypeCount + 1>, kColorCount> pieceBitboards_{};
    std::array<Bitboard, kColorCount> colorOccupancy_{};
    Bitboard totalOccupancy_{0};
    std::array<Piece, kSquareCount> mailbox_{};

    Color sideToMove_{Color::White};
    CastlingRights castlingRights_{CastlingRights::None};
    Square enPassantSquare_{Square::None};
    std::uint32_t halfmoveClock_{0};
    std::uint32_t fullmoveNumber_{1};
};

}  // namespace rooksac
