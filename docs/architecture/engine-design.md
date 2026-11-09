# Engine design

## Planned subsystems

| Subsystem       | Responsibility                                                 | Primary collaborators                 |
| --------------- | -------------------------------------------------------------- | ------------------------------------- |
| Position        | Own complete chess state and make/unmake state transitions.    | FEN, move generation, search, hashing |
| Move generation | Produce pseudo-legal moves and establish legal-move filtering. | Position, attack queries, perft       |
| Evaluation      | Score a position from the side-to-move perspective.            | Search, position                      |
| Search          | Explore moves, maintain bounds, choose a principal variation.  | Moves, evaluation, TT, time           |
| Hash/TT         | Identify position state and cache bounded search results.      | Position, search                      |
| Time management | Set and enforce search stopping conditions.                    | UCI, search                           |
| UCI             | Translate line-oriented protocol input/output at the boundary. | Position, search, options             |

## Interfaces and dependencies

The Position API should expose state needed to generate attacks, make and unmake a compact move, and inspect invariants without exposing gratuitous mutable internals. Search calls move generation and evaluation; neither should depend on UCI. The transposition table is owned or coordinated by search, while its key derives from position state. UCI orchestrates rather than containing chess logic.

## Data path

A position arrives as the initial position plus moves or as FEN. Parsing validates format and constructs Position. Search repeatedly requests legal candidates, orders them, makes a move, evaluates leaves or recurses, then restores state. Search returns a selected move and associated information to UCI. [Data flow](data-flow.md) expands failure boundaries.

## Performance-critical areas

Move generation, attack lookup, make/unmake, evaluation, transposition-table probes, and search-node control are likely hot. Design clarity establishes correct invariants first; profiling should identify actual bottlenecks before special treatment.

## Principles

Keep interfaces explicit, allocations rare in hot loops, lifetimes clear, and tests close to subsystem boundaries. Avoid framework-driven abstractions that obscure data layout or control flow.
