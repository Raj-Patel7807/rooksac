# Project status

This page is the authoritative status summary. It describes repository facts, not planned capabilities.

## Implemented

- Repository governance: agent guidance, contribution guidance, security reporting policy, support guidance, formatting/editor settings, ignore rules, and text attributes.
- Documentation structure: indexed product, architecture, engine, testing, engineering, reference, decision, ML, application, operations, and legal sections.
- Repository-layout decision record and lowercase kebab-case documentation naming, except conventional `README.md` indexes.
- Native build foundation: CMake 3.23+, C++20 smoke-test target, CTest registration, Debug/Release/RelWithDebInfo presets, compiler warnings, clang-format and optional clang-tidy/sanitizer targets.
- CI quality gates: Linux GCC 12 and Clang 15 builds, Windows MSVC build, CTest, formatting, clang-tidy, and Clang ASan/UBSan jobs.
- Core chess primitives: strongly typed colors, pieces, files, ranks, squares, bitboards, masks, and compact move encoding with exhaustive primitive tests.
- Position state and FEN parser: transactional FEN serialization and deserialization with structured errors, piece and occupancy bitboards with mailbox consistency, and reversible make/unmake move execution with `UndoState`.
- Attack and pseudo-legal move generation: compile-time leaper attack lookup tables, ray-stepping slider attacks, square attack predicates, bounded `MoveList`, and pseudo-legal move generation covering all pieces, captures, promotions, en-passant, and castling paths.
- Legal move generation and perft certification: king-safety filtering, castling path and check legality, en-passant discovered check prevention, check/checkmate/stalemate/fifty-move draw evaluation, and full perft and divide certification against canonical CPW benchmark suites.
- Baseline agents and match harness: `Agent` interface, reproducible `RandomAgent` with injectable seed, 1-ply `GreedyAgent` using material balance, configurable `MaterialWeights`, and standalone match runner `rooksac_match_harness`.
- Minimax and Negamax game-tree search: side-to-move-relative depth-limited negamax search, terminal state scoring (checkmate, stalemate, 50-move rule), mate-distance adjustments (`kMateScore - ply`), configurable leaf evaluators, `NegamaxAgent`, and search statistics reporting.
- Alpha-Beta pruning and move ordering: fail-soft Alpha-Beta search with cutoffs, staged move ordering (capture-promotions, promotions, MVV-LVA captures, quiet moves), unpruned differential testing path (`searchMinimax`), and substantial node reduction verified.

## In progress

- Legal and provenance baseline: the repository inventory and future review process are documented, but licence selection and copyright ownership require owner confirmation.

## Planned

- Quiescence search, iterative deepening, and aspiration windows (Sprint 11).
- Transposition table and Zobrist hashing (Sprint 12).
- Position evaluation, piece-square tables, and mobility terms.
- Time management and UCI protocol support.
- A standalone C++20 chess engine executable and UCI protocol support under `engine/`.
- Perft and benchmark tools, offline research tooling, and repository tools.
- A separate application layer under `app/` that communicates with the engine through UCI/process boundaries.
- CI, packaging, deployment, and release processes.

## Deferred

- Publishing a license, copyright notices, SPDX source headers, and any `NOTICE` file are deferred until the owner confirms the copyright holder, year policy, and license choice.
- Any networked application, database, queue, frontend, deployment, neural evaluation, or Python runtime integration is deferred to later roadmap work.

## Current limitations

There is no quiescence search, iterative deepening, transposition table, positional evaluation (PSTs/mobility), time management, UCI protocol implementation, or production engine executable. `rooksac_core` provides core types, bitboard operations, position state, FEN parsing, make/unmake move transitions, piece attacks, pseudo-legal and legal move generation, game evaluation, perft testing, material evaluation, baseline agents (random and greedy), fail-soft Alpha-Beta search with staged move ordering, and a match runner.
