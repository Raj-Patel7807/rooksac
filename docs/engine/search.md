# Search

## Concept and progression

Search explores legal move trees to select a move. RookSac plans this progression: Negamax; alpha-beta pruning; move ordering; iterative deepening; quiescence search; transposition tables and Zobrist hashing; principal variation search; aspiration windows; killer moves and history heuristics; then only justified advanced work.

| Technique | Problem solved / expected benefit | Trade-off |
| --- | --- | --- |
| Negamax | Uniform two-player score recursion | Explores too many nodes alone. |
| Alpha-beta | Cuts branches that cannot affect the result | Depends heavily on ordering. |
| Ordering | Finds useful cutoffs earlier | Heuristics need validation. |
| Iterative deepening | Produces usable partial results and ordering | Repeats shallow work. |
| Quiescence | Reduces unstable capture-horizon errors | Can grow excessively without limits. |
| TT / hashing | Reuses repeated-position search | Needs correct keys and bounds. |
| PVS / aspiration | Narrows common-case search windows | Requires robust re-search behavior. |
| Killer / history | Learns ordering from prior cutoffs | Can bias ordering and needs aging policy. |

## Planned design and flow

Search will receive a position, generate legal moves, make/unmake them, evaluate leaves, honor stop conditions, and return a best move with score and principal variation where supported. No technique in this document is implemented today.

## Edge cases, testing, and future work

Terminal positions, repetition and draw policies, mate-score normalization, interrupted search, bound correctness, and state restoration require tests. Compare against known positions and use deterministic node-limited tests where possible. See [TT](transposition-table.md), [time management](time-management.md), and [Alpha-Beta](https://www.chessprogramming.org/Alpha-Beta).
