#pragma once

#include <cstdint>

#include "rooksac/castling.hpp"
#include "rooksac/types.hpp"

namespace rooksac {

struct UndoState {
    Square enPassantSquare = Square::None;
    CastlingRights castlingRights = CastlingRights::None;
    std::uint32_t halfmoveClock = 0;
    Piece capturedPiece = Piece::None;
};

}  // namespace rooksac
