# Release Notes — RookSac v1.0.0

## Overview

RookSac v1.0.0 is the inaugural production release of RookSac, an autonomous chess engine and modern web analysis platform.

## Highlights

### 1. High-Performance C++20 Chess Engine

- **Bitboard Board Representation**: Fast magic bitboards for sliding pieces (bishops, rooks, queens), branchless popcount, and full FEN parse/format support.
- **Move Generation**: Fully validated legal move generator verified against standard Perft suites up to depth 6.
- **Search Architecture**:
    - Principal Variation Search (PVS) with iterative deepening.
    - Aspiration windows for narrowing alpha-beta search bounds.
    - Late Move Reductions (LMR) and Null-Move Pruning (NMP) with piece/check safety guards.
    - Zobrist transposition table with depth-preferred replacement.
    - Move ordering with TT hits, MVV-LVA captures, and killer moves.
- **Evaluation**:
    - Classical tapered evaluation with Texel tuning support.
    - Optional zero-allocation neural leaf evaluation and experimental NNUE accumulator track.
- **UCI Protocol**: Standard UCI interface with `isready`, `position`, `go`, `stop`, and `bench` commands.

### 2. Scalable Asynchronous Backend API

- FastAPI service with an asynchronous `BoundedEnginePool`.
- Subprocess lifecycle management with timeout guards and graceful cleanup.
- Endpoints for health checking (`/api/v1/health`), engine analysis (`/api/v1/analysis`), and PGN ingestion (`/api/v1/pgn/import`).

### 3. Responsive Web Frontend

- Modular React 19 + TypeScript single-page application built on Vite.
- Play against the native engine, deep analysis board with win-probability centipawn evaluation bar, PGN import/export, and configurable engine search settings.
- Resilient WebSocket client with sequence tracking and stale-packet drop logic.

### 4. Hardened Production Deployment

- Multi-stage Docker packaging with non-root security principles.
- Internal service isolation with PostgreSQL 16 and Redis 7 on private bridge networks.
- Sliding window rate limiting, CORS controls, and security headers.

## Known Limitations

- Native engine is single-threaded per UCI process; concurrency is scaled horizontally via the worker process pool.
- Syzygy endgame tablebases are not bundled in v1.0.0.
