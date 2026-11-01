# Product requirements document

## Product overview

RookSac is a planned cross-platform chess engine written in C++20. It will communicate with compatible GUIs through UCI and use Python 3 for offline experimentation.

## Motivation and users

The project serves contributors learning or practicing serious engine engineering, developers integrating an open engine with a GUI, and researchers running controlled evaluation experiments.

## Goals

- Represent all standard chess position state accurately.
- Generate and validate legal moves.
- Select moves through progressively stronger search and evaluation.
- Interoperate through UCI after the protocol milestone.
- Make correctness and performance evidence reproducible.

## Non-goals and boundaries

No GUI, online service, opening-book promise, rating target, or neural dependency is in the initial scope. Each roadmap phase limits what may be claimed as delivered.

## Functional requirements

The engine is intended to parse positions, generate legal moves, search with time awareness, evaluate positions, and eventually accept UCI commands. The uniquely identified source of requirement status is [REQUIREMENTS.md](REQUIREMENTS.md).

## Non-functional requirements

Correctness, deterministic testability where practical, C++20 portability across Windows and Linux, clear ownership, bounded memory use, and evidence-led performance work are required qualities.

## Technical constraints

The runtime engine remains native C++ and should avoid unnecessary allocation on hot paths. CMake is the planned build system. Python tooling must not be required to search or select a move.

## Success criteria

Milestone acceptance criteria in the [roadmap](ROADMAP.md), passing relevant tests, and documented benchmark methodology determine progress. Documentation alone never proves functionality.
