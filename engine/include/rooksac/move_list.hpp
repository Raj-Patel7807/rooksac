#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <initializer_list>

#include "rooksac/move.hpp"

namespace rooksac {

/// \brief Bounded, stack-allocated container for moves to prevent heap allocations
/// in performance-critical move generation and search paths.
///
/// Theoretical maximum pseudo-legal moves in a standard chess position is 218.
/// A fixed capacity of 256 provides ample safety margin while fitting cleanly in cache.
class MoveList {
public:
    static constexpr std::size_t kMaxCapacity = 256;

    constexpr MoveList() noexcept : size_(0) {}

    constexpr void pushBack(Move move) noexcept {
        assert(size_ < kMaxCapacity && "MoveList capacity overflow");
        if (size_ < kMaxCapacity) {
            moves_[size_++] = move;
        }
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept { return size_; }
    [[nodiscard]] constexpr bool empty() const noexcept { return size_ == 0; }
    constexpr void clear() noexcept { size_ = 0; }

    [[nodiscard]] constexpr Move operator[](std::size_t index) const noexcept {
        assert(index < size_);
        return moves_[index];
    }

    [[nodiscard]] constexpr Move& operator[](std::size_t index) noexcept {
        assert(index < size_);
        return moves_[index];
    }

    [[nodiscard]] constexpr const Move* begin() const noexcept { return moves_.data(); }
    [[nodiscard]] constexpr const Move* end() const noexcept { return moves_.data() + size_; }
    [[nodiscard]] constexpr Move* begin() noexcept { return moves_.data(); }
    [[nodiscard]] constexpr Move* end() noexcept { return moves_.data() + size_; }

    [[nodiscard]] constexpr bool contains(Move move) const noexcept {
        for (std::size_t i = 0; i < size_; ++i) {
            if (moves_[i].encoded() == move.encoded()) {
                return true;
            }
        }
        return false;
    }

private:
    std::array<Move, kMaxCapacity> moves_{};
    std::size_t size_{0};
};

}  // namespace rooksac
