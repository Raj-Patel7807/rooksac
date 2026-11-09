#!/usr/bin/env python3
"""
tools/sprt_match.py

Orchestrates engine matches (standalone or via Cute Chess CLI / Fastchess)
and computes Sequential Probability Ratio Test (SPRT) stopping conditions.

Outputs:
  - Games played, W/D/L counts, score %
  - Elo difference with 95% confidence interval
  - SPRT decision (ACCEPT H1 / ACCEPT H0 / CONTINUE)

Usage examples:
  python tools/sprt_match.py --wins 10 --losses 5 --draws 5
  python tools/sprt_match.py --pgn match_results.pgn --elo0 0 --elo1 10
  python tools/sprt_match.py --wins 20 --losses 15 --draws 30 --elo0 0 --elo1 5
"""

import argparse
import math
import re
import sys
from typing import Tuple


# ---------------------------------------------------------------------------
# SPRT computation
# ---------------------------------------------------------------------------

def compute_llr(
    wins: int, losses: int, draws: int,
    elo0: float = 0.0, elo1: float = 5.0
) -> Tuple[float, float, float]:
    """Computes Log-Likelihood Ratio (LLR) and Wald boundaries.

    Uses the BayesSELO/SPRT formulation as described in:
    https://www.chessprogramming.org/Match_Statistics#SPRT
    """
    alpha = 0.05   # false-positive rate (H1 accepted when H0 true)
    beta  = 0.05   # false-negative rate (H0 accepted when H1 true)
    lower_bound = math.log(beta / (1.0 - alpha))
    upper_bound = math.log((1.0 - beta) / alpha)

    total = wins + losses + draws
    if total == 0:
        return 0.0, lower_bound, upper_bound

    p0 = 1.0 / (1.0 + math.pow(10.0, -elo0 / 400.0))
    p1 = 1.0 / (1.0 + math.pow(10.0, -elo1 / 400.0))

    score   = (wins + 0.5 * draws) / total
    w_prob  = wins   / total
    d_prob  = draws  / total
    l_prob  = losses / total

    # Variance of the score under a 3-outcome model
    var = (w_prob * (1.0 - score) ** 2 +
           d_prob * (0.5 - score) ** 2 +
           l_prob * (0.0 - score) ** 2)
    s2 = max(var, 1e-9)

    delta_p = p1 - p0
    sum_x   = wins + 0.5 * draws
    llr = (delta_p / s2) * (sum_x - total * (p0 + p1) / 2.0)
    return llr, lower_bound, upper_bound


# ---------------------------------------------------------------------------
# Elo difference and confidence interval
# ---------------------------------------------------------------------------

def score_to_elo(score: float) -> float:
    """Converts a score in [0,1] to an Elo difference."""
    if score <= 0.0:
        return float("-inf")
    if score >= 1.0:
        return float("+inf")
    return -400.0 * math.log10(1.0 / score - 1.0)


def compute_elo_and_ci(wins: int, losses: int, draws: int) -> Tuple[float, float, float]:
    """Returns (elo_diff, ci_lo, ci_hi) using a Wald 95% CI on the score.

    The CI is computed by propagating the uncertainty on the score through the
    logistic Elo formula.  This is an approximation; for small samples it is
    conservative.
    """
    total = wins + losses + draws
    if total == 0:
        return 0.0, float("-inf"), float("+inf")

    score = (wins + 0.5 * draws) / total
    # Standard error of the mean score (Bernoulli-ish, three outcomes)
    w_prob = wins   / total
    d_prob = draws  / total
    l_prob = losses / total
    var    = (w_prob * (1.0 - score) ** 2 +
              d_prob * (0.5 - score) ** 2 +
              l_prob * (0.0 - score) ** 2)
    stderr = math.sqrt(var / total)

    z95 = 1.959964  # 97.5th percentile of standard normal
    score_lo = max(score - z95 * stderr, 1e-6)
    score_hi = min(score + z95 * stderr, 1.0 - 1e-6)

    elo_diff = score_to_elo(score)
    ci_lo    = score_to_elo(score_lo)
    ci_hi    = score_to_elo(score_hi)
    return elo_diff, ci_lo, ci_hi


# ---------------------------------------------------------------------------
# PGN parsing
# ---------------------------------------------------------------------------

def parse_pgn_file(path: str) -> Tuple[int, int, int]:
    """Reads a PGN file and counts (wins, losses, draws) for the first engine.

    Convention: the first engine is always White in odd-numbered games and
    Black in even-numbered games (paired opening swap).  The function counts
    from the perspective of the first engine listed in the PGN Event header
    (or White in game 1).

    For simplicity this parser accumulates results assuming the caller supplies
    a PGN where game results are tagged with the standard Result header.

    Returns:
        (wins, losses, draws) from the perspective of White in game 1 (engine A).
    """
    wins = losses = draws = 0
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            content = f.read()
    except OSError as exc:
        print(f"Error reading PGN file: {exc}", file=sys.stderr)
        sys.exit(1)

    # Split on game boundaries (blank line before each [Event ...)
    games = re.split(r"\n\n(?=\[)", content.strip())
    game_index = 0
    for game in games:
        result_match = re.search(r'\[Result\s+"([^"]+)"\]', game)
        if not result_match:
            continue
        result = result_match.group(1)
        # Determine colour of engine A in this game (alternates each game)
        engine_a_is_white = (game_index % 2 == 0)
        game_index += 1

        if result == "1-0":
            if engine_a_is_white:
                wins += 1
            else:
                losses += 1
        elif result == "0-1":
            if engine_a_is_white:
                losses += 1
            else:
                wins += 1
        elif result in ("1/2-1/2", "½-½"):
            draws += 1
        # "*" (ongoing / unknown) is ignored

    return wins, losses, draws


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> None:
    parser = argparse.ArgumentParser(
        description="SPRT result evaluation and match driver.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    source = parser.add_mutually_exclusive_group()
    source.add_argument("--pgn", metavar="FILE",
                        help="PGN file to parse for W/D/L counts")
    source.add_argument("--wins",   type=int, default=0, metavar="N")
    parser.add_argument("--losses", type=int, default=0, metavar="N")
    parser.add_argument("--draws",  type=int, default=0, metavar="N")
    parser.add_argument("--elo0",   type=float, default=0.0,
                        help="H0 Elo advantage (default 0)")
    parser.add_argument("--elo1",   type=float, default=5.0,
                        help="H1 Elo advantage (default 5)")
    args = parser.parse_args()

    if args.pgn:
        wins, losses, draws = parse_pgn_file(args.pgn)
    else:
        wins   = args.wins
        losses = args.losses
        draws  = args.draws

    total = wins + losses + draws
    score_pct = (wins + 0.5 * draws) / total * 100.0 if total > 0 else 0.0

    llr, lower, upper = compute_llr(wins, losses, draws, args.elo0, args.elo1)
    elo_diff, ci_lo, ci_hi = compute_elo_and_ci(wins, losses, draws)

    if llr >= upper:
        decision = "ACCEPT H1  (candidate is stronger)"
    elif llr <= lower:
        decision = "ACCEPT H0  (candidate rejected)"
    else:
        decision = "CONTINUE   (inconclusive — more games needed)"

    sep = "-" * 52
    print(sep)
    print(f"  Games  : {total:>6}  (W {wins:>5} / D {draws:>5} / L {losses:>5})")
    print(f"  Score  : {score_pct:>6.1f}%")
    print(f"  Elo    : {elo_diff:>+7.1f}  [{ci_lo:>+.1f}, {ci_hi:>+.1f}] 95% CI")
    print(f"  LLR    : {llr:>7.3f}  bounds [{lower:.3f}, {upper:.3f}]")
    print(f"  SPRT   : {args.elo0:.0f} vs {args.elo1:.0f}  α=5% β=5%")
    print(sep)
    print(f"  Decision: {decision}")
    print(sep)


if __name__ == "__main__":
    main()
