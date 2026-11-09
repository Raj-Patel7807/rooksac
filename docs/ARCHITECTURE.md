# RookSac — System Architecture & Runtime Data Flow

## 1. Overview

RookSac is a computational chess system comprising:

1. **Engine Core (`engine/`)**: A self-contained, dependency-free C++20 chess engine exposing a UCI protocol interface.
2. **Web Backend (`app/backend/`)**: A FastAPI Python service providing REST and WebSocket endpoints for human-vs-bot games and position analysis by orchestrating the engine via UCI.
3. **Web Frontend (`app/frontend/`)**: A React / Vite single-page application communicating with the backend over HTTP and WebSockets.
4. **Research Subsystem (`research/`)**: PyTorch-based neural network model definition, 18-plane feature extraction, training pipeline, and checkpoint utilities for experimental policy-value learning.

---

## 2. Engine Subsystem Architecture (`engine/`)

### 2.1 Core Types & Bitboard Representation

- **Bitboards (`rooksac::Bitboard`)**: 64-bit unsigned integers representing piece occupancy (`rooksac/bitboard.hpp`). Includes rank, file, color, and square bitmasks.
- **Mailbox & Dual Representation (`rooksac::Position`)**: Maintains 64-element mailbox array alongside `pieceBitboards_` (12 piece types) and `colorOccupancy_` (2 colors) plus `totalOccupancy_`.
- **Zobrist Hashing (`rooksac::zobrist`)**: 64-bit pseudo-random keys for pieces on squares, side-to-move, 4 castling rights bits, and 8 en-passant file targets. Updated incrementally on `makeMove` / `unmakeMove` and verified against full recomputation.

### 2.2 Move Generation & Rules Validation

- **Pseudo-Legal Move Generation (`rooksac::movegen`)**: Attack tables (precomputed knight, king, pawn attacks; ray attacks for sliding pieces). Generates pseudo-legal captures and quiets.
- **Full Legal Validation (`rooksac::legal_movegen`)**: `isInCheck`, `isKingInCheck`, `isLegalMove`, and `generateLegalMoves`. Verifies king safety, castling transit squares, and en-passant legality.
- **Draw & Game Termination Detection**:
    - Checkmate: King in check and zero legal moves (`-kMateScore + ply`).
    - Stalemate: King not in check and zero legal moves (`kDrawScore = 0`).
    - Fifty-Move Rule: `halfmoveClock >= 100` (`kDrawScore`). Checkmate strictly takes precedence.
    - Insufficient Material: Dead positions ($K$ vs $K$, $K+B$ vs $K$, $K+N$ vs $K$, same-color $K+B$ vs $K+B$) detected via `eval::hasInsufficientMaterial()`.
    - Search-Path Repetition: Ancestor path cycle detection (`historyPath`) within active alpha-beta branches.

### 2.3 Search Architecture (`rooksac::search`)

- **Iterative Deepening**: Root search iterates from depth 1 up to maximum requested depth or time limit.
- **Principal Variation Search (PVS)**: Searches first move with full window; subsequent moves with null window `[-(alpha+1), -alpha]`, re-searching with full window on fail-high.
- **Aspiration Windows**: Narrow search window centered on the previous iteration's score (`[score - delta, score + delta]`) with progressive doubling on fail-low/fail-high.
- **Pruning & Heuristics**:
    - Null-Move Pruning (NMP): Evaluates passing turn when not in check and non-pawn material exists.
    - Late Move Reductions (LMR): Depth-scaled reductions for late quiet moves.
    - Quiescence Search: Tactical captures ordered by MVV-LVA; in-check evasion search at depth boundary.
    - Killer Heuristic & History Heuristic: Prioritizes quiet moves that caused cutoffs.
- **Transposition Table (`rooksac::tt::TranspositionTable`)**:
    - 16-byte packed entries: 64-bit Zobrist key, 16-bit score (normalized to ply-independent mate distance), 8-bit depth, 8-bit bound (`Exact`, `Lower`, `Upper`), 16-bit encoded best move, 8-bit generation age.
    - Depth-preferred replacement with age generational replacement; stale moves explicitly cleared when an entry is overwritten by a different key without a best move.

### 2.4 Time Management (`rooksac::TimeManager`)

- **Time Controls**: Fixed move time (`movetime`), clock-based (`wtime`/`btime`, `winc`/`binc`, `movestogo`), or infinite.
- **Allocation Logic**:
    - Emergency panic mode for remaining clock $\le 1500\text{ ms}$ (allocates 30% of usable clock, skipping increment additions).
    - Standard mode allocates baseline time based on remaining clock / moves-to-go, plus 75% of increment.
    - Capped strictly at 20% of remaining clock and respects 100 ms hard safety reserve.
    - Adaptive termination in iterative deepening: breaks at depth $\ge 5$ if elapsed time exceeds 50% of optimal allocation.
    - Single legal move fast exit: immediately terminates at depth 1 when only one legal move exists.

### 2.5 Evaluation Systems

- **Classical Tapered Evaluation (`rooksac::eval`)**:
    - Interpolates between midgame (MG) and endgame (EG) phases using non-pawn piece material count.
    - Material weights + Piece-Square Tables (PSTs).
    - Positional terms: pawn structure (isolated, doubled, passed), mobility, king safety (shield, open files), bishop pair, rook on open/semi-open files.
- **Native Neural Evaluator Placeholder (`rooksac::NeuralEvaluator`)**:
    - _Current Status_: Fallback wrapper. Does NOT load weights or execute matrix multiplications; maps classical score through sigmoid to centipawns. Zero policy logits.
- **NNUE Accumulator Research (`rooksac::nnue::NnueEvaluator`)**:
    - Implements 832-feature piece-square input accumulator, Clipped ReLU activation, and incremental update hooks. Uses synthetic deterministic weights; experimental only.

---

## 3. UCI Protocol Architecture (`rooksac::uci`)

The engine communicates via stdin/stdout following standard UCI protocol:

- `uci` $\to$ `id name`, `id author`, `option name Hash`, `uciok`
- `isready` $\to$ `readyok`
- `ucinewgame` $\to$ resets position and clears TT
- `position [startpos|fen <fen>] [moves ...]` $\to$ sets root board state
- `go [depth|movetime|nodes|wtime|btime|winc|binc|movestogo|infinite]` $\to$ launches asynchronous search thread
- `stop` $\to$ triggers cooperative `SearchStopToken`, joins thread, emits `bestmove`
- `quit` $\to$ terminates session

UCI protocol adherence rules:

- All protocol tokens and `bestmove` output go strictly to `stdout`.
- Benchmark, debug, and info logs go to `stderr`.

---

## 4. Backend Architecture (`app/backend/`)

- **Framework**: FastAPI (Python 3.10+ / 3.14 compatible).
- **Process Orchestration (`app/backend/engine/wrapper.py`)**: Spawns engine subprocess (`rooksac.exe` or `rooksac`), communicates via asynchronous pipes using UCI protocol.
- **Endpoints**:
    - `GET /health`: Health and engine readiness check.
    - `POST /api/game/new`: Starts a new game session.
    - `POST /api/game/move`: Validates user move, queries engine via UCI, returns bot move and updated FEN.
    - `POST /api/analysis`: Runs deep analysis on a position FEN.
    - `WS /ws/play`: Real-time WebSocket game loop.

---

## 5. Frontend Architecture (`app/frontend/`)

- **Stack**: React 18, TypeScript, Vite, Tailwind CSS.
- **Components**:
    - `PlayScreen`: Interactive chessboard, move history, bot difficulty selector, clocks.
    - `AnalysisScreen`: Board evaluation bar, PV display, engine depth and NPS indicators.
    - `EngineClient`: API client managing WebSocket and REST requests to the backend.

---

## 6. Research Pipeline (`research/`)

- **Data Representation (`research/nn/features.py`)**: Encodes `chess.Board` into 18x8x8 planar float tensor (6 piece types $\times$ 2 colors + castling + ep + repetitions).
- **Neural Architecture (`research/nn/model.py`)**: `PolicyValueNetwork` with convolutional backbone, residual blocks, policy head (action space 4672 or 1968), and value head ($v \in [-1, 1]$).
- **Training Pipeline (`research/nn/train.py`)**: PGN parser, self-play / Lichess dataset loader, MSE value loss + cross-entropy policy loss.
- **Bridge to C++**: Checkpoint saved as `.pt` (PyTorch format). Currently disconnected from native C++ runtime (see Provenance and Compliance).
