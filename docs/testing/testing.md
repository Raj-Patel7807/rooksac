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

The current CTest suite contains one build-system smoke test. It verifies C++20 compilation and CTest integration only; it does not validate chess behavior. No unit-test framework, position test, perft test, or search test exists yet. See [Perft](perft.md) and [build guidance](../engineering/build.md).
