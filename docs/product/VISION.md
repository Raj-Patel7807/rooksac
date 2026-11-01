# Vision

## Motivation

RookSac is a vehicle for rigorous computer-chess engineering. A chess engine concentrates systems concerns—compact state, correctness-sensitive transformations, exponential search, timing, and reproducible measurement—in one coherent project.

## Long-term vision

Develop a capable, understandable C++ engine from first principles, with each subsystem independently testable and each meaningful optimization justified by evidence. Python supports analysis and research without becoming a runtime dependency for choosing moves.

## Philosophy

1. Correctness before optimization.
2. Understandable engineering before unnecessary complexity.
3. Measurement over assumptions.
4. Explicit reasons for architectural decisions.
5. Tests alongside implemented behavior.
6. Clear separation of present implementation and future intent.

## Goals

- Build a legal, UCI-compatible engine incrementally.
- Preserve modular boundaries inside a practical monolith.
- Make regressions discoverable through automated checks and reference positions.
- Enable reproducible performance and evaluation experiments.

## Non-goals

- A graphical chess client.
- Claims of playing strength without a reproducible method.
- Python in the real-time search loop.
- Adding complexity solely to resemble other engines.

## What success looks like

Success is a verified, maintainable engine with documented decisions, repeatable tests and benchmarks, and a clear path for further research—not merely a program that returns moves.
