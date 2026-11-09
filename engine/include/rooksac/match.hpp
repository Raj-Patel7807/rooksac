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

    /// \brief Formats the match into standard PGN string format.
    [[nodiscard]] std::string toPgn(
        std::string_view whiteName,
        std::string_view blackName,
        std::string_view eventName = "RookSac Match",
        std::string_view site = "Local",
        std::string_view initialFen = "") const;
};

/// \brief Plays a match between two agents starting from the given position until termination or maxPlies.
/// Both agents are guaranteed safe execution with make/unmake and legal move constraints.
[[nodiscard]] MatchResult playMatch(
    Agent& whiteAgent,
    Agent& blackAgent,
    Position initialPosition = Position::startPosition(),
    std::uint32_t maxPlies = 200);

}  // namespace rooksac
