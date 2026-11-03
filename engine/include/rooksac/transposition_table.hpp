#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "rooksac/move.hpp"

namespace rooksac {
namespace tt {

/// \brief Bound types indicating whether the stored score is exact or a cutoff bound.
enum class Bound : std::uint8_t {
    None = 0,
    Exact = 1,  ///< Score was within [alpha, beta] window (PV node)
    Lower = 2,  ///< Score caused beta cutoff (>= beta, fail-high)
    Upper = 3,  ///< All moves failed low (<= alpha, fail-low)
};

/// \brief A single entry in the Transposition Table.
/// Packed into 16 bytes:
/// - 8 bytes: key (full 64-bit verification key)
/// - 2 bytes: move (encoded 16-bit Move)
/// - 2 bytes: score (16-bit signed integer in centipawns or mate bounds)
/// - 1 byte: depth (search depth in plies)
/// - 1 byte: bound (Exact, Lower, Upper)
/// - 1 byte: age / generation
/// - 1 byte: padding/reserved
struct alignas(16) Entry {
    std::uint64_t key = 0;
    std::uint16_t move = 0;
    std::int16_t score = 0;
    std::uint8_t depth = 0;
    Bound bound = Bound::None;
    std::uint8_t age = 0;
    std::uint8_t reserved = 0;
};

static_assert(sizeof(Entry) == 16, "Transposition table Entry must be exactly 16 bytes");

/// \brief Transposition table lookup result.
struct ProbeResult {
    bool hit = false;
    std::int32_t score = 0;
    std::uint32_t depth = 0;
    Bound bound = Bound::None;
    std::optional<Move> move{};
};

/// \brief Fixed-size memory-bounded Transposition Table.
class TranspositionTable {
public:
    static constexpr std::size_t kDefaultSizeBytes = 64 * 1024 * 1024;  ///< Default 64 MB
    static constexpr std::size_t kMinSizeBytes = 1024 * 1024;           ///< Minimum 1 MB
    static constexpr std::size_t kMaxSizeBytes = 1024ULL * 1024 * 1024; ///< Maximum 1024 MB

    explicit TranspositionTable(std::size_t sizeBytes = kDefaultSizeBytes);

    /// \brief Resizes the transposition table and clears its contents.
    void resize(std::size_t sizeBytes);

    /// \brief Clears all entries in the table.
    void clear() noexcept;

    /// \brief Advances the table age/generation (called at the start of a new search or iteration).
    void advanceAge() noexcept { ++currentAge_; }

    /// \brief Probes the table for a given position key.
    /// Adjusts stored mate distance relative to the current search ply.
    [[nodiscard]] ProbeResult probe(std::uint64_t key, std::uint32_t ply) const noexcept;

    /// \brief Stores an entry in the table according to depth-preferred replacement.
    /// Normalizes mate scores to root-relative distance before storage.
    void store(
        std::uint64_t key,
        std::uint32_t ply,
        std::uint32_t depth,
        std::int32_t score,
        Bound bound,
        std::optional<Move> bestMove) noexcept;

    /// \brief Normalizes root-relative mate scores to ply-independent distance for storage.
    [[nodiscard]] static std::int32_t scoreToTT(std::int32_t score, std::uint32_t ply) noexcept;

    /// \brief Converts ply-independent stored mate scores to root-relative distance for search.
    [[nodiscard]] static std::int32_t scoreFromTT(std::int32_t score, std::uint32_t ply) noexcept;

    /// \brief Returns current capacity (number of entries).
    [[nodiscard]] std::size_t entryCount() const noexcept { return entries_.size(); }

    /// \brief Returns memory size in megabytes.
    [[nodiscard]] std::size_t sizeMb() const noexcept { return (entries_.size() * sizeof(Entry)) / (1024 * 1024); }

    /// \brief Returns hash fullness in permill (per thousand, 0..1000).
    [[nodiscard]] std::uint32_t permillFull() const noexcept;

private:
    std::vector<Entry> entries_{};
    std::uint8_t currentAge_{0};
};

}  // namespace tt
}  // namespace rooksac
