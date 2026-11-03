#include "rooksac/time_management.hpp"

#include <algorithm>
#include <cstdint>

namespace rooksac {

TimeAllocation TimeManager::allocate(
    const TimeControl& tc,
    std::uint32_t emergencyThresholdMs) noexcept {
    TimeAllocation allocation{};

    // 1. Explicit fixed move time takes precedence
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

    // 3. Hard floor: if remaining clock is at or below the hard reserve floor,
    // return minimum move time immediately.
    // This strictly preserves kHardReserveMs against OS scheduler jitter,
    // IPC/UCI latency, and search-stop polling intervals.
    if (remaining <= kHardReserveMs) {
        allocation.optimalTimeMs = kMinMoveTimeMs;
        allocation.maximumTimeMs = kMinMoveTimeMs;
        return allocation;
    }

    const std::uint32_t usable = remaining - kHardReserveMs;

    // 4. Emergency low-time handling: triggers when remaining clock <= emergencyThresholdMs
    // (e.g. <= 1500 ms). In this regime:
    // - We spend at most 30% of the usable clock (leaving 70% of usable + 100ms reserve).
    // - NO increment bonus is credited, because the increment is only awarded AFTER
    //   the move is legally transmitted and accepted by the arbiter.
    if (remaining <= emergencyThresholdMs) {
        const std::uint32_t emergencyTime = std::max(kMinMoveTimeMs, (usable * 3U) / 10U);
        allocation.optimalTimeMs = emergencyTime;
        allocation.maximumTimeMs = emergencyTime;
        return allocation;
    }

    // 5. Standard clock allocation
    const std::uint32_t divisor = tc.movesToGo.has_value()
        ? std::max(1U, *tc.movesToGo)
        : kDefaultExpectedMoves;

    const std::uint32_t baseTime = usable / divisor;

    // Increment bonus: 75% of increment, capped at 10% of remaining clock.
    // Capping at remaining / 10 ensures that on low-ish clocks (e.g. 2s - 5s),
    // a large increment (e.g. 2000 ms) cannot single-handedly cause an overspend
    // before that increment is actually in hand.
    const std::uint32_t rawIncBonus = (increment * 3U) / 4U;
    const std::uint32_t incCap = remaining / 10U;
    const std::uint32_t incBonus = std::min(rawIncBonus, incCap);

    std::uint32_t optimal = baseTime + incBonus;

    // Maximum thinking time caps:
    //   Cap A: 2x optimal (allows reasonable overtime on complex positions)
    //   Cap B: 20% of remaining time (strict ceiling to prevent spending too much on one move)
    //   Cap C: usable clock (strictly remaining - kHardReserveMs)
    const std::uint32_t capByOptimal = optimal * 2U;
    const std::uint32_t capByRemain  = remaining / 5U;
    std::uint32_t maximum = std::min({usable, capByOptimal, capByRemain});

    // Invariant resolution:
    // If optimal exceeds maximum due to caps or integer arithmetic, clamp optimal down to maximum.
    // Never allow maximum = std::max(maximum, optimal) to override the 20% safety ceiling!
    if (optimal > maximum) {
        optimal = maximum;
    }
    optimal = std::max(kMinMoveTimeMs, optimal);
    maximum = std::max(optimal, maximum);

    allocation.optimalTimeMs = optimal;
    allocation.maximumTimeMs = maximum;
    return allocation;
}

}  // namespace rooksac
