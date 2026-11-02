#include "rooksac/position.hpp"

#include <cassert>
#include <cctype>
#include <charconv>
#include <sstream>
#include <vector>

#include "rooksac/zobrist.hpp"

namespace rooksac {

namespace {

[[nodiscard]] constexpr std::optional<Piece> pieceFromChar(char c) noexcept {
    switch (c) {
        case 'P': return Piece::WhitePawn;
        case 'N': return Piece::WhiteKnight;
        case 'B': return Piece::WhiteBishop;
        case 'R': return Piece::WhiteRook;
        case 'Q': return Piece::WhiteQueen;
        case 'K': return Piece::WhiteKing;
        case 'p': return Piece::BlackPawn;
        case 'n': return Piece::BlackKnight;
        case 'b': return Piece::BlackBishop;
        case 'r': return Piece::BlackRook;
        case 'q': return Piece::BlackQueen;
        case 'k': return Piece::BlackKing;
        default: return std::nullopt;
    }
}

[[nodiscard]] constexpr char charFromPiece(Piece piece) noexcept {
    switch (piece) {
        case Piece::WhitePawn: return 'P';
        case Piece::WhiteKnight: return 'N';
        case Piece::WhiteBishop: return 'B';
        case Piece::WhiteRook: return 'R';
        case Piece::WhiteQueen: return 'Q';
        case Piece::WhiteKing: return 'K';
        case Piece::BlackPawn: return 'p';
        case Piece::BlackKnight: return 'n';
        case Piece::BlackBishop: return 'b';
        case Piece::BlackRook: return 'r';
        case Piece::BlackQueen: return 'q';
        case Piece::BlackKing: return 'k';
        case Piece::None: return ' ';
    }
    return ' ';
}

}  // namespace

Position::Position() noexcept {
    clear();
}

Position Position::startPosition() noexcept {
    const auto result = fromFen(kStartPositionFen);
    assert(result.hasValue());
    return *result;
}

void Position::clear() noexcept {
    for (auto& colorPieces : pieceBitboards_) {
        colorPieces.fill(kEmptyBitboard);
    }
    colorOccupancy_.fill(kEmptyBitboard);
    totalOccupancy_ = kEmptyBitboard;
    mailbox_.fill(Piece::None);

    sideToMove_ = Color::White;
    castlingRights_ = CastlingRights::None;
    enPassantSquare_ = Square::None;
    halfmoveClock_ = 0;
    fullmoveNumber_ = 1;
}

void Position::putPiece(Square square, Piece piece) noexcept {
    assert(isValidSquare(square));
    assert(isValidPiece(piece));

    const auto idx = squareIndex(square);
    assert(mailbox_[idx] == Piece::None);

    const auto color = *colorOf(piece);
    const auto type = typeOf(piece);

    mailbox_[idx] = piece;
    setBit(pieceBitboards_[static_cast<std::size_t>(color)][static_cast<std::size_t>(type)], square);
    setBit(colorOccupancy_[static_cast<std::size_t>(color)], square);
    setBit(totalOccupancy_, square);
}

void Position::removePiece(Square square) noexcept {
    assert(isValidSquare(square));

    const auto idx = squareIndex(square);
    const auto piece = mailbox_[idx];
    assert(isValidPiece(piece));

    const auto color = *colorOf(piece);
    const auto type = typeOf(piece);

    mailbox_[idx] = Piece::None;
    clearBit(pieceBitboards_[static_cast<std::size_t>(color)][static_cast<std::size_t>(type)], square);
    clearBit(colorOccupancy_[static_cast<std::size_t>(color)], square);
    clearBit(totalOccupancy_, square);
}

bool Position::checkInvariants() const noexcept {
    Bitboard expectedWhiteOccupancy = kEmptyBitboard;
    Bitboard expectedBlackOccupancy = kEmptyBitboard;

    for (std::size_t t = 1; t <= kPieceTypeCount; ++t) {
        const Bitboard wb = pieceBitboards_[static_cast<std::size_t>(Color::White)][t];
        const Bitboard bb = pieceBitboards_[static_cast<std::size_t>(Color::Black)][t];
        if ((expectedWhiteOccupancy & wb) != 0) return false;
        if ((expectedBlackOccupancy & bb) != 0) return false;
        expectedWhiteOccupancy |= wb;
        expectedBlackOccupancy |= bb;
    }

    if (colorOccupancy_[static_cast<std::size_t>(Color::White)] != expectedWhiteOccupancy) return false;
    if (colorOccupancy_[static_cast<std::size_t>(Color::Black)] != expectedBlackOccupancy) return false;
    if ((expectedWhiteOccupancy & expectedBlackOccupancy) != 0) return false;
    if (totalOccupancy_ != (expectedWhiteOccupancy | expectedBlackOccupancy)) return false;

    for (std::uint8_t i = 0; i < kSquareCount; ++i) {
        const Square sq = static_cast<Square>(i);
        const Piece piece = mailbox_[i];
        if (piece == Piece::None) {
            if (testBit(totalOccupancy_, sq)) return false;
        } else {
            if (!testBit(totalOccupancy_, sq)) return false;
            const auto color = *colorOf(piece);
            const auto type = typeOf(piece);
            if (!testBit(pieceBitboards_[static_cast<std::size_t>(color)][static_cast<std::size_t>(type)], sq)) {
                return false;
            }
        }
    }

    if (enPassantSquare_ != Square::None) {
        if (!isValidSquare(enPassantSquare_)) return false;
        const auto r = rankOf(enPassantSquare_);
        if (sideToMove_ == Color::White && r != Rank::Six) return false;
        if (sideToMove_ == Color::Black && r != Rank::Three) return false;
        if (pieceAt(enPassantSquare_) != Piece::None) return false;
    }

    if (fullmoveNumber_ == 0) return false;

    if (zobristHash_ != zobrist::computeHash(*this)) return false;

    return true;
}

FenResult<Position> Position::fromFen(std::string_view fen) noexcept {
    std::vector<std::string_view> fields;
    std::size_t start = 0;
    while (start < fen.size()) {
        while (start < fen.size() && std::isspace(static_cast<unsigned char>(fen[start]))) {
            ++start;
        }
        if (start >= fen.size()) break;
        std::size_t end = start;
        while (end < fen.size() && !std::isspace(static_cast<unsigned char>(fen[end]))) {
            ++end;
        }
        fields.push_back(fen.substr(start, end - start));
        start = end;
    }

    if (fields.size() != 6) {
        return FenParseError::InvalidFieldCount;
    }

    Position pos;

    // Field 1: Piece placement (starts from rank 8 down to rank 1)
    std::string_view placement = fields[0];
    int rank = 7;
    int file = 0;

    for (char c : placement) {
        if (c == '/') {
            if (file != 8) {
                return FenParseError::RowTooShort;
            }
            --rank;
            file = 0;
            if (rank < 0) {
                return FenParseError::InvalidRowRankCount;
            }
            continue;
        }

        if (c >= '1' && c <= '8') {
            int emptyCount = c - '0';
            file += emptyCount;
            if (file > 8) {
                return FenParseError::RowTooLong;
            }
            continue;
        }

        auto pieceOpt = pieceFromChar(c);
        if (!pieceOpt) {
            return FenParseError::InvalidPieceCharacter;
        }

        if (file >= 8) {
            return FenParseError::RowTooLong;
        }

        const auto sqOpt = tryMakeSquare(static_cast<std::uint8_t>(file), static_cast<std::uint8_t>(rank));
        if (!sqOpt) {
            return FenParseError::InvalidPiecePlacement;
        }

        pos.putPiece(*sqOpt, *pieceOpt);
        ++file;
    }

    if (rank != 0 || file != 8) {
        return rank < 0 ? FenParseError::InvalidRowRankCount : FenParseError::RowTooShort;
    }

    // Field 2: Side to move
    std::string_view stm = fields[1];
    if (stm == "w") {
        pos.sideToMove_ = Color::White;
    } else if (stm == "b") {
        pos.sideToMove_ = Color::Black;
    } else {
        return FenParseError::InvalidSideToMove;
    }

    // Field 3: Castling rights
    std::string_view castling = fields[2];
    CastlingRights cr = CastlingRights::None;
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': cr |= CastlingRights::WhiteKingside; break;
                case 'Q': cr |= CastlingRights::WhiteQueenside; break;
                case 'k': cr |= CastlingRights::BlackKingside; break;
                case 'q': cr |= CastlingRights::BlackQueenside; break;
                default:
                    return FenParseError::InvalidCastlingRights;
            }
        }
    }
    pos.castlingRights_ = cr;

    // Field 4: En-passant square
    std::string_view ep = fields[3];
    if (ep == "-") {
        pos.enPassantSquare_ = Square::None;
    } else {
        if (ep.size() != 2) {
            return FenParseError::InvalidEnPassantSquare;
        }
        auto sqOpt = squareFromAlgebraic(ep[0], ep[1]);
        if (!sqOpt) {
            return FenParseError::InvalidEnPassantSquare;
        }
        const auto r = rankOf(*sqOpt);
        if (pos.sideToMove_ == Color::White && r != Rank::Six) {
            return FenParseError::InvalidEnPassantRank;
        }
        if (pos.sideToMove_ == Color::Black && r != Rank::Three) {
            return FenParseError::InvalidEnPassantRank;
        }
        pos.enPassantSquare_ = *sqOpt;
    }

    // Field 5: Halfmove clock
    std::string_view hm = fields[4];
    std::uint32_t halfmove = 0;
    auto [ptr1, ec1] = std::from_chars(hm.data(), hm.data() + hm.size(), halfmove);
    if (ec1 != std::errc() || ptr1 != hm.data() + hm.size()) {
        return FenParseError::InvalidHalfmoveClock;
    }
    pos.halfmoveClock_ = halfmove;

    // Field 6: Fullmove number
    std::string_view fm = fields[5];
    std::uint32_t fullmove = 0;
    auto [ptr2, ec2] = std::from_chars(fm.data(), fm.data() + fm.size(), fullmove);
    if (ec2 != std::errc() || ptr2 != fm.data() + fm.size() || fullmove == 0) {
        return FenParseError::InvalidFullmoveNumber;
    }
    pos.fullmoveNumber_ = fullmove;
    pos.zobristHash_ = zobrist::computeHash(pos);

    assert(pos.checkInvariants());
    return pos;
}

std::string Position::toFen() const {
    std::string fen;

    // 1. Piece placement
    for (int r = 7; r >= 0; --r) {
        int emptyCount = 0;
        for (int f = 0; f < 8; ++f) {
            const Square sq = makeSquare(static_cast<File>(f), static_cast<Rank>(r));
            const Piece p = pieceAt(sq);
            if (p == Piece::None) {
                ++emptyCount;
            } else {
                if (emptyCount > 0) {
                    fen.push_back(static_cast<char>('0' + emptyCount));
                    emptyCount = 0;
                }
                fen.push_back(charFromPiece(p));
            }
        }
        if (emptyCount > 0) {
            fen.push_back(static_cast<char>('0' + emptyCount));
        }
        if (r > 0) {
            fen.push_back('/');
        }
    }

    // 2. Side to move
    fen.push_back(' ');
    fen.push_back(sideToMove_ == Color::White ? 'w' : 'b');

    // 3. Castling rights
    fen.push_back(' ');
    if (castlingRights_ == CastlingRights::None) {
        fen.push_back('-');
    } else {
        if (hasCastlingRight(castlingRights_, CastlingRights::WhiteKingside)) fen.push_back('K');
        if (hasCastlingRight(castlingRights_, CastlingRights::WhiteQueenside)) fen.push_back('Q');
        if (hasCastlingRight(castlingRights_, CastlingRights::BlackKingside)) fen.push_back('k');
        if (hasCastlingRight(castlingRights_, CastlingRights::BlackQueenside)) fen.push_back('q');
    }

    // 4. En-passant square
    fen.push_back(' ');
    if (enPassantSquare_ == Square::None) {
        fen.push_back('-');
    } else {
        const auto alg = squareToAlgebraic(enPassantSquare_);
        fen.push_back(alg[0]);
        fen.push_back(alg[1]);
    }

    // 5. Halfmove clock
    fen.push_back(' ');
    fen.append(std::to_string(halfmoveClock_));

    // 6. Fullmove number
    fen.push_back(' ');
    fen.append(std::to_string(fullmoveNumber_));

    return fen;
}

bool Position::makeMove(Move move, UndoState& undoState) noexcept {
    const Square from = move.from();
    const Square to = move.to();
    const Piece movingPiece = pieceAt(from);

    if (movingPiece == Piece::None) {
        return false;
    }

    const auto movingColor = *colorOf(movingPiece);
    if (movingColor != sideToMove_) {
        return false;
    }

    const PieceType movingType = typeOf(movingPiece);
    const Piece capturedPiece = pieceAt(to);

    // Save undo state
    undoState.enPassantSquare = enPassantSquare_;
    undoState.castlingRights = castlingRights_;
    undoState.halfmoveClock = halfmoveClock_;
    undoState.capturedPiece = capturedPiece;
    undoState.zobristHash = zobristHash_;

    bool isEnPassantCapture = false;
    Square enPassantPawnSquare = Square::None;

    // Check if this is an en-passant capture
    if (movingType == PieceType::Pawn && to == enPassantSquare_ && capturedPiece == Piece::None) {
        isEnPassantCapture = true;
        enPassantPawnSquare = (movingColor == Color::White)
            ? makeSquare(fileOf(to), Rank::Five)
            : makeSquare(fileOf(to), Rank::Four);
        undoState.capturedPiece = pieceAt(enPassantPawnSquare);
    }

    // Update halfmove clock: reset on pawn move or capture, otherwise increment
    if (movingType == PieceType::Pawn || capturedPiece != Piece::None || isEnPassantCapture) {
        halfmoveClock_ = 0;
    } else {
        ++halfmoveClock_;
    }

    // Fullmove number increments after Black moves
    if (sideToMove_ == Color::Black) {
        ++fullmoveNumber_;
    }

    // Reset en-passant square by default
    enPassantSquare_ = Square::None;

    // If double pawn push, set new en-passant target square
    if (movingType == PieceType::Pawn) {
        const auto fromRank = rankOf(from);
        const auto toRank = rankOf(to);
        if (movingColor == Color::White && fromRank == Rank::Two && toRank == Rank::Four) {
            enPassantSquare_ = makeSquare(fileOf(from), Rank::Three);
        } else if (movingColor == Color::Black && fromRank == Rank::Seven && toRank == Rank::Five) {
            enPassantSquare_ = makeSquare(fileOf(from), Rank::Six);
        }
    }

    // Remove piece from source
    removePiece(from);

    // Remove captured piece if regular capture
    if (capturedPiece != Piece::None) {
        removePiece(to);
    } else if (isEnPassantCapture) {
        removePiece(enPassantPawnSquare);
    }

    // Handle castling king move
    if (movingType == PieceType::King) {
        if (from == Square::E1 && to == Square::G1) {
            removePiece(Square::H1);
            putPiece(Square::F1, Piece::WhiteRook);
        } else if (from == Square::E1 && to == Square::C1) {
            removePiece(Square::A1);
            putPiece(Square::D1, Piece::WhiteRook);
        } else if (from == Square::E8 && to == Square::G8) {
            removePiece(Square::H8);
            putPiece(Square::F8, Piece::BlackRook);
        } else if (from == Square::E8 && to == Square::C8) {
            removePiece(Square::A8);
            putPiece(Square::D8, Piece::BlackRook);
        }
    }

    // Put moving piece (or promoted piece) onto destination
    Piece pieceToPlace = movingPiece;
    if (move.promotion() != PieceType::None) {
        pieceToPlace = makePiece(movingColor, move.promotion());
    }
    putPiece(to, pieceToPlace);

    // Update castling rights if king or rook moved, or rook was captured
    if (movingType == PieceType::King) {
        if (movingColor == Color::White) {
            castlingRights_ &= ~CastlingRights::WhiteBoth;
        } else {
            castlingRights_ &= ~CastlingRights::BlackBoth;
        }
    }

    if (from == Square::A1 || to == Square::A1) {
        castlingRights_ &= ~CastlingRights::WhiteQueenside;
    }
    if (from == Square::H1 || to == Square::H1) {
        castlingRights_ &= ~CastlingRights::WhiteKingside;
    }
    if (from == Square::A8 || to == Square::A8) {
        castlingRights_ &= ~CastlingRights::BlackQueenside;
    }
    if (from == Square::H8 || to == Square::H8) {
        castlingRights_ &= ~CastlingRights::BlackKingside;
    }

    // Flip side to move
    sideToMove_ = opposite(sideToMove_);

    // Incremental Zobrist update
    const auto& ztable = zobrist::getTable();
    // 1. Moving piece from -> to (or promotion piece)
    zobristHash_ ^= ztable.pieceSquare[static_cast<std::size_t>(movingPiece)][squareIndex(from)];
    zobristHash_ ^= ztable.pieceSquare[static_cast<std::size_t>(pieceToPlace)][squareIndex(to)];

    // 2. Captured piece removal
    if (capturedPiece != Piece::None) {
        zobristHash_ ^= ztable.pieceSquare[static_cast<std::size_t>(capturedPiece)][squareIndex(to)];
    } else if (isEnPassantCapture) {
        zobristHash_ ^= ztable.pieceSquare[static_cast<std::size_t>(undoState.capturedPiece)][squareIndex(enPassantPawnSquare)];
    }

    // 3. Castling rook move
    if (movingType == PieceType::King) {
        if (from == Square::E1 && to == Square::G1) {
            zobristHash_ ^= ztable.pieceSquare[static_cast<std::size_t>(Piece::WhiteRook)][squareIndex(Square::H1)];
            zobristHash_ ^= ztable.pieceSquare[static_cast<std::size_t>(Piece::WhiteRook)][squareIndex(Square::F1)];
        } else if (from == Square::E1 && to == Square::C1) {
            zobristHash_ ^= ztable.pieceSquare[static_cast<std::size_t>(Piece::WhiteRook)][squareIndex(Square::A1)];
            zobristHash_ ^= ztable.pieceSquare[static_cast<std::size_t>(Piece::WhiteRook)][squareIndex(Square::D1)];
        } else if (from == Square::E8 && to == Square::G8) {
            zobristHash_ ^= ztable.pieceSquare[static_cast<std::size_t>(Piece::BlackRook)][squareIndex(Square::H8)];
            zobristHash_ ^= ztable.pieceSquare[static_cast<std::size_t>(Piece::BlackRook)][squareIndex(Square::F8)];
        } else if (from == Square::E8 && to == Square::C8) {
            zobristHash_ ^= ztable.pieceSquare[static_cast<std::size_t>(Piece::BlackRook)][squareIndex(Square::A8)];
            zobristHash_ ^= ztable.pieceSquare[static_cast<std::size_t>(Piece::BlackRook)][squareIndex(Square::D8)];
        }
    }

    // 4. Side to move flip
    zobristHash_ ^= ztable.sideToMove;

    // 5. Castling rights change
    if (castlingRights_ != undoState.castlingRights) {
        const auto oldC = static_cast<std::size_t>(static_cast<std::uint8_t>(undoState.castlingRights)) & 0x0FU;
        const auto newC = static_cast<std::size_t>(static_cast<std::uint8_t>(castlingRights_)) & 0x0FU;
        zobristHash_ ^= ztable.castling[oldC];
        zobristHash_ ^= ztable.castling[newC];
    }

    // 6. En-passant square change
    if (undoState.enPassantSquare != Square::None) {
        zobristHash_ ^= ztable.enPassantFile[static_cast<std::size_t>(fileOf(undoState.enPassantSquare))];
    }
    if (enPassantSquare_ != Square::None) {
        zobristHash_ ^= ztable.enPassantFile[static_cast<std::size_t>(fileOf(enPassantSquare_))];
    }

    assert(checkInvariants());
    return true;
}

void Position::unmakeMove(Move move, const UndoState& undoState) noexcept {
    // Flip side to move back to mover
    sideToMove_ = opposite(sideToMove_);
    const Color movingColor = sideToMove_;

    const Square from = move.from();
    const Square to = move.to();
    const Piece placedPiece = pieceAt(to);
    assert(placedPiece != Piece::None);

    const PieceType placedType = typeOf(placedPiece);

    // Fullmove number decrements if Black was the moving side
    if (movingColor == Color::Black) {
        --fullmoveNumber_;
    }

    // Remove the piece from target square
    removePiece(to);

    // Check if this move was castling
    if (placedType == PieceType::King) {
        if (from == Square::E1 && to == Square::G1) {
            removePiece(Square::F1);
            putPiece(Square::H1, Piece::WhiteRook);
        } else if (from == Square::E1 && to == Square::C1) {
            removePiece(Square::D1);
            putPiece(Square::A1, Piece::WhiteRook);
        } else if (from == Square::E8 && to == Square::G8) {
            removePiece(Square::F8);
            putPiece(Square::H8, Piece::BlackRook);
        } else if (from == Square::E8 && to == Square::C8) {
            removePiece(Square::D8);
            putPiece(Square::A8, Piece::BlackRook);
        }
    }

    // Reconstruct originally moved piece (undo promotion if promoted)
    Piece originalMovingPiece = placedPiece;
    if (move.promotion() != PieceType::None) {
        originalMovingPiece = makePiece(movingColor, PieceType::Pawn);
    }
    putPiece(from, originalMovingPiece);

    // Restore captured piece if any
    if (undoState.capturedPiece != Piece::None) {
        // Check if it was en-passant
        if (typeOf(originalMovingPiece) == PieceType::Pawn && to == undoState.enPassantSquare) {
            const Square enPassantPawnSquare = (movingColor == Color::White)
                ? makeSquare(fileOf(to), Rank::Five)
                : makeSquare(fileOf(to), Rank::Four);
            putPiece(enPassantPawnSquare, undoState.capturedPiece);
        } else {
            putPiece(to, undoState.capturedPiece);
        }
    }

    // Restore prior reversible state
    enPassantSquare_ = undoState.enPassantSquare;
    castlingRights_ = undoState.castlingRights;
    halfmoveClock_ = undoState.halfmoveClock;
    zobristHash_ = undoState.zobristHash;

    assert(checkInvariants());
}

void Position::makeNullMove(UndoState& undoState) noexcept {
    // Save reversible state (no piece moves, but en-passant and half-move clock change)
    undoState.enPassantSquare = enPassantSquare_;
    undoState.castlingRights = castlingRights_;
    undoState.halfmoveClock = halfmoveClock_;
    undoState.capturedPiece = Piece::None;
    undoState.zobristHash = zobristHash_;

    const zobrist::ZobristTable& ztable = zobrist::getTable();

    // Remove the en-passant file key (if any) before clearing en passant
    if (enPassantSquare_ != Square::None) {
        zobristHash_ ^= ztable.enPassantFile[static_cast<std::size_t>(fileOf(enPassantSquare_))];
    }

    // Clear en-passant: passing the turn means no en-passant opportunity
    enPassantSquare_ = Square::None;

    // Increment half-move clock (null move is not a pawn move or capture)
    ++halfmoveClock_;

    // Fullmove number increments after Black moves
    if (sideToMove_ == Color::Black) {
        ++fullmoveNumber_;
    }

    // Flip side-to-move and update Zobrist key
    sideToMove_ = opposite(sideToMove_);
    zobristHash_ ^= ztable.sideToMove;
}

void Position::unmakeNullMove(const UndoState& undoState) noexcept {
    // Flip side-to-move back
    sideToMove_ = opposite(sideToMove_);

    // Restore fullmove number if Black was the side-to-move before null move
    if (sideToMove_ == Color::Black) {
        --fullmoveNumber_;
    }

    // Restore all reversible state (including zobristHash_)
    enPassantSquare_ = undoState.enPassantSquare;
    castlingRights_ = undoState.castlingRights;
    halfmoveClock_ = undoState.halfmoveClock;
    zobristHash_ = undoState.zobristHash;
}

}  // namespace rooksac
