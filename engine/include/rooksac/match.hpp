#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "rooksac/agent.hpp"
#include "rooksac/legal_movegen.hpp"
#include "rooksac/position.hpp"

namespace rooksac {

struct MatchResult {
    GameResult result = GameResult::Ongoing;
    std::optional<Color> winner;
    std::uint32_t totalPlies = 0;
    std::string terminationReason;
    std::string finalFen;
    std::vector<Move> moveHistory;
};

/// \brief Plays a match between two agents starting from the given position until termination or maxPlies.
/// Both agents are guaranteed safe execution with make/unmake and legal move constraints.
[[nodiscard]] MatchResult playMatch(
    Agent& whiteAgent,
    Agent& blackAgent,
    Position initialPosition = Position::startPosition(),
    std::uint32_t maxPlies = 200);

}  // namespace rooksac

