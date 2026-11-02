# Time management

## Concept and value

Time management turns a searcher into a practical engine: it allocates available clock time, checks stop conditions, and returns a legal move before a deadline.

## Implemented design

Time management is implemented in `rooksac/time_management.hpp` and `engine/src/time_management.cpp`. Three types are provided:

- **`TimeControl`** — carries the raw limit parameters from the caller: `moveTimeMs`, `remainingTimeMs`, `incrementMs`, `movesToGo`.
- **`TimeAllocation`** — carries the derived result: `optimalMs` (target) and `maximumMs` (hard stop).
- **`TimeManager`** — converts `TimeControl` into `TimeAllocation` via `allocate()`.

### Allocation formula

`TimeManager::allocate()` handles three cases:

| Case | Condition | Logic |
| --- | --- | --- |
| Fixed move time | `moveTimeMs > 0` | `optimal = maximum = moveTimeMs` |
| Normal clock | `remainingTimeMs > safetyReserveMs (50 ms)` | `available = remaining − 50`; `divisor = movesToGo ∥ 40`; `optimal = available/divisor + increment×0.75`; `maximum = min(available, max(optimal×3, optimal+increment))` |
| Emergency clock | `remainingTimeMs ≤ 50 ms` | `optimal = max(1 ms, remaining/2)`; `maximum = remaining − 2 ms` |

The safety reserve of 50 ms guards against network and process latency. The default divisor of 40 assumes 40 moves remain in a sudden-death game, which is a common practical heuristic.

### Cooperative stop token

`SearchStopToken` (a thin wrapper over `std::atomic<bool>`) is signaled from the outside and checked by the search:
- At the **start of each iterative deepening iteration**, before beginning any search work.
- Every **`kStopCheckInterval` (1024) nodes** inside `alphaBetaInternal`.

This guarantees that the search stops promptly without requiring shared state beyond the atomic flag. The search always returns at least the result from the last completed iteration, and always guarantees a legal fallback move.

## Alternatives and trade-offs

Fixed-depth or fixed-node limits are deterministic and valuable for tests but do not play timed games well. Aggressive clock use may find stronger moves but risks flagging; conservative margins reduce risk but may underuse time.

## Edge cases and testing

`rooksac_search_test` covers: fixed move-time allocation, normal clock allocation with increment and movesToGo, emergency clock (≤50 ms), cooperative stop via pre-signaled token, and search interrupted mid-iteration returning last-completed result with a legal fallback.

## Future improvements and references

Allocation may incorporate position volatility only after evidence. See [ADR 002](../decisions/ADR_002_SEARCH_EVALUATION_AND_LEARNING.md), [UCI protocol](uci-protocol.md), and [Time Management](https://www.chessprogramming.org/Time_Management).
