# Strength History and Rating Progression

This document tracks empirical playing strength measurements across sprints and releases under controlled match conditions.

---

## Controlled Evaluation Criteria

All records below conform to the protocol defined in [`docs/match_config.md`](match_config.md):
- **Concurrency**: 1 thread
- **Transposition Table**: 16 MB
- **Adjudication**: Standard chess rules (checkmate, stalemate, 50-move rule, insufficient material)
- **Openings**: Fixed 20-opening suite (`tools/openings.epd`), played with color reversals
- **SPRT Bounds**: $\text{elo}_0 = 0.0$, $\text{elo}_1 = +5.0$, $\alpha = 0.05$, $\beta = 0.05$

---

## Progression Table

| Match ID | Date | Candidate | Baseline | Conditions | Score (W/D/L) | Score % | $\Delta\text{Elo}$ [95% CI] | SPRT Decision | Artifact |
|:---|:---|:---|:---|:---|:---|:---|:---|:---|:---|
| **M-001** | 2026-11-09 | `NegamaxAgent` (depth 2) | `GreedyAgent` | Fixed Depth 2, 20 openings, paired | 20 / 0 / 0 | 100.0% | $+\infty$ [$+\infty$, $+2400.0$] | **ACCEPT H1** (Candidate stronger) | `docs/benchmarks/sprint06_internal.pgn` |

---

## Notes & Honest Strength Reporting Guidelines

1. **No Puzzle-to-Elo Extrapolations**: Tactical suites (e.g. WAC, STS) measure pattern recognition, not playing strength. Elo ratings are only reported from head-to-head game play under identical time controls.
2. **Inconclusive Outcomes**: If an SPRT run finishes with $LLR$ between $-2.944$ and $+2.944$, the status is marked as **CONTINUE (Inconclusive)**. We do not claim statistical superiority without reaching the decision boundary.
3. **Artifact Retention**: Every entry in this ledger must reference an exact, reproducible PGN file and git commit SHA.
