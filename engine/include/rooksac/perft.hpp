#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "rooksac/move.hpp"
#include "rooksac/position.hpp"

namespace rooksac {
namespace perft {

struct DivideEntry {
    Move move;
    std::uint64_t nodeCount;
};

struct PerftResult {
    std::uint64_t totalNodes = 0;
    std::vector<DivideEntry> divideEntries;
};

/// \brief Recursively counts total leaf positions reachable at the given depth.
/// \param position The position to evaluate.
/// \param depth The search depth (0 returns 1 node).
[[nodiscard]] std::uint64_t perft(Position& position, int depth) noexcept;

/// \brief Executes perft and collects divide results for each legal root move.
[[nodiscard]] PerftResult divide(Position& position, int depth);

/// \brief Formats divide results into a human-readable string.
[[nodiscard]] std::string formatDivide(const PerftResult& result);

}  // namespace perft
}  // namespace rooksac
