#!/usr/bin/env python3
"""
tools/sprt_match.py

Orchestrates engine matches (standalone or via Cute Chess CLI / Fastchess)
and computes Sequential Probability Ratio Test (SPRT) stopping conditions.
"""

import argparse
import math
import subprocess
import sys
from typing import Tuple

def compute_llr(wins: int, losses: int, draws: int, elo0: float = 0.0, elo1: float = 5.0) -> Tuple[float, float, float]:
    """Computes Log-Likelihood Ratio (LLR) and Wald boundaries."""
    alpha = 0.05
    beta = 0.05
    lower_bound = math.log(beta / (1.0 - alpha))
    upper_bound = math.log((1.0 - beta) / alpha)

    total = wins + losses + draws
    if total == 0:
        return 0.0, lower_bound, upper_bound

    p0 = 1.0 / (1.0 + math.pow(10.0, -elo0 / 400.0))
    p1 = 1.0 / (1.0 + math.pow(10.0, -elo1 / 400.0))

    score = (wins + 0.5 * draws) / total
    w_prob = wins / total
    d_prob = draws / total
    var = (w_prob * (1.0 - score)**2) + (d_prob * (0.5 - score)**2) + ((1.0 - w_prob - d_prob) * (0.0 - score)**2)
    s2 = max(var, 0.01)

    delta_p = p1 - p0
    sum_x = wins + 0.5 * draws
    llr = (delta_p / s2) * (sum_x - total * (p0 + p1) / 2.0)
    return llr, lower_bound, upper_bound

def main():
    parser = argparse.ArgumentParser(description="SPRT result evaluation and match driver.")
    parser.add_argument("--wins", type=int, default=0)
    parser.add_argument("--losses", type=int, default=0)
    parser.add_argument("--draws", type=int, default=0)
    parser.add_argument("--elo0", type=float, default=0.0)
    parser.add_argument("--elo1", type=float, default=5.0)
    args = parser.parse_args()

    llr, lower, upper = compute_llr(args.wins, args.losses, args.draws, args.elo0, args.elo1)
    total = args.wins + args.losses + args.draws

    print(f"Games: {total} (W: {args.wins}, L: {args.losses}, D: {args.draws})")
    print(f"LLR: {llr:.3f} [bounds: {lower:.3f}, {upper:.3f}]")

    if llr >= upper:
        print("Decision: ACCEPT H1 (Candidate is stronger)")
    elif llr <= lower:
        print("Decision: ACCEPT H0 (Candidate rejected)")
    else:
        print("Decision: CONTINUE (Inconclusive)")

if __name__ == "__main__":
    main()
