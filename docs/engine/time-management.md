# Time Management

## Concept and Purpose

Time management transforms an abstract search algorithm into a competitive, clock-aware tournament engine. It determines:
1. **Target search time (`optimalTimeMs`)**: The baseline duration to spend on the current move, allowing iterative deepening to terminate between iterations if exceeded.
2. **Hard search deadline (`maximumTimeMs`)**: The absolute ceiling beyond which the search cannot continue under any circumstances. If the deadline is reached mid-iteration, search aborts cooperatively and returns the best move from the last fully completed iteration.

In tournament play (such as 3+2 blitz: 3 minutes initial time plus 2 seconds increment per move), poor time management leads directly to forfeits on time (flagging) or severe tactical blunders due to time panic.

---

## Architecture and Data Types

Implemented across `engine/include/rooksac/time_management.hpp` and `engine/src/time_management.cpp`:

- **`TimeControl`**: Inbound limits passed by the UCI session or test harness:
  - `moveTimeMs`: Explicit fixed thinking time per move (e.g. `go movetime 1000`).
  - `remainingTimeMs`: Milliseconds left on player's clock (from UCI `wtime` or `btime`).
  - `incrementMs`: Milliseconds added to clock after move is made (from UCI `winc` or `binc`).
  - `movesToGo`: Moves remaining until next time control (from UCI `movestogo`).
  - `depth`: Fixed depth limit (plies).
  - `maxNodes`: Fixed node visit limit.

- **`TimeAllocation`**: Output bounds:
  - `optimalTimeMs`: Iterative deepening stop boundary between plies.
  - `maximumTimeMs`: Hard deadline checked inside alpha-beta and quiescence search.

- **`TimeManager::allocate(tc, emergencyThresholdMs)`**: Pure function converting `TimeControl` into `TimeAllocation`.

---

## Allocation Policy and Invariants

### 1. Invariants
- **Hard Reserve Invariant**: For any clock > 100 ms, `maximumTimeMs + kHardReserveMs <= remainingTimeMs`. At least 100 ms remains untouched on the clock to absorb operating system scheduling jitter, IPC latency, and search-stop polling intervals.
- **Clock Safety Invariant**: `maximumTimeMs <= remainingTimeMs`. The engine never allocates more time than actually remains on the physical clock.
- **Maximum vs. Optimal Invariant**: `maximumTimeMs >= optimalTimeMs` at all times.
- **20% Ceiling Invariant**: In normal play, `maximumTimeMs` never exceeds 20% of the remaining clock, ensuring that no single move exhausts a disproportionate share of the clock.
- **Increment Protection Invariant**: The increment bonus is capped at `remaining / 10`. When the clock is low, future increment is never spent before it has actually been earned.

### 2. Allocation Formulas

#### Step A: Fixed Move Time
If `tc.moveTimeMs` is provided:
$$\text{optimal} = \text{maximum} = \max(1, \text{moveTimeMs})$$

#### Step B: Hard Reserve Floor
If $\text{remaining} \le 100\text{ ms}$ (`kHardReserveMs`):
$$\text{optimal} = \text{maximum} = 1\text{ ms}$$

#### Step C: Emergency Mode
If $\text{remaining} \le 1500\text{ ms}$ (`kDefaultEmergencyThresholdMs`):
$$\text{usable} = \text{remaining} - 100\text{ ms}$$
$$\text{optimal} = \text{maximum} = \max(1, \lfloor \text{usable} \times 0.30 \rfloor)$$
*Note: No increment bonus is credited during emergency mode because increment is only awarded by the chess clock after the move is completed and sent.*

#### Step D: Standard Clock Allocation
For normal play ($\text{remaining} > 1500\text{ ms}$):
1. **Usable time**:
   $$\text{usable} = \text{remaining} - 100\text{ ms}$$
2. **Move horizon divisor**:
   $$\text{divisor} = \begin{cases} \max(1, \text{movesToGo}) & \text{if } \text{movesToGo is specified} \\ 40 & \text{default for sudden-death blitz} \end{cases}$$
3. **Base allocation**:
   $$\text{baseTime} = \lfloor \text{usable} / \text{divisor} \rfloor$$
4. **Increment bonus**:
   $$\text{rawBonus} = \lfloor (\text{increment} \times 3) / 4 \rfloor$$
   $$\text{incBonus} = \min(\text{rawBonus}, \lfloor \text{remaining} / 10 \rfloor)$$
5. **Optimal time**:
   $$\text{optimal} = \text{baseTime} + \text{incBonus}$$
6. **Maximum time calculation**:
   $$\text{capByOptimal} = \text{optimal} \times 2$$
   $$\text{capByRemain} = \lfloor \text{remaining} / 5 \rfloor \quad (20\% \text{ of clock})$$
   $$\text{maximum} = \min(\text{usable}, \text{capByOptimal}, \text{capByRemain})$$
7. **Invariant preservation**:
   If $\text{optimal} > \text{maximum}$, $\text{optimal} = \text{maximum}$.
   $$\text{optimal} = \max(1, \text{optimal})$$
   $$\text{maximum} = \max(\text{optimal}, \text{maximum})$$

---

## 3+2 Blitz Allocation Schedule

Calculated values under a 3-minute + 2-second increment clock:

| Clock State | Remaining Time | Increment | Optimal Time | Maximum Time | Clock % | Hard Reserve Kept |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|
| **Match Opening** | 180,000 ms (180.0s) | 2,000 ms | 5,997 ms | 11,994 ms | 6.7% | 168,006 ms |
| **Early Middlegame** | 120,000 ms (120.0s) | 2,000 ms | 4,497 ms | 8,994 ms | 7.5% | 111,006 ms |
| **Middlegame** | 60,000 ms (60.0s) | 2,000 ms | 2,997 ms | 5,994 ms | 10.0% | 54,006 ms |
| **Late Middlegame** | 30,000 ms (30.0s) | 2,000 ms | 2,247 ms | 4,494 ms | 15.0% | 25,506 ms |
| **Low Clock** | 10,000 ms (10.0s) | 2,000 ms | 1,247 ms | 2,000 ms | 20.0% | 8,000 ms |
| **Critical Clock** | 5,000 ms (5.0s) | 2,000 ms | 622 ms | 1,000 ms | 20.0% | 4,000 ms |
| **Scramble Clock** | 2,000 ms (2.0s) | 2,000 ms | 247 ms | 400 ms | 20.0% | 1,600 ms |
| **Emergency Mode** | 1,500 ms (1.5s) | 2,000 ms | 420 ms | 420 ms | 28.0% | 1,080 ms |
| **Emergency Mode** | 1,000 ms (1.0s) | 2,000 ms | 270 ms | 270 ms | 27.0% | 730 ms |
| **Emergency Mode** | 500 ms (0.5s) | 2,000 ms | 120 ms | 120 ms | 24.0% | 380 ms |
| **Reserve Floor** | 100 ms (0.1s) | 2,000 ms | 1 ms | 1 ms | 1.0% | 99 ms |
| **Reserve Floor** | 50 ms (0.05s) | 2,000 ms | 1 ms | 1 ms | 2.0% | 49 ms |
| **Depleted** | 1 ms | 2,000 ms | 1 ms | 1 ms | 100.0% | 0 ms |

---

## Search-Stop Verification and Cooperative Deadline Enforcement

1. **`SearchContext::checkStop()`**:
   - Called at every node in `alphaBetaInternal` and at the start of every node in `quiescenceInternal`.
   - Node counter check: checked every $1024$ nodes (`kStopCheckInterval`). At ~1.5M NPS, 1024 nodes corresponds to $\approx 0.68\text{ ms}$, ensuring near-instantaneous response to deadline expiration without excessive clock query overhead.
2. **Iterative Deepening Boundary**:
   - At the completion of each iteration in `search()`, the engine compares elapsed time against `optimalTimeMs`.
   - If `elapsedMs >= optimalTimeMs`, the search finishes cleanly and does not begin a deeper iteration that it cannot finish.
3. **Emergency Interruption Handling**:
   - If the hard deadline expires mid-iteration, `context.aborted` is set to `true`.
   - The partial iteration is discarded.
   - The engine returns `lastCompletedResult`, which contains the exact evaluation and best move from the last fully completed depth.
   - Even if interrupted at depth 1, the root search guarantees a legal fallback move.
