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

    const std::uint32_t remaining = *tc.remainingTimeMs;
    const std::uint32_t increment = tc.incrementMs.value_or(0U);

    // 3. Emergency low-time handling (clock below safety reserve)
    if (remaining <= safetyReserveMs) {
        // Under extreme time pressure, spend at most half the remaining time down to floor
        const std::uint32_t emergencyTime = std::max(kMinMoveTimeMs, remaining / 2U);
        allocation.optimalTimeMs = emergencyTime;
        allocation.maximumTimeMs = std::max(emergencyTime, remaining > 2U ? remaining - 2U : remaining);
        return allocation;
    }

    // 4. Standard clock allocation
    const std::uint32_t available = remaining - safetyReserveMs;
    const std::uint32_t divisor = tc.movesToGo.has_value()
        ? std::max(1U, *tc.movesToGo)
        : kDefaultExpectedMoves;

    // Allocate base fraction of available time plus 75% of increment
    const std::uint32_t baseTime = available / divisor;
    const std::uint32_t incBonus = (increment * 3U) / 4U;
    std::uint32_t optimal = baseTime + incBonus;

    optimal = std::clamp(optimal, kMinMoveTimeMs, available);

    // Hard ceiling allows extra thinking on critical turns up to 3x optimal or available time
    std::uint32_t maximum = std::min(available, std::max(optimal * 3U, optimal + increment));
    maximum = std::max(maximum, optimal);

    allocation.optimalTimeMs = optimal;
    allocation.maximumTimeMs = maximum;
    return allocation;
}

}  // namespace rooksac

