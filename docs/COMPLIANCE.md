# RookSac — Rules & Competition Compliance Matrix

## 1. Context & Purpose

This document audits RookSac against standard collegiate, academic, and computer chess tournament guidelines.

Common tournament rules typically establish:

1. **Originality**: The engine logic, search, and evaluation code must be the authors' own work. Wholesale copying or cloning of open-source engines (e.g., Stockfish, Crafty, Fruit) is strictly prohibited.
2. **Third-Party Code & Libraries**: No external chess-specific libraries or evaluation engines may be linked at runtime. Standard system libraries (C++ STL) are allowed.
3. **Opening Books & Tablebases**: Precomputed opening books or endgame tablebases (Syzygy, Nalimov) are generally prohibited unless explicitly specified in competition rules.
4. **Pretrained Models**: Any machine-learning weights or evaluation models must either be trained directly by the team or explicitly disclosed with verifiable training pipelines and dataset provenance.
5. **Runtime Network Access**: The engine must operate entirely standalone without network connections, cloud API calls, or external compute clusters during match play.
6. **Protocol & Conformance**: Standard UCI protocol must be implemented faithfully without illegal moves or clock forfeits.

---

## 2. Rule-by-Rule Compliance Matrix

| Rule Area                             | Requirement                                                                              | Current RookSac Status                                                              | Evidence / Implementation Details                                                                                                                                                                                                   |  Compliance Verdict  |
| ------------------------------------- | ---------------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- | :------------------: |
| **Originality & Independence**        | No copied engine code; independent algorithmic implementation.                           | Independent implementation of standard computer chess algorithms.                   | Git history and source inspection confirm bitboards, movegen, alpha-beta, PVS, LMR, and TT are written natively in C++20. No Stockfish or foreign engine source trees.                                                              |       **PASS**       |
| **Runtime Dependencies**              | Standalone executable; no third-party chess libraries.                                   | Zero external runtime libraries in C++ engine.                                      | Uses C++20 STL only (`<cstdint>`, `<vector>`, `<array>`, `<algorithm>`, `<chrono>`, `<thread>`, `<atomic>`). Links only `libstdc++` / system runtime.                                                                               |       **PASS**       |
| **Opening Books**                     | No unauthorized precomputed opening book files.                                          | No opening book is bundled or loaded.                                               | Search commences from root board state via legal move generator without opening book lookup tables.                                                                                                                                 |       **PASS**       |
| **Endgame Tablebases**                | No external Syzygy / Nalimov tablebases.                                                 | No tablebase probes.                                                                | All endgame positions are solved directly via alpha-beta search and material heuristics. Dead-position detection ($K$ vs $K$, $K+B$ vs $K$, $K+N$ vs $K$) is implemented purely via bitboard counts.                                |       **PASS**       |
| **Network & API Isolation**           | Engine must run offline with no runtime HTTP/socket calls.                               | Fully isolated C++ binary.                                                          | Engine communicates exclusively over standard input/output (`std::cin` / `std::cout`). No socket headers (`<sys/socket.h>`, `<winsock2.h>`, or HTTP clients) exist in `engine/`.                                                    |       **PASS**       |
| **Evaluation Data Provenance (PSTs)** | Legality and disclosure of piece-square table constants.                                 | Public domain PeSTO / CPW classical tables documented with attribution.             | Initial default parameters in `engine/src/evaluation.cpp` are based on Tomasz Michniewski's public-domain PeSTO tables (standard educational reference). Disclosed in `docs/PROVENANCE.md` and `docs/legal/THIRD_PARTY_NOTICES.md`. | **PASS (Disclosed)** |
| **Pretrained Models & ML Weights**    | Pretrained neural models must be disclosed and trained by the team.                      | No foreign weights bundled. Native C++ neural evaluator is a placeholder.           | `research/checkpoints/policy_value_test.pt` is a small synthetic test artifact. `NeuralEvaluator::loadModel()` is disconnected from real weight files, preventing undisclosed black-box models.                                     |       **PASS**       |
| **Protocol Compliance (UCI)**         | Must support standard UCI handshake, position setup, clock go, stop, and clean bestmove. | Certified UCI conformance.                                                          | 21/21 UCI tests pass cleanly. `rooksac.exe` handles `go depth`, `movetime`, `nodes`, `wtime/btime/winc/binc`, `stop`, and returns valid algebraic moves. Diagnostic messages routed to `stderr`.                                    |       **PASS**       |
| **Chess Rules Correctness**           | Full FIDE legal moves, check evasion, draw rules, and checkmate priority.                | Comprehensive legal movegen and draw rules implemented.                             | Perft suite matches canonical node counts; checkmate takes precedence over fifty-move rule; search-path repetition and insufficient material dead positions are detected.                                                           |       **PASS**       |
| **Time Management & Clock Safety**    | No time forfeits under competition clock controls (3+2, 10+0, 10+5).                     | 100 ms hard safety reserve; panic threshold $\le 1500\text{ ms}$; 20% max-time cap. | Tested across simulated 3+2 games; zero clock forfeits; fast-exits on single legal move to preserve clock.                                                                                                                          |       **PASS**       |

---

## 3. Unresolved Questions & Next Steps

1. **Competition Guidelines Review**: If specific competition regulations mandate that all evaluation tables (PSTs) must be tuned exclusively by the team (disallowing PeSTO tables), RookSac's Texel tuning harness (`engine/src/tuning.cpp`) will be run in Sprint 04 to generate original weights.
2. **ML Evaluation Readiness**: The research PyTorch model is currently experimental. For competition games, the classical tapered evaluator (`rooksac::eval`) must remain the active production evaluation engine.
3. **Stand-Alone Binary Requirement**: Matches must run using the compiled native binary (`./build/rooksac` or `rooksac.exe`) directly via a UCI tournament harness (e.g., `cutechess-cli`), bypassing the Python backend and web layers.
