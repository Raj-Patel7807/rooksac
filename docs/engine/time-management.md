# Time management

## Concept and value

Time management turns a searcher into a practical engine: it allocates available clock time, checks stop conditions, and returns a legal move before a deadline.

## Planned RookSac design

After basic search and iterative deepening exist, the search controller will receive limits from the protocol boundary, derive a conservative target and hard stop, and check cancellation at controlled intervals. Exact policies for increment, moves-to-go, infinite analysis, and overhead are open until UCI integration and measurement.

## Alternatives and trade-offs

Fixed-depth or fixed-node limits are deterministic and valuable for tests but do not play timed games well. Aggressive clock use may find stronger moves but risks flagging; conservative margins reduce risk but may underuse time.

## Edge cases and testing

Immediate stop, tiny time budgets, no legal moves, interrupted iterative deepening, and clock unit conversion need explicit tests. Test with deterministic fake clocks where possible and protocol transcripts later.

## Future improvements and references

Allocation may incorporate position volatility only after evidence. See [UCI protocol](uci-protocol.md) and [Time Management](https://www.chessprogramming.org/Time_Management).
