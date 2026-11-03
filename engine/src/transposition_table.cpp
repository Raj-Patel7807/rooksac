#include "rooksac/transposition_table.hpp"

#include <algorithm>
#include <cstdint>

#include "rooksac/search.hpp"

namespace rooksac {
namespace tt {

TranspositionTable::TranspositionTable(std::size_t sizeBytes) {
    resize(sizeBytes);
}

void TranspositionTable::resize(std::size_t sizeBytes) {
    const std::size_t clampedSize = std::clamp(sizeBytes, kMinSizeBytes, kMaxSizeBytes);
    const std::size_t count = clampedSize / sizeof(Entry);
    entries_.assign(count, Entry{});
    currentAge_ = 0;
}

void TranspositionTable::clear() noexcept {
    std::fill(entries_.begin(), entries_.end(), Entry{});
    currentAge_ = 0;
}

std::int32_t TranspositionTable::scoreToTT(std::int32_t score, std::uint32_t ply) noexcept {
    if (score >= search::kMateScore - search::kMaxMatePly) {
        return score + static_cast<std::int32_t>(ply);
    }
    if (score <= -search::kMateScore + search::kMaxMatePly) {
        return score - static_cast<std::int32_t>(ply);
    }
    return score;
}

std::int32_t TranspositionTable::scoreFromTT(std::int32_t score, std::uint32_t ply) noexcept {
    if (score >= search::kMateScore - search::kMaxMatePly) {
        return score - static_cast<std::int32_t>(ply);
    }
    if (score <= -search::kMateScore + search::kMaxMatePly) {
        return score + static_cast<std::int32_t>(ply);
    }
    return score;
}

ProbeResult TranspositionTable::probe(std::uint64_t key, std::uint32_t ply) const noexcept {
    ProbeResult res;
    if (entries_.empty()) {
        return res;
    }

    const std::size_t idx = static_cast<std::size_t>(key % entries_.size());
    const Entry& entry = entries_[idx];

    // Verify key match and non-empty bound
    if (entry.key == key && entry.bound != Bound::None) {
        res.hit = true;
        res.score = scoreFromTT(static_cast<std::int32_t>(entry.score), ply);
        res.depth = entry.depth;
        res.bound = entry.bound;
        if (entry.move != 0) {
            // Reconstruct move from encoded bits
            const auto m = Move::create(
                static_cast<Square>(entry.move & 0x003FU),
                static_cast<Square>((entry.move >> 6U) & 0x003FU));
            if (m.has_value()) {
                // If promotion encoded
                const auto promoCode = static_cast<std::uint16_t>((entry.move >> 12U) & 0x0007U);
                PieceType pt = PieceType::None;
                if (promoCode == 1U) pt = PieceType::Knight;
                else if (promoCode == 2U) pt = PieceType::Bishop;
                else if (promoCode == 3U) pt = PieceType::Rook;
                else if (promoCode == 4U) pt = PieceType::Queen;

                res.move = Move::create(m->from(), m->to(), pt);
            }
        }
    }

    return res;
}

void TranspositionTable::store(
    std::uint64_t key,
    std::uint32_t ply,
    std::uint32_t depth,
    std::int32_t score,
    Bound bound,
    std::optional<Move> bestMove) noexcept {
    if (entries_.empty()) {
        return;
    }

    const std::size_t idx = static_cast<std::size_t>(key % entries_.size());
    Entry& entry = entries_[idx];

    // Replacement scheme:
    // 1. If slot is empty or matches key, replace.
    // 2. If new search depth >= stored depth, replace.
    // 3. If stored entry is from an older generation, replace.
    bool shouldReplace = (entry.bound == Bound::None) ||
                         (entry.key == key) ||
                         (depth >= entry.depth) ||
                         (entry.age != currentAge_);

    if (!shouldReplace) {
        // If we didn't replace, we can still preserve/update the best move if the key matches
        if (entry.key == key && bestMove.has_value()) {
            entry.move = bestMove->encoded();
        }
        return;
    }

    // Save whether the incoming key matches the existing slot key BEFORE overwriting.
    // After entry.key = key the comparison entry.key != key is always false, so the
    // stale-move guard must be evaluated first.
    const bool samePosition = (entry.key == key);
    entry.key = key;
    if (bestMove.has_value()) {
        entry.move = bestMove->encoded();
    } else if (!samePosition) {
        // Different position is displacing this slot: clear any stale move from the prior entry.
        entry.move = 0;
    }

    const std::int32_t normalizedScore = scoreToTT(score, ply);
    entry.score = static_cast<std::int16_t>(std::clamp(normalizedScore, -32767, 32767));
    entry.depth = static_cast<std::uint8_t>(std::min(depth, 255U));
    entry.bound = bound;
    entry.age = currentAge_;
}

std::uint32_t TranspositionTable::permillFull() const noexcept {
    if (entries_.empty()) {
        return 0;
    }

    // Sample up to 1000 entries to estimate fullness quickly
    constexpr std::size_t kSampleSize = 1000;
    const std::size_t n = std::min(entries_.size(), kSampleSize);
    std::size_t filled = 0;
    for (std::size_t i = 0; i < n; ++i) {
        if (entries_[i].bound != Bound::None) {
            ++filled;
        }
    }

    return static_cast<std::uint32_t>((filled * 1000U) / n);
}

}  // namespace tt
}  // namespace rooksac
