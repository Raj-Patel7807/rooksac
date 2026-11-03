#include "rooksac/time_management.hpp"

#include <algorithm>
#include <cstdint>

namespace rooksac {

TimeAllocation TimeManager::allocate(
    const TimeControl& tc,
    std::uint32_t safetyReserveMs) noexcept {
    TimeAllocation allocation{};

    // 1. Explicit fixed move time
    if (tc.moveTimeMs.has_value()) {
        const std::uint32_t moveTime = std::max(kMinMoveTimeMs, *tc.moveTimeMs);
        allocation.optimalTimeMs = moveTime;
        allocation.maximumTimeMs = moveTime;
        return allocation;
    }

    // 2. If no clock time is specified, return zero (unbounded by clock)
    if (!tc.remainingTimeMs.has_value()) {
        return allocation;
    }

    const std::uint32_t remaining  = *tc.remainingTimeMs;
    const std::uint32_t increment  = tc.incrementMs.value_or(0U);

    // 3. Hard floor: always leave at least kHardReserveMs on the clock.
    //    This guards against OS jitter, network latency, and the worst-case
    //    node-interval check delay.
    if (remaining <= kHardReserveMs) {
        allocation.optimalTimeMs = kMinMoveTimeMs;
        allocation.maximumTimeMs = kMinMoveTimeMs;
        return allocation;
    }

    // 4. Emergency low-time handling (remaining <= safetyReserveMs)
    //    Spend only a small fraction of remaining. Never add increment here
    //    because the increment is only earned after the move is safely delivered.
    if (remaining <= safetyReserveMs) {
        // Use 40% of what remains after hard reserve floor
        const std::uint32_t usable = remaining - kHardReserveMs;
        const std::uint32_t emergencyTime = std::max(kMinMoveTimeMs, usable * 2U / 5U);
        allocation.optimalTimeMs = emergencyTime;
        allocation.maximumTimeMs = emergencyTime;
        return allocation;
    }

    // 5. Standard clock allocation
    // usable: available time after deducting safety reserve and hard reserve
    const std::uint32_t usable = remaining - safetyReserveMs;

    const std::uint32_t divisor = tc.movesToGo.has_value()
        ? std::max(1U, *tc.movesToGo)
        : kDefaultExpectedMoves;

    // Base time = usable / expected_moves, plus 75% of increment as bonus.
    // 75% (not 100%) because the increment arrives only after the move completes;
    // using it fully would leave no margin if the search runs slightly long.
    const std::uint32_t baseTime = usable / divisor;
    const std::uint32_t incBonus = (increment * 3U) / 4U;
    std::uint32_t optimal = baseTime + incBonus;
    optimal = std::clamp(optimal, kMinMoveTimeMs, usable);

    // Maximum thinking time: apply two caps, take the stricter of the two.
    //   Cap A: 2× optimal (allows overtime on critical positions)
    //   Cap B: 20% of total remaining clock (prevents spending too much on one move)
    // Then apply the hard reserve: maximum must leave kHardReserveMs on clock.
    const std::uint32_t capByOptimal = optimal * 2U;
    const std::uint32_t capByRemain  = remaining / 5U;       // 20% of clock
    std::uint32_t maximum = std::min({usable, capByOptimal, capByRemain});
    maximum = std::max(maximum, optimal);                     // max must be >= optimal

    // Final hard-reserve clamp: maximum must leave at least kHardReserveMs remaining
    if (maximum + kHardReserveMs > remaining) {
        maximum = (remaining > kHardReserveMs) ? (remaining - kHardReserveMs) : kMinMoveTimeMs;
    }
    // If the reserve clamp reduced maximum below optimal, reduce optimal too
    if (maximum < optimal) {
        optimal = maximum;
    }

    allocation.optimalTimeMs = optimal;
    allocation.maximumTimeMs = maximum;
    return allocation;
}

}  // namespace rooksac
