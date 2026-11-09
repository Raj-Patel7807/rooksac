# RookSac — Baseline Verification & Component Inventory

## 1. Test Execution Baseline

All tests were executed against the current `main` branch.

### 1.1 C++ Core Engine Test Suite (CTest)

**Command:**

```powershell
ctest --test-dir build --output-on-failure
```

**Outcome: 13 / 13 tests PASSED (real time: 0.96s)**

| #   | Test Name             | Target Executable              |   Result   | Elapsed Time | Coverage                                                  |
| --- | --------------------- | ------------------------------ | :--------: | :----------: | --------------------------------------------------------- |
| 1   | `rooksac.build-smoke` | `rooksac_build_smoke_test.exe` | **PASSED** |    0.04s     | Header compilation and basic instantiation                |
| 2   | `rooksac.core-types`  | `rooksac_core_types_test.exe`  | **PASSED** |    0.04s     | Bitboards, squares, moves, piece encoding                 |
| 3   | `rooksac.position`    | `rooksac_position_test.exe`    | **PASSED** |    0.03s     | FEN parsing/serialization, make/unmake invariants         |
| 4   | `rooksac.movegen`     | `rooksac_movegen_test.exe`     | **PASSED** |    0.03s     | Attack tables, pseudo-legal moves, check evasion          |
| 5   | `rooksac.perft`       | `rooksac_perft_test.exe`       | **PASSED** |    0.13s     | Canonical Perft positions (Pos 1–5, Kiwipete)             |
| 6   | `rooksac.agent`       | `rooksac_agent_test.exe`       | **PASSED** |    0.08s     | Agent interfaces, RandomAgent, GreedyAgent                |
| 7   | `rooksac.search`      | `rooksac_search_test.exe`      | **PASSED** |    0.15s     | Alpha-beta, PVS, LMR, NMP, TT, mate scores, repetition    |
| 8   | `rooksac.uci`         | `rooksac_uci_test.exe`         | **PASSED** |    0.22s     | Protocol handshake, position parsing, go commands         |
| 9   | `rooksac.zobrist_tt`  | `rooksac_zobrist_tt_test.exe`  | **PASSED** |    0.07s     | Zobrist hash invariants, TT replacement, stale moves      |
| 10  | `rooksac.eval`        | `rooksac_eval_test.exe`        | **PASSED** |    0.05s     | Tapered eval, PSTs, pawn structure, insufficient material |
| 11  | `rooksac.tuning`      | `rooksac_tuning_test.exe`      | **PASSED** |    0.03s     | Texel tuning loss evaluation, parameter extraction        |
| 12  | `rooksac.neural_eval` | `rooksac_neural_eval_test.exe` | **PASSED** |    0.03s     | 18-plane tensor encoding, native evaluator scaffolding    |
| 13  | `rooksac.nnue`        | `rooksac_nnue_test.exe`        | **PASSED** |    0.03s     | 832-feature accumulator, incremental updates              |

---

### 1.2 UCI Protocol Conformance Suite

**Command:**

```powershell
python scratch/uci_test.py
```

**Outcome: 21 / 21 checks PASSED (0 failures)**

- Verified `uci`, `isready`, `ucinewgame`, `position startpos [moves ...]`, `position fen`.
- Verified `go depth`, `movetime`, `nodes`, `wtime/btime`, `infinite`, and cooperative `stop`.
- Confirmed `stdout` remains strictly protocol-compliant; diagnostics routed to `stderr`.

---

### 1.3 Backend Integration Test Suite

**Command:**

```powershell
python -m unittest discover -s app/backend/tests
```

**Outcome: 4 / 4 tests PASSED (0.103s)**

- `test_root_endpoint`: `GET /` returns 200 and API metadata.
- `test_health_endpoint`: `GET /health` reports engine available.
- `test_new_game_endpoint`: `POST /api/game/new` initializes session.
- `test_analysis_endpoint_contract`: `POST /api/analysis` returns valid analysis response contract.

---

### 1.4 Frontend Test Suite

**Command:**

```powershell
cd app/frontend; npm test -- --run
```

**Outcome: 9 / 9 tests PASSED (3 test files, duration 24.36s)**

- `src/test/websocket.test.ts`: 3 / 3 passed.
- `src/test/client.test.ts`: 3 / 3 passed.
- `src/test/components.test.tsx`: 3 / 3 passed.

---

### 1.5 Research Test Suite

**Command:**

```powershell
python -m unittest discover -s research/tests
```

**Outcome: 5 / 5 tests PASSED (0.564s)**

- `test_feature_extraction`: 18-plane board tensor shape `(18, 8, 8)`.
- `test_action_encoding`: Move to index / index to move bijective mapping.
- `test_model_forward_pass`: Output shapes for policy head `(B, 4672)` and value head `(B, 1)`.
- `test_dataset_loading`: PGN parsing and batch collation.
- `test_training_step`: Loss computation and backward gradient pass.

---

### 1.6 Performance Benchmark Baseline

**Command:**

```powershell
.\build\rooksac_bench.exe --depth 4 --repetitions 1
```

**Outcome:**

- **Positions**: 8 canonical benchmark positions (Startpos, Kiwipete, Tactical-Midgame, Rook-Endgame, Queen-Endgame, Knight-Pawn, Passed-Pawns, Kingside-Attack).
- **Total Nodes**: 164,333 nodes.
- **Total Elapsed Time**: 114 ms.
- **Overall NPS**: ~1,441,517 nodes/sec (Release build with GCC 14.2.0 on Windows 11).

---

## 2. Component Inventory

| Component                            | File Path                                    | Status                   | Verification Evidence                                                  |
| ------------------------------------ | -------------------------------------------- | ------------------------ | ---------------------------------------------------------------------- |
| **Bitboard Engine**                  | `engine/include/rooksac/bitboard.hpp`        | **Implemented & Tested** | Full unit tests in `core-types-test.cpp`                               |
| **Position & Mailbox**               | `engine/src/position.cpp`                    | **Implemented & Tested** | `position-test.cpp`, Perft tests                                       |
| **Zobrist Hashing**                  | `engine/src/zobrist.cpp`                     | **Implemented & Tested** | `zobrist-tt-test.cpp`                                                  |
| **Legal Move Generation**            | `engine/src/legal_movegen.cpp`               | **Implemented & Tested** | `perft-test.cpp` (canonical suites match)                              |
| **Iterative Deepening Search**       | `engine/src/search.cpp`                      | **Implemented & Tested** | `search-test.cpp`                                                      |
| **PVS / Alpha-Beta**                 | `engine/src/search.cpp`                      | **Implemented & Tested** | `search-test.cpp` (minimax equivalence tests)                          |
| **Null-Move Pruning (NMP)**          | `engine/src/search.cpp`                      | **Implemented & Tested** | `search-test.cpp` (`testNullMovePruningSafety`)                        |
| **Late Move Reductions (LMR)**       | `engine/src/search.cpp`                      | **Implemented & Tested** | `search-test.cpp`                                                      |
| **Quiescence Search**                | `engine/src/search.cpp`                      | **Implemented & Tested** | `search-test.cpp` (tactical horizons and check evasions)               |
| **Transposition Table**              | `engine/src/transposition_table.cpp`         | **Implemented & Tested** | `zobrist-tt-test.cpp` (stale move clearing tested)                     |
| **3+2 Time Management**              | `engine/src/time_management.cpp`             | **Implemented & Tested** | `search-test.cpp`, `clock_test.py` (7 scenarios)                       |
| **Classical Tapered Eval**           | `engine/src/evaluation.cpp`                  | **Implemented & Tested** | `eval-test.cpp`                                                        |
| **Draw Rule: Fifty-Move**            | `engine/src/legal_movegen.cpp`, `search.cpp` | **Implemented & Tested** | `search-test.cpp` (checkmate precedence verified)                      |
| **Draw Rule: Insufficient Material** | `engine/src/material.cpp`                    | **Implemented & Tested** | `eval-test.cpp` (13 test cases)                                        |
| **Draw Rule: Repetition**            | `engine/src/search.cpp`                      | **Implemented & Tested** | `search-test.cpp` (search path cycle detection)                        |
| **UCI Engine Protocol**              | `engine/src/uci.cpp`                         | **Implemented & Tested** | `uci-test.cpp`, `uci_test.py` (21/21 passed)                           |
| **Texel Tuning Framework**           | `engine/src/tuning.cpp`                      | **Implemented & Tested** | `tuning-test.cpp`                                                      |
| **Native Neural Evaluator**          | `engine/src/neural_evaluator.cpp`            | **Placeholder**          | `loadModel()` does not load weights; returns sigmoid of classical eval |
| **NNUE Accumulator**                 | `engine/src/nnue.cpp`                        | **Implemented (Unused)** | `nnue-test.cpp` passes, but not linked into active search path         |
| **PyTorch Training Pipeline**        | `research/nn/train.py`                       | **Implemented & Tested** | `test_nn_pipeline.py` (5/5 tests pass)                                 |
| **FastAPI Backend Service**          | `app/backend/main.py`                        | **Implemented & Tested** | `app/backend/tests` (4/4 tests pass)                                   |
| **React Chess Frontend**             | `app/frontend/src/App.tsx`                   | **Implemented & Tested** | `vitest` (9/9 tests pass)                                              |

---

## 3. Top 10 High-Risk Correctness & Strength Issues

Ranked by impact and codebase evidence:

1. **Neural Inference Disconnect (High Impact / Architectural)**:
    - _Evidence_: `engine/src/neural_evaluator.cpp` line 81 ignores `modelPath` and returns placeholder values.
    - _Risk_: Misleading status claims; no actual neural inference occurs in production C++.
    - _Plan_: Sprint 09 will implement true ONNX or native quantized tensor inference.

2. **Time Control Generalization Beyond 3+2 (High Impact / Strength)**:
    - _Evidence_: `TimeManager::allocate()` was primarily optimized for 3+2 blitz. Longer classical controls (e.g., 10+0, 10+5, 40/90) or variable increment profiles have not been comprehensively calibrated against external engines.
    - _Plan_: Sprint 05 will calibrate multi-time-control profiles.

3. **Absence of Search Extensions (Medium-High Impact / Strength)**:
    - _Evidence_: `engine/src/search.cpp` implements reductions (LMR) and null-move pruning, but does not implement check extensions or singular extensions.
    - _Risk_: Risk of horizon tactical blindness in sharp tactical lines.
    - _Plan_: Sprint 03 will benchmark check extensions.

4. **PST Constant Provenance & Originality (Medium Impact / Compliance)**:
    - _Evidence_: Initial evaluation parameters are borrowed from PeSTO / CPW classical PSTs.
    - _Risk_: Potential originality concern in strict collegiate competitions requiring 100% self-generated weights.
    - _Plan_: Sprint 04 will tune custom PSTs via Texel tuning on clean datasets.

5. **Evaluation Mobility & King Safety Granularity (Medium Impact / Strength)**:
    - _Evidence_: `engine/src/evaluation.cpp` evaluates king safety via pawn shields and open files, but lacks attack-ray king zone calculations and complex mobility metrics.
    - _Risk_: Engine can underestimate opposite-side castling attacks or long-range bishop pressure.
    - _Plan_: Sprint 04 will refine positional terms.

6. **Tactical Puzzle Suite Accuracy (Medium Impact / Strength)**:
    - _Evidence_: Tactical test suite (`scratch/tactical_test.py`) passes 5/8 test positions (3 positions choose alternate mate-in-2 lines or equal tactical payoffs).
    - _Risk_: Move ordering in complex tactical trees could be sharper.
    - _Plan_: Sprint 03 will implement counter-move and capture ordering improvements.

7. **Cross-Engine SPRT Rating Validation Missing (Medium Impact / Strength)**:
    - _Evidence_: No continuous self-play / SPRT match runner integration exists in the repository.
    - _Risk_: Engine improvements cannot be definitively measured in Elo increments.
    - _Plan_: Sprint 06 will integrate an automated match harness with Ordo / Bayes-Elo rating estimation.

8. **Hard-Coded Engine Binary Discovery in Backend (Low-Medium Impact / Stability)**:
    - _Evidence_: `app/backend/config.py` searches a hard-coded list of build paths (`build/rooksac`, `build/rooksac.exe`). If built with different build presets or paths, the backend fails to connect.
    - _Plan_: Sprint 07 will make the binary path explicitly configurable via environment variables.

9. **Frontend User Experience & Clock Sync (Low-Medium Impact / Usability)**:
    - _Evidence_: The frontend uses a basic SVG board and polling/WebSocket interface, but lacks modern premove handling, board sounds, and responsive clock synchronization.
    - _Plan_: Sprint 07 will overhaul the web UI experience.

10. **NNUE Research vs Engine Integration (Low Impact / Optimization)**:
    - _Evidence_: `engine/src/nnue.cpp` has working accumulator logic, but is completely bypassed by the search loop.
    - _Plan_: Sprint 09 will integrate the NNUE accumulator once real weights are trained.
