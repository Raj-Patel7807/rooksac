# Testing

## Philosophy

Tests establish confidence in actual behavior; documentation and a passing compile do not. Add the smallest useful test at the layer where a regression can be isolated, retain discovered regressions, and record what was run.

## Planned test layers

- **Unit tests:** Position state, parsing, attack logic, evaluation terms, hash updates, and isolated utilities.
- **Integration tests:** Position-to-search behavior, make/unmake paths, and later UCI transcripts.
- **Regression tests:** A permanent reproduction for every fixed defect where practical.
- **Position validation:** FEN parsing, state invariants, and known legal edge cases.
- **Move generation:** Perft and divide output against published reference positions.
- **Search:** Deterministic node/depth-limited positions, mate and draw semantics, and stop behavior.
- **Protocol:** Commands, option handling, output ordering, and cancellation once UCI exists.

## Tooling and edge cases

Future C++ builds should support warnings and sanitizers where platform/toolchain availability permits. Prioritize memory safety, undefined behavior, state restoration, pinned pieces, checks, castling, en passant, promotions, terminal positions, malformed input, and time interruption.

The current CTest suite contains a build-system smoke test, exhaustive core-primitive coverage for square mapping, algebraic coordinates, masks, bit operations, piece encoding, and move encoding, position state testing covering FEN round trips, malformed FEN error handling, and exact make/unmake reversibility, move generation tests covering piece attacks, ray-stepping sliders with blockers, attack detection, and pseudo-legal move generation across pawn pushes, captures, promotions, en-passant, and castling, an authoritative perft suite certifying legal move generation against canonical Chess Programming Wiki suites across depths 1 through 4, and baseline agent tests verifying deterministic seeding, legality of selected moves, material balance scoring, greedy free captures, documented greedy capture weaknesses, and automated match play. No search test exists yet. See [Perft](perft.md) and [build guidance](../engineering/build.md).
