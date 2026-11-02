# ADR-024 — NNUE incremental accumulator evaluation research track

- **Status:** Accepted
- **Context / problem:** Efficiently Updatable Neural Networks (NNUE) utilize sparse piece-square feature representations and incremental accumulator updates to provide high-throughput neural evaluation inside Alpha-Beta search. We must evaluate whether NNUE is feasible and desirable for RookSac while preserving existing classical and CNN evaluators.
- **Options considered:**
  1. Deferring NNUE completely in favor of purely classical or heavy CNN evaluation.
  2. Adopting third-party pre-trained network files and code (e.g., Stockfish NNUE) without independent provenance.
  3. Implementing an independent, modular NNUE prototype in C++ (`rooksac::nnue::NnueEvaluator`) with sparse piece-square features, fixed-size int16 accumulators, incremental update arithmetic, and rigorous unmake parity tests.
- **Decision:** Implement option 3 as a selectable research evaluation track:
  - **Feature Schema:** Sparse $(13 \text{ pieces} \times 64 \text{ squares} = 832)$ piece-square input features.
  - **Incremental Accumulators:** $\mathbf{a}_{t+1} = \mathbf{a}_t + \mathbf{w}_{\text{added}} - \mathbf{w}_{\text{removed}}$ applied for moves, captures, and promotions; exactly verified against full recomputations.
  - **Independent Clean Architecture:** Zero copied code or third-party proprietary weights. All weights are trained or calibrated within the RookSac provenance boundary.
  - **Evaluator Multiplicity:** Classical tapered evaluation and CNN neural evaluation remain fully available as defaults.
- **Consequences:**
  - Complete mathematical verification of incremental updates matching full scratch calculations across make/unmake sequences.
  - Clear architectural foundation for future high-speed integer SIMD vectorized accumulator operations if deeper NNUE architectures are adopted.
