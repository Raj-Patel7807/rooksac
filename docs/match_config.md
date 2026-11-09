# Match Configuration and Reproducibility Protocol

## 1. Overview

This document specifies the standard match protocol, environment constraints, opening suites, and adjudication rules for measuring RookSac's playing strength in accordance with the Phase 1 & Sprint 06 requirements.

All engine-vs-engine evaluations must adhere to these controlled parameters. Comparing results across varying hardware, thread allocations, or hash sizes without explicit normalization is strictly prohibited.

---

## 2. Fixed Hardware & Environmental Controls

| Parameter                 | Standard Setting                                  | Rationale                                                      |
| :------------------------ | :------------------------------------------------ | :------------------------------------------------------------- |
| **OS**                    | Windows 11 / Linux x86-64                         | Operating system under test                                    |
| **Compiler**              | GCC 14.2.0 (`-O3 -DNDEBUG -std=c++20`)            | Default release toolchain                                      |
| **Concurrency / Threads** | 1 thread (`Threads = 1`)                          | Prevents search nondeterminism and CPU throttling interference |
| **Transposition Table**   | 16 MB (`Hash = 16`)                               | Uniform memory footprint across match runs                     |
| **Pondering**             | Off (`Ponder = false`)                            | Eliminates out-of-turn compute advantages                      |
| **Adjudication (Draw)**   | 50-move rule, threefold repetition, material draw | Standard FIDE rules strictly enforced                          |
| **Adjudication (Plies)**  | 200 plies cutoff                                  | Bound runtime on non-terminating endgames                      |

---

## 3. Opening Suite

Matches must use paired openings where every opening is played twice with swapped colors to eliminate first-move advantage bias.

- **Suite File**: `tools/openings.epd`
- **Count**: 20 distinct opening positions
- **Provenance**: Standard public domain theoretical lines (e.g. Italian, Sicilian, French, QGD, Nimzo-Indian, King's Indian).
- **SHA-256 Digest**:
    ```
    2ACAF3EC5DB72109B5B4D01D7E77FD7F5FF5826530139AE8B84A9F8B834E3372
    ```

---

## 4. Time Controls

| Category               | Time Control                                    | Use Case                                        |
| :--------------------- | :---------------------------------------------- | :---------------------------------------------- |
| **Fixed Depth**        | Depth 2 / 3 / 4                                 | Fast CI regression checks and algorithmic tests |
| **Fixed Move Time**    | `movetime 50` / `movetime 100`                  | Rapid internal sanity tests                     |
| **Blitz Time Control** | 3 min + 2s increment (`wtime 180000 winc 2000`) | Standard playing strength baseline              |
| **Rapid Time Control** | 10 min + 0s increment (`wtime 600000 winc 0`)   | High-confidence rating qualification            |

---

## 5. Sequential Probability Ratio Test (SPRT) Protocol

Candidate release versions are accepted or rejected using Wald's SPRT:

- **Null Hypothesis ($H_0$)**: $\Delta\text{Elo} = 0.0$ (Candidate is no better than baseline)
- **Alternative Hypothesis ($H_1$)**: $\Delta\text{Elo} = +5.0$ (Candidate is at least 5 Elo stronger)
- **Type I error rate ($\alpha$)**: $0.05$ (5% false positive acceptance)
- **Type II error rate ($\beta$)**: $0.05$ (5% false negative rejection)
- **Lower boundary**: $B = \ln(\beta / (1 - \alpha)) \approx -2.944$
- **Upper boundary**: $A = \ln((1 - \beta) / \alpha) \approx +2.944$

### Acceptance Rules:

1. **$LLR \ge A$**: Accept $H_1$. Candidate is accepted and merged.
2. **$LLR \le B$**: Accept $H_0$. Candidate is rejected.
3. **$B < LLR < A$**: Inconclusive. Match continues until boundary hit or game ceiling reached.
4. _Small-sample optimism is forbidden_: Do not claim rating gains from small sample size runs without hitting SPRT or achieving statistical significance (95% CI strictly positive).

---

## 6. Execution Commands

### Internal Match Harness (Built-in)

```powershell
# Run 20 paired games between Negamax and Greedy:
./build/rooksac_match_harness.exe --white negamax --black greedy --depth 2 --games 20 --openings tools/openings.epd --pgn docs/benchmarks/sprint06_internal.pgn

# Calculate SPRT and Elo CI:
python tools/sprt_match.py --pgn docs/benchmarks/sprint06_internal.pgn --elo0 0 --elo1 5
```

### Cute Chess CLI (External Engine Match)

```bash
cutechess-cli \
  -engine cmd=./build/rooksac.exe name=RookSac_Candidate \
  -engine cmd=./build/rooksac_baseline.exe name=RookSac_Baseline \
  -each proto=uci tc=3+2 hash=16 \
  -openings file=tools/openings.epd format=epd order=random \
  -repeat \
  -concurrency 1 \
  -sprt elo0=0 elo1=5 alpha=0.05 beta=0.05 \
  -pgnout matches.pgn
```

### Fastchess Integration

```bash
fastchess \
  -engine cmd=./build/rooksac.exe name=RookSac_Candidate \
  -engine cmd=./build/rooksac_baseline.exe name=RookSac_Baseline \
  -each tc=10/0.1 hash=16 \
  -openings file=tools/openings.epd format=epd order=random \
  -rounds 100 \
  -repeat \
  -concurrency 1 \
  -sprt elo0=0 elo1=5 alpha=0.05 beta=0.05 \
  -pgnout matches.pgn
```
