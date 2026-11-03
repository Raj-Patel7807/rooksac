#pragma once

#include <cstdint>
#include <optional>

namespace rooksac {

/// \brief Time and resource constraints passed to the search controller.
struct TimeControl {
    std::optional<std::uint32_t> depth{};           ///< Search depth in plies.
    std::optional<std::uint64_t> maxNodes{};        ///< Maximum node visits.
    std::optional<std::uint32_t> moveTimeMs{};      ///< Fixed search time per move in ms.
    std::optional<std::uint32_t> remainingTimeMs{}; ///< Clock time remaining in ms.
    std::optional<std::uint32_t> incrementMs{};     ///< Clock increment per move in ms.
    std::optional<std::uint32_t> movesToGo{};       ///< Moves until next time control.

    constexpr TimeControl() noexcept = default;
};

/// \brief Target and deadline allocation calculated from time controls.
struct TimeAllocation {
    std::uint32_t optimalTimeMs = 0; ///< Target time to spend on current move.
    std::uint32_t maximumTimeMs = 0; ///< Hard upper bound; search must stop before this.
};

/// \brief Calculates time allocation for a single move with safety reserves.
class TimeManager {
public:
    static constexpr std::uint32_t kDefaultSafetyReserveMs = 50;  ///< Soft reserve subtracted before allocation.
    static constexpr std::uint32_t kHardReserveMs = 100;           ///< Hard floor kept on the clock at all times to prevent flagging.
    static constexpr std::uint32_t kDefaultExpectedMoves = 40;     ///< Expected remaining moves in sudden-death; 40 is more realistic than 50 for blitz.
    static constexpr std::uint32_t kMinMoveTimeMs = 1;             ///< Absolute floor for move allocation.

    /// \brief Allocates optimal and maximum time for a move based on clock state.
    /// \param tc Time control specifications.
    /// \param safetyReserveMs Safety reserve in ms to prevent flagging.
    /// \return Calculated TimeAllocation.
    [[nodiscard]] static TimeAllocation allocate(
        const TimeControl& tc,
        std::uint32_t safetyReserveMs = kDefaultSafetyReserveMs) noexcept;
};

}  // namespace rooksac
