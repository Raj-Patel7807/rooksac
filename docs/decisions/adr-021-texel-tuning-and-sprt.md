# ADR-021 — Texel evaluation tuning, self-play matches, and SPRT

- **Status:** Accepted
- **Context / problem:** Handcrafted evaluation terms and weights provide a baseline, but fine-tuning parameters requires objective fitting against game outcomes and rigorous statistical verification. Lower prediction error on a training set alone does not guarantee playing strength; controlled self-play with Sequential Probability Ratio Testing (SPRT) is required before adopting new parameter configurations.
- **Options considered:**
  1. Manual iterative parameter tweaking based on subjective play.
  2. Ad-hoc genetic algorithms without statistical testing gates.
  3. Texel tuning with Mean Squared Error (MSE) objective on quiet positions, whole-game dataset splits (train/val/test), versioned JSON schema serialization, and SPRT hypothesis testing for match gating.
- **Decision:** Implement option 3:
  - **Texel Sigmoid:** $S(\text{eval}, K) = \frac{1}{1 + 10^{-K \cdot \text{eval} / 400}}$, mapping White-perspective static evaluation in centipawns to expected win probability.
  - **MSE Objective:** $\frac{1}{N} \sum (\text{result}_i - S(\text{eval}_i, K))^2$ computed over quiet, non-tactical extracted positions.
  - **Whole-Game Splitting:** Dataset extraction tool (`tools/pgn_extract.py`) clusters positions by unique game identifiers to strictly eliminate position leakage across train, validation, and test sets.
  - **Versioned Parameter Schema:** JSON serialization/deserialization for `EvalParameters` enabling reproducible parameter configurations and diffing.
  - **SPRT Match Verification:** Wald Sequential Probability Ratio Test implemented in C++ (`rooksac::tuning::evaluateSprt`) and Python (`tools/sprt_match.py`) testing $H_0: \text{Elo}_0$ vs $H_1: \text{Elo}_1$ with type I/II error rates ($\alpha=0.05, \beta=0.05$) on colour-balanced self-play matches.
- **Consequences:**
  - Evaluation parameters can be adjusted via external JSON manifests and tuned reproducibly.
  - Candidate weight sets must pass SPRT statistical match gates before adoption into `rooksac_core`.
  - Machine and tuning manifest metadata preserve full provenance of any future weight adjustments.
