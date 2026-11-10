# Project status

This page is the authoritative status summary. It describes repository facts, not planned capabilities.

## Implemented

- Repository governance: agent guidance, contribution guidance, security reporting policy, support guidance, formatting/editor settings, ignore rules, and text attributes.
- MIT License: officially selected and committed in `LICENSE` with clear copyright ownership and provenance.
- Native build foundation: CMake 3.23+, C++20 standard, CTest registration, Debug/Release presets, strict compiler warnings, and Ninja build integration.
- CI quality gates: automated GitHub Actions workflow running C++ engine builds & CTest on Linux (GCC), Python FastAPI backend test suite, and Node.js frontend Vitest suite and production build.
- Core chess primitives: strongly typed colors, pieces, files, ranks, squares, bitboards, masks, and compact move encoding with exhaustive primitive tests.
- Position state and FEN parser: transactional FEN serialization and deserialization with structured errors, piece and occupancy bitboards with mailbox consistency, and reversible make/unmake move execution with `UndoState`.
- Attack and pseudo-legal move generation: compile-time leaper attack lookup tables, ray-stepping slider attacks, square attack predicates, bounded `MoveList`, and pseudo-legal move generation covering all pieces, captures, promotions, en-passant, and castling paths.
- Legal move generation and perft certification: king-safety filtering, castling path and check legality, en-passant discovered check prevention, check/checkmate/stalemate/fifty-move draw evaluation, and full perft and divide certification against canonical CPW benchmark suites.
- Baseline agents and match harness: `Agent` interface, reproducible `RandomAgent` with injectable seed, 1-ply `GreedyAgent` using material balance, configurable `MaterialWeights`, and standalone match runner `rooksac_match_harness`.
- Minimax and Negamax game-tree search: side-to-move-relative depth-limited negamax search, terminal state scoring (checkmate, stalemate, 50-move rule), mate-distance adjustments (`kMateScore - ply`), configurable leaf evaluators, `NegamaxAgent`, and search statistics reporting.
- Alpha-Beta pruning and move ordering: fail-soft Alpha-Beta search with cutoffs, staged move ordering (capture-promotions, promotions, MVV-LVA captures, quiet moves), unpruned differential testing path (`searchMinimax`), and substantial node reduction verified.
- Iterative deepening, quiescence, and time management: progressive deepening from depth 1 upward with PV-first move ordering, tactical quiescence search with check evasions and stand-pat rules, dedicated `TimeManager` with 100 ms hard reserve, emergency panic threshold (1500 ms), capped increment bonus, cooperative `SearchStopToken`, and guaranteed legal fallback moves.
- UCI protocol and standalone executable: Universal Chess Interface protocol implementation (`rooksac::uci::EngineSession`) supporting `uci`, `isready`, `ucinewgame`, `position`, `go` (depth, movetime, wtime/btime/winc/binc, nodes, infinite), `stop`, and `quit`; strict stdout stream discipline, stderr diagnostic routing, per-depth UCI info output, and standalone `rooksac` executable.
- Zobrist hashing and transposition table: deterministic 64-bit Zobrist keys updated incrementally on make/unmake and verified with full recalculation; memory-bounded `TranspositionTable` with 16-byte packed entries, depth-preferred replacement with generational aging, mate score ply normalization, and configurable UCI `Hash` option.
- Classical tapered evaluation: tapered `(mg, eg)` score evaluation blended by non-pawn material game phase (24 total), colour-mirrored Piece-Square Tables (PSTs), pawn structure terms (isolated, doubled, passed), king safety penalties (missing shield pawns, semi-open files), piece activity (bishop pair, open rooks), safe mobility, and centralized `EvalParameters`.
- Advanced classical search and benchmark suite: aspiration windows with fail-soft widening, Principal Variation Search (PVS), Late Move Reductions (LMR) with full-depth re-search, null-move pruning (NMP) with zugzwang safeguards, killer move heuristic (2 slots/ply), history heuristic table, `SearchOptions` feature isolation, canonical 8-position fixed FEN benchmark suite, standalone `rooksac_bench` executable, and protocol-safe UCI `bench` command.
- Texel evaluation tuning, self-play matches, and SPRT: versioned evaluation parameter JSON serialization/deserialization schema, Texel sigmoid probability mapping and Mean Squared Error objective calculation, whole-game position dataset extraction and splitting (`tools/pgn_extract.py`), and Sequential Probability Ratio Test (SPRT) statistical decision engine in C++ and Python (`tools/sprt_match.py`).
- Supervised policy-value neural network pipeline: offline PyTorch residual CNN architecture (`PolicyValueNet`) with 18-plane feature encoding, 4096-action move vocabulary, joint policy-value loss training pipeline, whole-game zero-leakage dataset validation, and model checkpointing (`research/nn/`).
- Native neural evaluation and policy move ranking: self-contained native C++ neural inference module (`NeuralEvaluator`), 18-plane feature tensor extraction (`encodePosition`), value calibration mapping $v \in [-1, 1]$ to engine centipawns with side-to-move perspective, legal-only policy move ordering (`orderMovesByPolicy`), and selectable evaluation modes (`rooksac::neural::EvalMode`).
- NNUE incremental accumulator evaluation research track: sparse 832-feature piece-square schema, int16 hidden accumulator with incremental update rules (`updateAccumulator`), exact make/unmake parity certification, and perspective symmetry verification (`rooksac::nnue::NnueEvaluator`).
- Scalable FastAPI backend and bounded engine orchestration: modular backend architecture (`app/backend/`), bounded worker pool (`BoundedEnginePool`) using `asyncio.Semaphore` with persistent worker process reuse, eliminating process startup overhead, versioned REST APIs (`/api/v1/health`, `/api/v1/analysis`, `/api/v1/pgn/import`), and safe stdio UCI communication.
- Frontend web application: React 19 single-page client under `app/frontend/` with interactive chessboard, real-time analysis display, PGN viewer, and WebSocket client.

## Current focus & readiness

- **Production Readiness**: Verified across all quality gates:
  - 15/15 native C++ CTest test suites pass (0 failures), including core types, position, movegen, perft regression, minimax/alpha-beta search, UCI protocol, Zobrist TT, evaluation, tuning, neural evaluation, NNUE accumulator, benchmark, and tactical puzzle suites.
  - 10/10 FastAPI backend integration tests pass, verifying health readiness, rate limiting, depth bounds, and PGN payload ingestion.
  - 22/22 ML and research pipeline tests pass, covering planar tensor parity, streaming PGN extraction, and residual CNN training.
  - 7/7 tool and deployment smoke tests pass, including Dockerfile syntax and container contracts.
  - Frontend TypeScript typechecking, linting, and 19/19 Vitest unit tests pass cleanly.
  - Complete evidence and verification instructions recorded in [PRODUCTION_CHECKLIST.md](PRODUCTION_CHECKLIST.md).
