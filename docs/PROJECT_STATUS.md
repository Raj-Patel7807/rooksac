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
- Iterative deepening, quiescence, and time management: progressive deepening from depth 1 upward with PV-first move ordering, tactical quiescence search with check evasions and stand-pat rules, dedicated `TimeManager` with safety reserve and emergency clock handling, cooperative `SearchStopToken`, and guaranteed legal fallback moves.
- UCI protocol and standalone executable: Universal Chess Interface protocol implementation (`rooksac::uci::EngineSession`) supporting `uci`, `isready`, `ucinewgame`, `position`, `go` (depth, movetime, wtime/btime/winc/binc, nodes, infinite), `stop`, and `quit`; strict stdout stream discipline, stderr diagnostic routing, and standalone `rooksac` executable.
- Zobrist hashing and transposition table: deterministic 64-bit Zobrist keys updated incrementally on make/unmake and verified with full recalculation; memory-bounded `TranspositionTable` with 16-byte packed entries, depth-preferred replacement with generational aging, mate score ply normalization, and configurable UCI `Hash` option.
- Classical tapered evaluation: tapered `(mg, eg)` score evaluation blended by non-pawn material game phase (24 total), colour-mirrored Piece-Square Tables (PSTs), pawn structure terms (isolated, doubled, passed), king safety penalties (missing shield pawns, semi-open files), piece activity (bishop pair, open rooks), safe mobility, and centralized `EvalParameters`.
- Advanced classical search and benchmark suite: aspiration windows with fail-soft widening, Principal Variation Search (PVS), Late Move Reductions (LMR) with full-depth re-search, null-move pruning (NMP) with zugzwang safeguards, killer move heuristic (2 slots/ply), `SearchOptions` feature isolation, canonical 8-position fixed FEN benchmark suite, standalone `rooksac_bench` executable, and protocol-safe UCI `bench` command.
- Texel evaluation tuning, self-play matches, and SPRT: versioned evaluation parameter JSON serialization/deserialization schema, Texel sigmoid probability mapping and Mean Squared Error objective calculation, whole-game position dataset extraction and splitting (`tools/pgn_extract.py`), and Sequential Probability Ratio Test (SPRT) statistical decision engine in C++ and Python (`tools/sprt_match.py`).
- Supervised policy-value neural network pipeline: offline PyTorch residual CNN architecture (`PolicyValueNet`) with 18-plane feature encoding, 4096-action move vocabulary, joint policy-value loss training pipeline, whole-game zero-leakage dataset validation, and model checkpointing (`research/nn/`).
- Native neural evaluation and policy move ranking: self-contained native C++ neural inference module (`NeuralEvaluator`), 18-plane feature tensor extraction (`encodePosition`), value calibration mapping $v \in [-1, 1]$ to engine centipawns with side-to-move perspective, legal-only policy move ordering (`orderMovesByPolicy`), and selectable evaluation modes (`rooksac::neural::EvalMode`).

## In progress

- Legal and provenance baseline: the repository inventory and future review process are documented, but licence selection and copyright ownership require owner confirmation.

## Planned

- Extended neural network architectures and hybrid classical-neural blend experiments.
- Perft and benchmark tools, offline research tooling, and repository tools.
- A separate application layer under `app/` that communicates with the engine through UCI/process boundaries.
- CI, packaging, deployment, and release processes.

## Deferred

- Publishing a license, copyright notices, SPDX source headers, and any `NOTICE` file are deferred until the owner confirms the copyright holder, year policy, and license choice.
- Any networked application, database, queue, frontend, deployment, neural evaluation, or Python runtime integration is deferred to later roadmap work.

## Current limitations

The engine uses classical tapered evaluation; neural network evaluation (NNUE) is not yet implemented. `rooksac_core` provides core types, bitboard operations, position state, FEN parsing, make/unmake move transitions, piece attacks, pseudo-legal and legal move generation, game evaluation, perft testing, material evaluation, classical tapered evaluation, baseline agents (random and greedy), fail-soft Alpha-Beta search with staged move ordering, iterative deepening, quiescence search, time management, deterministic Zobrist hashing, memory-bounded transposition tables, UCI protocol handling, advanced search heuristics (aspiration windows, PVS, LMR, NMP, killer moves), benchmark suite, standalone executable `rooksac`, benchmark binary `rooksac_bench`, and match runner `rooksac_match_harness`.
