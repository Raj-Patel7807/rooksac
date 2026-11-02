#!/usr/bin/env python3
"""
tools/pgn_extract.py

Ingests PGN files, filters games, and extracts quiet positions with game outcomes.
Ensures whole-game train/val/test splitting to prevent data leakage.
"""

import argparse
import hashlib
import json
import os
import re
import sys
from typing import Dict, List, Optional, Tuple

def parse_game(pgn_text: str) -> Optional[Dict]:
    """Extracts headers and moves from a single PGN text block."""
    headers = {}
    header_pattern = re.compile(r'\[(\w+)\s+"([^"]*)"\]')
    for match in header_pattern.finditer(pgn_text):
        headers[match.group(1)] = match.group(2)

    result = headers.get("Result", "*")
    if result not in ["1-0", "0-1", "1/2-1/2"]:
        return None

    # Filter out non-standard chess variants
    variant = headers.get("Variant", "Standard")
    if variant.lower() not in ["standard", "chess"]:
        return None

    # Normalize result score
    score = 1.0 if result == "1-0" else (0.0 if result == "0-1" else 0.5)

    # Simple game ID based on players and date
    raw_id = f"{headers.get('White', '')}_{headers.get('Black', '')}_{headers.get('Date', '')}_{headers.get('Site', '')}"
    game_id = hashlib.sha256(raw_id.encode('utf-8')).hexdigest()[:16]

    return {
        "game_id": game_id,
        "score": score,
        "headers": headers,
        "raw_text": pgn_text
    }

def split_dataset(games: List[Dict], train_ratio=0.7, val_ratio=0.15, seed=42) -> Tuple[List[Dict], List[Dict], List[Dict]]:
    """Splits whole games into train, validation, and test subsets."""
    # Deterministic sorting by game_id to ensure reproducibility
    sorted_games = sorted(games, key=lambda g: g["game_id"])
    n = len(sorted_games)
    n_train = int(n * train_ratio)
    n_val = int(n * val_ratio)

    train = sorted_games[:n_train]
    val = sorted_games[n_train:n_train + n_val]
    test = sorted_games[n_train + n_val:]
    return train, val, test

def main():
    parser = argparse.ArgumentParser(description="Extract positions and split games for tuning.")
    parser.add_argument("--input", required=True, help="Input PGN file or directory")
    parser.add_argument("--output-dir", default="research/data", help="Output directory for manifests")
    parser.add_argument("--train-ratio", type=float, default=0.7)
    parser.add_argument("--val-ratio", type=float, default=0.15)
    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"Error: input file {args.input} does not exist", file=sys.stderr)
        sys.exit(1)

    os.makedirs(args.output_dir, exist_ok=True)
    print(f"Ingesting {args.input}...")

if __name__ == "__main__":
    main()
