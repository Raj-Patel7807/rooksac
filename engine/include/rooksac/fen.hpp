#pragma once

#include <optional>
#include <string_view>
#include <utility>

namespace rooksac {

enum class FenParseError {
    InvalidFieldCount,
    InvalidPiecePlacement,
    InvalidRowRankCount,
    InvalidPieceCharacter,
    ConsecutiveOrInvalidDigits,
    RowTooShort,
    RowTooLong,
    InvalidSideToMove,
    InvalidCastlingRights,
    InvalidEnPassantSquare,
    InvalidEnPassantRank,
    InvalidHalfmoveClock,
    InvalidFullmoveNumber,
};

[[nodiscard]] constexpr std::string_view fenParseErrorMessage(FenParseError error) noexcept {
    switch (error) {
        case FenParseError::InvalidFieldCount:
            return "FEN string must contain exactly 6 whitespace-delimited fields";
        case FenParseError::InvalidPiecePlacement:
            return "FEN piece placement must contain 8 ranks separated by '/'";
        case FenParseError::InvalidRowRankCount:
            return "FEN piece placement rank count does not equal 8";
        case FenParseError::InvalidPieceCharacter:
            return "FEN piece placement contains an unexpected piece character";
        case FenParseError::ConsecutiveOrInvalidDigits:
            return "FEN piece placement contains invalid empty square counts";
        case FenParseError::RowTooShort:
            return "FEN rank contains fewer than 8 squares";
        case FenParseError::RowTooLong:
            return "FEN rank contains more than 8 squares";
        case FenParseError::InvalidSideToMove:
            return "FEN side to move must be 'w' or 'b'";
        case FenParseError::InvalidCastlingRights:
            return "FEN castling rights must be '-' or a combination of [KQkq]";
        case FenParseError::InvalidEnPassantSquare:
            return "FEN en-passant square must be '-' or valid algebraic square";
        case FenParseError::InvalidEnPassantRank:
            return "FEN en-passant square must be on rank 3 (for black target) or rank 6 (for white target)";
        case FenParseError::InvalidHalfmoveClock:
            return "FEN halfmove clock must be a non-negative integer";
        case FenParseError::InvalidFullmoveNumber:
            return "FEN fullmove number must be a positive integer (>= 1)";
    }
    return "Unknown FEN parse error";
}

template <typename T>
class FenResult {
public:
    constexpr FenResult(T val) noexcept : value_(std::move(val)), hasValue_(true), error_{} {}
    constexpr FenResult(FenParseError err) noexcept : value_{}, hasValue_(false), error_(err) {}

    [[nodiscard]] constexpr bool hasValue() const noexcept { return hasValue_; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return hasValue_; }

    [[nodiscard]] constexpr const T& value() const& noexcept { return *value_; }
    [[nodiscard]] constexpr T& value() & noexcept { return *value_; }
    [[nodiscard]] constexpr const T* operator->() const noexcept { return &*value_; }
    [[nodiscard]] constexpr T* operator->() noexcept { return &*value_; }
    [[nodiscard]] constexpr const T& operator*() const& noexcept { return *value_; }
    [[nodiscard]] constexpr T& operator*() & noexcept { return *value_; }

    [[nodiscard]] constexpr FenParseError error() const noexcept { return error_; }

private:
    std::optional<T> value_;
    bool hasValue_;
    FenParseError error_;
};

}  // namespace rooksac
