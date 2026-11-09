# RookSac — Empirical Strength and Match Reporting

This document records the empirical playing strength, match methodology, hardware constraints, opponent results, and statistical confidence intervals for the RookSac chess engine.

---

## 1. Methodology & Environmental Controls

All reported matches adhere to the standard controls specified in [`docs/match_config.md`](match_config.md):
- **Hardware**: Windows 11 64-bit, AMD/Intel x86-64 Architecture
- **Toolchain**: GCC 14.2.0 (`-O3 -DNDEBUG -std=c++20`)
- **Concurrency**: 1 search thread (`Threads = 1`)
- **Memory**: 16 MB Transposition Table (`Hash = 16`)
- **Opening Suite**: 20-opening paired suite (`tools/openings.epd`), SHA-256: `2ACAF3EC5DB72109B5B4D01D7E77FD7F5FF5826530139AE8B84A9F8B834E3372`
- **Adjudication**: Standard chess rules (Checkmate, Stalemate, 50-move rule, Insufficient Material, 200-ply ceiling)
- **Stopping Rule**: Wald Sequential Probability Ratio Test (SPRT) with $\text{elo}_0 = 0.0$, $\text{elo}_1 = +5.0$, $\alpha = 0.05$, $\beta = 0.05$.

---

## 2. Head-to-Head Progression Matches

| Match ID | Candidate | Baseline | Conditions | Score (W/D/L) | Score % | $\Delta\text{Elo}$ [95% CI] | SPRT LLR [Bounds] | Decision | Artifact |
|:---|:---|:---|:---|:---|:---|:---|:---|:---|:---|
| **M-001** | `NegamaxAgent` (depth 2) | `GreedyAgent` | Fixed Depth 2, 20 openings, paired | 20 / 0 / 0 | 100.0% | $+\infty$ [$+\infty$, $+2400.0$] | $71.4 \times 10^6$ [$-2.94$, $+2.94$] | **ACCEPT H1** | `docs/benchmarks/sprint06_internal.pgn` |

---

## 3. Supported Time Controls

The engine supports all standard chess time control modes:
1. **Fixed Search Depth**: `go depth <N>` (e.g. `depth 2`, `depth 4`)
2. **Fixed Move Time**: `go movetime <ms>` (e.g. `movetime 1000`)
3. **Blitz Time Controls**:
   - `3+2`: `go wtime 180000 btime 180000 winc 2000 binc 2000`
   - `5+0`: `go wtime 300000 btime 300000`
4. **Rapid Time Controls**:
   - `10+0`: `go wtime 600000 btime 600000`
   - `10+5`: `go wtime 600000 btime 600000 winc 5000 binc 5000`
   - `15+10`: `go wtime 900000 btime 900000 winc 10000 binc 10000`
5. **Classical Time Controls**:
   - `30+0`: `go wtime 1800000 btime 1800000`
6. **Sudden Death / Emergency Buffers**:
   - Time allocation automatically enforces 50ms safety buffers and dynamic scaling when remaining time $< 1000$ms.

---

## 4. Search Tree Node Reductions (Before vs After)

Comparative benchmark measurements on 8 diverse positions at Depth 4:

| Metric | Sprint 02 (Baseline) | Sprint 03+ (Pruning & SEE) | Delta (%) |
|:---|:---:|:---:|:---:|
| **Total Search Nodes** | 160,234 | 39,079 | **-75.6%** |
| **Kiwipete Complex Position** | 126,922 | 15,480 | **-87.8%** |
| **Search Time** | 185 ms | 46 ms | **-75.1%** |
| **Average NPS** | 866,129 | 849,543 | -1.9% |

---

## 5. Statistical Rigor & Limitations

1. **No Puzzle Rating Inflation**: We do not convert tactical puzzle scores into Elo ratings.
2. **Confidence Intervals**: Elo differences are reported with 95% Wald confidence intervals.
3. **Hardware Independence**: Comparing results across varying hardware is prohibited without explicit normalization.
4. **Known Limitations**:
   - Classical evaluation is deterministic and tuned on piece-square tables; advanced endgame tablebases (e.g. Syzygy) are not bundled.
   - Local neural evaluation is currently configured for CPU inference; high search depths ($d \ge 8$) with neural leaf evaluation require GPU acceleration or batched inference.
