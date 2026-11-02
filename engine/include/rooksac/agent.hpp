#pragma once

#include <optional>
#include <string_view>

#include "rooksac/move.hpp"
#include "rooksac/move_list.hpp"
#include "rooksac/position.hpp"

namespace rooksac {

/// \brief Pure virtual interface for move selection policies / agents.
class Agent {
public:
    virtual ~Agent() = default;

    /// \brief Returns the human-readable name of the agent policy.
    [[nodiscard]] virtual std::string_view name() const noexcept = 0;

    /// \brief Selects one legal move from the given position and legal move list.
    /// \param position The position state (guaranteed to be preserved after selection).
    /// \param legalMoves The pre-generated legal moves.
    /// \return The selected legal move, or std::nullopt if legalMoves is empty (terminal state).
    [[nodiscard]] virtual std::optional<Move> selectMove(Position& position, const MoveList& legalMoves) = 0;
};

}  // namespace rooksac
