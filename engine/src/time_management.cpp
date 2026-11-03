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

    const std::uint64_t remaining64 = *tc.remainingTimeMs;
    const std::uint64_t increment64 = tc.incrementMs.value_or(0U);

    // 3. Hard floor: if remaining clock is at or below the hard reserve floor,
    // return minimum move time immediately.
    // This strictly preserves kHardReserveMs against OS scheduler jitter,
    // IPC/UCI latency, and search-stop polling intervals.
    if (remaining64 <= kHardReserveMs) {
        allocation.optimalTimeMs = kMinMoveTimeMs;
        allocation.maximumTimeMs = kMinMoveTimeMs;
        return allocation;
    }

    const std::uint64_t usable64 = remaining64 - kHardReserveMs;

    // 4. Emergency low-time handling: triggers when remaining clock <= emergencyThresholdMs
    // (e.g. <= 1500 ms). In this regime:
    // - We spend at most 30% of the usable clock (leaving 70% of usable + 100ms reserve).
    // - NO increment bonus is credited, because the increment is only awarded AFTER
    //   the move is legally transmitted and accepted by the arbiter.
    // - Checked 64-bit arithmetic prevents uint32_t overflow on extreme values.
    if (remaining64 <= emergencyThresholdMs) {
        const std::uint64_t emergencyTime64 = std::max<std::uint64_t>(
            kMinMoveTimeMs, (usable64 * 3ULL) / 10ULL);
        const auto emergencyTime = static_cast<std::uint32_t>(
            std::min<std::uint64_t>(emergencyTime64, usable64));
        allocation.optimalTimeMs = emergencyTime;
        allocation.maximumTimeMs = emergencyTime;
        return allocation;
    }

    // 5. Standard clock allocation
    const std::uint64_t divisor64 = tc.movesToGo.has_value()
        ? std::max<std::uint64_t>(1ULL, static_cast<std::uint64_t>(*tc.movesToGo))
        : static_cast<std::uint64_t>(kDefaultExpectedMoves);

    const std::uint64_t baseTime64 = usable64 / divisor64;

    // Increment bonus: 75% of increment, capped at 10% of remaining clock.
    // Capping at remaining / 10 ensures that on low-ish clocks (e.g. 2s - 5s),
    // a large increment (e.g. 2000 ms) cannot single-handedly cause an overspend
    // before that increment is actually in hand.
    // Use 64-bit intermediate arithmetic throughout to prevent overflow.
    const std::uint64_t rawIncBonus64 = (increment64 * 3ULL) / 4ULL;
    const std::uint64_t incCap64 = remaining64 / 10ULL;
    const std::uint64_t incBonus64 = std::min<std::uint64_t>(rawIncBonus64, incCap64);

    // In 32-bit math, baseTime + incBonus can overflow UINT32_MAX when remaining time
    // is large (e.g. 4 billion ms with movesToGo = 1).
    // Using 64-bit addition guarantees exact arithmetic without wraparound.
    const std::uint64_t optimal64 = baseTime64 + incBonus64;

    // Maximum thinking time caps in 64-bit space:
    //   Cap A: 2x optimal (allows reasonable overtime on complex positions)
    //   Cap B: 20% of remaining time (strict ceiling to prevent spending too much on one move)
    //   Cap C: usable clock (strictly remaining - kHardReserveMs)
    const std::uint64_t capByOptimal64 = optimal64 * 2ULL;
    const std::uint64_t capByRemain64  = remaining64 / 5ULL;
    std::uint64_t maximum64 = std::min({usable64, capByOptimal64, capByRemain64});

    // Invariant resolution:
    // If optimal exceeds maximum due to caps or integer arithmetic, clamp optimal down to maximum.
    // Never allow maximum = std::max(maximum, optimal) to override the 20% safety ceiling!
    std::uint64_t boundedOptimal64 = optimal64;
    if (boundedOptimal64 > maximum64) {
        boundedOptimal64 = maximum64;
    }
    boundedOptimal64 = std::max<std::uint64_t>(kMinMoveTimeMs, boundedOptimal64);
    maximum64 = std::max<std::uint64_t>(boundedOptimal64, maximum64);

    // Final bounded conversion to uint32_t (clamping to UINT32_MAX though maximum64 <= remaining64 <= UINT32_MAX)
    const auto optimal = static_cast<std::uint32_t>(std::min<std::uint64_t>(boundedOptimal64, UINT32_MAX));
    const auto maximum = static_cast<std::uint32_t>(std::min<std::uint64_t>(maximum64, UINT32_MAX));

    allocation.optimalTimeMs = optimal;
    allocation.maximumTimeMs = maximum;
    return allocation;
}

}  // namespace rooksac
