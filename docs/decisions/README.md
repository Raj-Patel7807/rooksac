# Architecture decisions

This file is the initial ADR register. Status distinguishes an adopted working direction from a choice still subject to evidence. New material changes should add an entry rather than silently revising history.

## Decision index

- [ADR-007 — Modular monorepo repository layout](adr-007-repository-layout.md)
- [ADR-008 — Legal provenance and licence decision gate](adr-008-legal-provenance-and-license-gate.md)
- [ADR-009 — Core primitives and square mapping](adr-009-core-primitives-and-square-mapping.md)
- [ADR-010 — Position state representation, FEN parser, and reversible make/unmake](adr-010-position-representation-and-reversible-state.md)
- [ADR-011 — Piece attack generation, MoveList container, and pseudo-legal move generation](adr-011-attack-generation-and-pseudo-legal-moves.md)
- [ADR-012 — Legal move filtering, game evaluation, and perft certification](adr-012-legal-movegen-and-perft-certification.md)
- [ADR-013 — Baseline agents, material evaluation, and self-play match harness](adr-013-baseline-agents-and-material-evaluation.md)
- [ADR-014 — Minimax and Negamax game-tree search](adr-014-minimax-and-negamax-search.md)
- [ADR-015 — Alpha-Beta pruning, fail-soft bounds, and staged move ordering](adr-015-alpha-beta-pruning-and-move-ordering.md)

## ADR-001 — C++20 for the engine

- **Status:** Accepted
- **Context / problem:** The engine needs efficient native code with modern language facilities and broad compiler support.
- **Options considered:** Earlier C++, C++20, another primary runtime language.
- **Decision:** Use C++20 for the engine.
- **Reasoning:** It balances modern expressive facilities with established Windows and Linux toolchains.
- **Trade-offs / consequences:** Contributors need a C++20-capable compiler; language features still require clear ownership and performance discipline.

## ADR-002 — CMake for builds

- **Status:** Accepted
- **Context / problem:** A cross-platform project needs a build description independent of one IDE.
- **Options considered:** CMake, IDE-specific projects, alternative build systems.
- **Decision:** Use CMake when a build system is introduced.
- **Reasoning:** It is widely supported by target compilers and testing tools.
- **Trade-offs / consequences:** CMake configuration must be maintained and documented with real targets.

## ADR-003 — Bitboards as the preferred initial representation

- **Status:** Proposed
- **Context / problem:** Board operations need compact, fast attack-oriented state.
- **Options considered:** Bitboards, mailbox array, 2D array, hybrid designs.
- **Decision:** Begin design around bitboards, subject to implementation validation and benchmarks.
- **Reasoning:** A 64-square board maps naturally to a 64-bit word and supports parallel set operations.
- **Trade-offs / consequences:** Bitwise code can be less immediately readable; square conventions and tests must be explicit.

## ADR-004 — UCI as target protocol

- **Status:** Accepted
- **Context / problem:** The engine needs interoperable GUI communication.
- **Options considered:** UCI, a custom protocol, direct GUI integration.
- **Decision:** Target UCI in a later milestone.
- **Reasoning:** UCI is widely supported and keeps the engine independent of a GUI.
- **Trade-offs / consequences:** Protocol parsing and timing semantics need dedicated tests.

## ADR-005 — Python for offline experimentation

- **Status:** Accepted
- **Context / problem:** Analysis and research benefit from a productive scripting environment without harming runtime performance.
- **Options considered:** C++ only, Python runtime integration, Python offline tooling.
- **Decision:** Use Python 3 for offline experiments, not the main search loop.
- **Reasoning:** It enables data analysis and tuning while retaining a self-contained C++ engine.
- **Trade-offs / consequences:** Tool results need reproducibility metadata and stable data boundaries.

## ADR-006 — Modular monolith

- **Status:** Accepted
- **Context / problem:** The project needs separable responsibilities without distributed-system overhead.
- **Options considered:** One undifferentiated codebase, modular monolith, separate services/libraries.
- **Decision:** Use explicit internal modules in one native engine application.
- **Reasoning:** It preserves direct data flow and performance while making components testable.
- **Trade-offs / consequences:** Module boundaries require ongoing discipline; premature library splitting is avoided.
