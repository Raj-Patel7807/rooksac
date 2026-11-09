# Changelog

All notable changes to RookSac are documented here.
Format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).
Versioning follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [Unreleased]

### Added

- Per-depth UCI `info depth … score cp … nodes … nps … time … pv` lines emitted after every
  completed iterative-deepening iteration, matching standard GUI expectations.
- `DepthCallback` type alias in `search.hpp`; optional 7th parameter on `search::search()`.

---

## [0.1.0] — 2026-11-03

First complete, production-ready release of the RookSac chess engine and web application.

### Engine (C++20)

#### Added

- Core chess primitives: `Square`, `Piece`, `Color`, `Move` (compact 16-bit encoding), `MoveList`
  bounded stack, bitboard helpers, and compile-time leaper attack tables.
- `Position` with piece bitboards, square mailbox, side-to-move, castling rights, en-passant
  target, ply/fifty-move counters, and transactional FEN parsing / serialization.
- Reversible `makeMove` / `unmakeMove` with full `UndoState` including incremental Zobrist hash
  updates.
- Compile-time leaper attack generation; ray-stepping slider attacks; square-attack detection.
- Staged pseudo-legal move generation (captures, quiet moves, special moves).
- Legal move filtering via make-check-unmake; check, checkmate, stalemate, and fifty-move draw
  detection.
- Perft certification suite validated against known node counts at depths 1–5 for multiple
  positions.
- `RandomAgent` and `GreedyAgent` baseline policies with match harness.
- Fail-soft Negamax search with iterative deepening, aspiration windows, and PV extraction.
- Quiescence search resolving capture / promotion chains; cooperative stop token.
- Staged move ordering: PV-move first (score 40 000), then MVV-LVA captures, then quiet moves
  ordered by killer heuristic, history heuristic, and PST deltas.
- Alpha-Beta pruning with null-move pruning, late-move reduction (depth-scaled formula), and
  principal variation search.
- Classical tapered evaluation: PSTs, pawn structure (doubled, isolated, passed), king safety,
  mobility, and game-phase blending.
- Zobrist hashing with 64 MB transposition table (4 M entries) and generational aging.
- `SearchOptions` compile-time configuration; `SearchBenchmark` perft + speed harness.
- Texel tuning: MSE objective, sigmoid, SPRT statistical testing, and parameter serializer.
- Native neural evaluator: 18-plane feature encoding, HalfKP-style NNUE sparse accumulator with
  incremental updates, and value calibration.
- Supervised policy-value network (PyTorch, 18-plane input) with training pipeline and dataset
  split isolation.
- Universal Chess Interface (`EngineSession`): `uci`, `isready`, `position`, `go depth/movetime`,
  `stop`, `setoption`, `ucinewgame`, `quit`; `setoption name Hash value N` (MB).
- Standalone `rooksac` UCI executable and `rooksac_bench` benchmark binary.
- Match execution harness (`rooksac_match_harness`).
- History heuristic: 12 × 64 table updated on beta cutoffs with depth² bonus.
- Depth-scaled LMR (`base + depth/5`); progressive aspiration window doubling on failure.

### Backend (Python / FastAPI)

#### Added

- FastAPI service with `/api/v1/health`, `/api/v1/analysis`, `/api/v1/pgn/import` endpoints.
- `BoundedEnginePool` with persistent UCI worker processes; eliminates ~800 ms per-request
  startup overhead.
- Graceful `shutdown()` lifecycle method for worker cleanup.
- `resolve_engine_binary()` with environment override, build-directory scan, and PATH fallback.
- PGN parser (`tools/pgn_extract`) with result validation; import endpoint with size guard.
- CORS, request-size, and depth-limit guards.

### Frontend (React / TypeScript)

#### Added

- React TypeScript chess application: interactive board, play mode, analysis mode, and saved games.
- Production build with Vite; unit tests with Vitest.

### Infrastructure

#### Added

- CMake C++20 build with Ninja; `cmake --preset release` and `cmake --preset debug`.
- CTest suite: 13 test targets across primitives, position, movegen, perft, agents, search, UCI,
  transposition table, evaluation, tuning, neural evaluation, and NNUE.
- GitHub Actions CI: C++ build + CTest (Ubuntu, GCC), Python backend + ML tests, and frontend
  build + test.
- `.clang-format`, `.editorconfig`, `.gitattributes`, and `.gitignore`.
- Production multi-stage Docker packaging with security hardening.
- Architecture Decision Records (ADRs) 001–025 covering all major design decisions.

### Performance (measured on release build, GCC 14.2.0)

| Benchmark                   | Before                  | After                   |
| --------------------------- | ----------------------- | ----------------------- |
| `rooksac_bench` depth-4 NPS | ~1 383 000              | ~1 469 000 (+6%)        |
| Startpos depth 8 wall time  | ~800 ms (incl. startup) | 112 ms                  |
| Kiwipete depth 8 wall time  | ~2 260 ms               | ~1 780 ms (−21%)        |
| Backend per-request startup | ~800 ms                 | ~0 ms (persistent pool) |

---

[Unreleased]: https://github.com/Raj-Patel7807/rooksac/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/Raj-Patel7807/rooksac/releases/tag/v0.1.0
