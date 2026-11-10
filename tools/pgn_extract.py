#!/usr/bin/env python3
"""
tools/pgn_extract.py

Streaming PGN ingestion, validation, deduplication, whole-game splitting,
and per-position sample extraction for RookSac policy-value neural network training.

Features:
- Streaming generator parser with bounded memory (safe for multi-GB PGNs).
- Full legal move validation using python-chess during extraction.
- Rejection of non-standard variants, malformed moves, unknown results, invalid FENs.
- Configurable quality filters: Elo ratings, min/max ply, opening plies exclusion.
- Content-hash based deduplication (hashing normalized move sequence and result).
- Deterministic whole-game partitioning (train/val/test) preventing position leakage.
- Compressed sharded JSONL output (.jsonl.gz) to bound disk and memory footprint.
- Emits dataset manifest (metadata, checksums, counts) and data quality report.
"""

import argparse
import gzip
import hashlib
import json
import os
import sys
from datetime import datetime, timezone
from typing import Dict, Generator, List, Optional, Set, Tuple

import chess
import chess.pgn

SCHEMA_VERSION = "2.0.0"


def parse_game(pgn_text: str) -> Optional[Dict]:
    """Compatibility helper to parse a raw PGN text string and return game details."""
    import io
    game = chess.pgn.read_game(io.StringIO(pgn_text))
    if not game:
        return None
    record, reason = process_single_game(game, GameFilter(min_ply=1, skip_opening_ply=0))
    if not record:
        return None
    return {
        "game_id": record["game_id"],
        "score": record["score"],
        "result_str": record["result_str"],
        "raw_text": pgn_text,
    }


def split_dataset(
    games: List[Dict], train_ratio: float = 0.7, val_ratio: float = 0.15, seed: int = 42
) -> Tuple[List[Dict], List[Dict], List[Dict]]:
    """Compatibility alias for split_games."""
    test_ratio = max(0.0, 1.0 - (train_ratio + val_ratio))
    return split_games(games, train_ratio=train_ratio, val_ratio=val_ratio, test_ratio=test_ratio, seed=seed)


def verify_no_split_leakage(train: List[Dict], val: List[Dict], test: List[Dict]) -> bool:
    """Compatibility alias for verify_no_leakage."""
    return verify_no_leakage(train, val, test)
    """Computes a SHA-256 hash based strictly on normalized UCI moves and game result."""
    hasher = hashlib.sha256()
    hasher.update(result.encode("utf-8"))
    for move in moves:
        hasher.update(b" ")
        hasher.update(move.encode("utf-8"))
    return hasher.hexdigest()[:16]


def compute_content_hash(moves: List[str], result: str) -> str:
    """Computes a SHA-256 hash based strictly on normalized UCI moves and game result."""
    hasher = hashlib.sha256()
    hasher.update(result.encode("utf-8"))
    for move in moves:
        hasher.update(b" ")
        hasher.update(move.encode("utf-8"))
    return hasher.hexdigest()[:16]


def parse_numeric_header(headers: chess.pgn.Headers, key: str) -> Optional[int]:
    val = headers.get(key)
    if val is None or val == "?" or val == "":
        return None
    try:
        return int(val)
    except ValueError:
        return None


def stream_pgn_games(pgn_file_path: str) -> Generator[chess.pgn.Game, None, None]:
    """Streams games one at a time using python-chess without loading the full file."""
    if pgn_file_path.endswith(".gz"):
        handle = gzip.open(pgn_file_path, "rt", encoding="utf-8", errors="replace")
    else:
        handle = open(pgn_file_path, "r", encoding="utf-8", errors="replace")

    try:
        while True:
            game = chess.pgn.read_game(handle)
            if game is None:
                break
            yield game
    finally:
        handle.close()


class GameFilter:
    def __init__(
        self,
        min_rating: Optional[int] = None,
        min_ply: int = 10,
        max_ply: Optional[int] = None,
        skip_opening_ply: int = 4,
    ):
        self.min_rating = min_rating
        self.min_ply = min_ply
        self.max_ply = max_ply
        self.skip_opening_ply = skip_opening_ply


def process_single_game(
    game: chess.pgn.Game,
    filters: GameFilter,
) -> Tuple[Optional[Dict], Optional[str]]:
    """Validates game headers and replays mainline moves with python-chess.

    Returns: (processed_game_dict, rejection_reason)
    """
    # 0. Check for python-chess parse errors
    if game.errors:
        return None, "malformed_pgn_syntax"

    headers = game.headers
    # 1. Variant check
    variant = headers.get("Variant", "Standard")
    if variant.lower() not in ["standard", "chess"]:
        return None, "non_standard_variant"

    # 2. Result check
    result_str = headers.get("Result", "*")
    if result_str == "1-0":
        score = 1.0
    elif result_str == "0-1":
        score = 0.0
    elif result_str in ["1/2-1/2", "0.5-0.5", "1/2"]:
        score = 0.5
    else:
        return None, "unknown_result"

    # 3. Rating check
    white_elo = parse_numeric_header(headers, "WhiteElo")
    black_elo = parse_numeric_header(headers, "BlackElo")
    if filters.min_rating is not None:
        if white_elo is None or black_elo is None:
            return None, "missing_ratings"
        if white_elo < filters.min_rating or black_elo < filters.min_rating:
            return None, "below_min_rating"

    # 4. Replay moves and validate legality
    board = game.board()
    positions: List[Dict] = []
    uci_moves: List[str] = []

    ply_idx = 0
    for node in game.mainline():
        move = node.move
        if move not in board.legal_moves:
            return None, "illegal_move_in_mainline"

        fen_before = board.fen()
        stm = "w" if board.turn == chess.WHITE else "b"
        uci_move = move.uci()
        uci_moves.append(uci_move)

        # Apply opening ply exclusion
        if ply_idx >= filters.skip_opening_ply:
            positions.append({
                "fen": fen_before,
                "move": uci_move,
                "result": score,
                "stm": stm,
                "ply": ply_idx,
            })

        board.push(move)
        ply_idx += 1

    total_ply = len(uci_moves)
    if total_ply < filters.min_ply:
        return None, "too_few_ply"
    if filters.max_ply is not None and total_ply > filters.max_ply:
        return None, "too_many_ply"

    if not positions:
        return None, "no_positions_extracted"

    content_hash = compute_content_hash(uci_moves, result_str)

    game_record = {
        "game_id": content_hash,
        "result_str": result_str,
        "score": score,
        "total_ply": total_ply,
        "white": headers.get("White", "?"),
        "black": headers.get("Black", "?"),
        "white_elo": white_elo,
        "black_elo": black_elo,
        "date": headers.get("Date", "????.??.??"),
        "time_control": headers.get("TimeControl", "?"),
        "event": headers.get("Event", "?"),
        "positions": positions,
    }

    return game_record, None


def split_games(
    games: List[Dict],
    train_ratio: float = 0.8,
    val_ratio: float = 0.1,
    test_ratio: float = 0.1,
    seed: int = 42,
) -> Tuple[List[Dict], List[Dict], List[Dict]]:
    """Deterministically splits whole games by game_id hash with zero leakage."""
    assert abs((train_ratio + val_ratio + test_ratio) - 1.0) < 1e-5

    # Sort deterministically by game_id
    sorted_games = sorted(games, key=lambda g: g["game_id"])

    # PRNG shuffle with fixed seed
    import random
    rng = random.Random(seed)
    indices = list(range(len(sorted_games)))
    rng.shuffle(indices)

    shuffled_games = [sorted_games[i] for i in indices]

    n = len(shuffled_games)
    n_train = int(n * train_ratio)
    n_val = int(n * val_ratio)

    train = shuffled_games[:n_train]
    val = shuffled_games[n_train : n_train + n_val]
    test = shuffled_games[n_train + n_val :]

    return train, val, test


def verify_no_leakage(train: List[Dict], val: List[Dict], test: List[Dict]) -> bool:
    train_ids = {g["game_id"] for g in train}
    val_ids = {g["game_id"] for g in val}
    test_ids = {g["game_id"] for g in test}

    return (
        train_ids.isdisjoint(val_ids)
        and train_ids.isdisjoint(test_ids)
        and val_ids.isdisjoint(test_ids)
    )


def write_sharded_jsonl(
    games: List[Dict],
    split_name: str,
    output_dir: str,
    shard_size_positions: int = 50000,
) -> Tuple[List[str], int]:
    """Writes positions to compressed .jsonl.gz shards. Returns (shard_filenames, total_positions)."""
    shards_dir = os.path.join(output_dir, "shards")
    os.makedirs(shards_dir, exist_ok=True)

    shard_idx = 0
    current_shard_positions = 0
    total_positions = 0
    shard_files: List[str] = []

    current_file = None

    for game in games:
        gid = game["game_id"]
        for pos in game["positions"]:
            if current_file is None or current_shard_positions >= shard_size_positions:
                if current_file is not None:
                    current_file.close()
                shard_filename = f"{split_name}_shard_{shard_idx:03d}.jsonl.gz"
                shard_path = os.path.join(shards_dir, shard_filename)
                shard_files.append(shard_filename)
                current_file = gzip.open(shard_path, "wt", encoding="utf-8")
                shard_idx += 1
                current_shard_positions = 0

            record = {
                "game_id": gid,
                "fen": pos["fen"],
                "move": pos["move"],
                "result": pos["result"],
                "stm": pos["stm"],
                "ply": pos["ply"],
            }
            current_file.write(json.dumps(record) + "\n")
            current_shard_positions += 1
            total_positions += 1

    if current_file is not None:
        current_file.close()

    return shard_files, total_positions


def calculate_sha256(file_path: str) -> str:
    hasher = hashlib.sha256()
    with open(file_path, "rb") as f:
        while chunk := f.read(65536):
            hasher.update(chunk)
    return hasher.hexdigest()


def process_pgn(
    pgn_path: str,
    output_dir: str,
    source_name: str = "PGN Dataset",
    license_str: str = "Public Domain / CC0",
    train_ratio: float = 0.8,
    val_ratio: float = 0.1,
    test_ratio: float = 0.1,
    min_rating: Optional[int] = None,
    min_ply: int = 10,
    max_ply: Optional[int] = None,
    skip_opening_ply: int = 4,
    max_games: Optional[int] = None,
    seed: int = 42,
    shard_size_positions: int = 50000,
) -> Dict:
    """End-to-end dataset processing from PGN to validated shards and manifests."""
    os.makedirs(output_dir, exist_ok=True)
    filters = GameFilter(
        min_rating=min_rating,
        min_ply=min_ply,
        max_ply=max_ply,
        skip_opening_ply=skip_opening_ply,
    )

    seen_hashes: Set[str] = set()
    accepted_games: List[Dict] = []
    rejection_counts: Dict[str, int] = {}
    rating_buckets: Dict[str, int] = {
        "<1500": 0, "1500-1800": 0, "1800-2100": 0, "2100-2400": 0, "2400+": 0, "Unknown": 0
    }
    result_counts: Dict[str, int] = {"1-0": 0, "0-1": 0, "1/2-1/2": 0}

    total_parsed = 0
    duplicate_count = 0

    print(f"Streaming PGN: {pgn_path} ...")
    for game in stream_pgn_games(pgn_path):
        total_parsed += 1
        record, reason = process_single_game(game, filters)
        if reason:
            rejection_counts[reason] = rejection_counts.get(reason, 0) + 1
            continue

        gid = record["game_id"]
        if gid in seen_hashes:
            duplicate_count += 1
            rejection_counts["duplicate"] = rejection_counts.get("duplicate", 0) + 1
            continue

        seen_hashes.add(gid)
        accepted_games.append(record)

        # Track result distribution
        r_str = record["result_str"]
        result_counts[r_str] = result_counts.get(r_str, 0) + 1

        # Track rating bucket (average of players)
        w_elo = record["white_elo"]
        b_elo = record["black_elo"]
        if w_elo and b_elo:
            avg_elo = (w_elo + b_elo) // 2
            if avg_elo < 1500: rating_buckets["<1500"] += 1
            elif avg_elo < 1800: rating_buckets["1500-1800"] += 1
            elif avg_elo < 2100: rating_buckets["1800-2100"] += 1
            elif avg_elo < 2400: rating_buckets["2100-2400"] += 1
            else: rating_buckets["2400+"] += 1
        else:
            rating_buckets["Unknown"] += 1

        if max_games and len(accepted_games) >= max_games:
            break

    print(f"Accepted {len(accepted_games)} games from {total_parsed} parsed (duplicates: {duplicate_count}).")

    # Split games
    train_games, val_games, test_games = split_games(
        accepted_games,
        train_ratio=train_ratio,
        val_ratio=val_ratio,
        test_ratio=test_ratio,
        seed=seed,
    )
    assert verify_no_leakage(train_games, val_games, test_games), "Data leakage detected across splits!"

    # Write sharded positions
    splits_info = {}
    total_positions = 0
    for s_name, s_games in [("train", train_games), ("val", val_games), ("test", test_games)]:
        shards, p_count = write_sharded_jsonl(s_games, s_name, output_dir, shard_size_positions)
        total_positions += p_count
        splits_info[s_name] = {
            "game_count": len(s_games),
            "position_count": p_count,
            "shards": shards,
            "game_ids": [g["game_id"] for g in s_games],
        }

    # Write individual split manifests
    for s_name, s_games in [("train", train_games), ("val", val_games), ("test", test_games)]:
        manifest_path = os.path.join(output_dir, f"{s_name}_manifest.json")
        summary = [
            {
                "game_id": g["game_id"],
                "score": g["score"],
                "result": g["result_str"],
                "move_count": g["total_ply"],
                "extracted_positions": len(g["positions"]),
                "white": g["white"],
                "black": g["black"],
                "white_elo": g["white_elo"],
                "black_elo": g["black_elo"],
            }
            for g in s_games
        ]
        with open(manifest_path, "w", encoding="utf-8") as f:
            json.dump(summary, f, indent=2)

    # Compute checksums of all shard files
    shard_checksums = {}
    shards_dir = os.path.join(output_dir, "shards")
    for s_name, info in splits_info.items():
        for sf in info["shards"]:
            full_sf = os.path.join(shards_dir, sf)
            shard_checksums[sf] = calculate_sha256(full_sf)

    # Dataset Manifest
    dataset_manifest = {
        "schema_version": SCHEMA_VERSION,
        "source_name": source_name,
        "date_extracted": datetime.now(timezone.utc).isoformat(),
        "license": license_str,
        "seed": seed,
        "filters": {
            "min_rating": min_rating,
            "min_ply": min_ply,
            "max_ply": max_ply,
            "skip_opening_ply": skip_opening_ply,
            "train_ratio": train_ratio,
            "val_ratio": val_ratio,
            "test_ratio": test_ratio,
        },
        "totals": {
            "total_parsed_games": total_parsed,
            "total_valid_games": len(accepted_games),
            "total_positions": total_positions,
            "duplicate_games": duplicate_count,
        },
        "splits": {
            "train": {
                "game_count": splits_info["train"]["game_count"],
                "position_count": splits_info["train"]["position_count"],
                "shards": splits_info["train"]["shards"],
            },
            "val": {
                "game_count": splits_info["val"]["game_count"],
                "position_count": splits_info["val"]["position_count"],
                "shards": splits_info["val"]["shards"],
            },
            "test": {
                "game_count": splits_info["test"]["game_count"],
                "position_count": splits_info["test"]["position_count"],
                "shards": splits_info["test"]["shards"],
            },
        },
        "shard_checksums": shard_checksums,
    }

    manifest_file = os.path.join(output_dir, "dataset_manifest.json")
    with open(manifest_file, "w", encoding="utf-8") as f:
        json.dump(dataset_manifest, f, indent=2)

    # Data Quality Report
    quality_report = {
        "schema_version": SCHEMA_VERSION,
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "total_parsed_games": total_parsed,
        "accepted_games": len(accepted_games),
        "total_positions": total_positions,
        "duplicate_count": duplicate_count,
        "rejection_breakdown": rejection_counts,
        "results_distribution": result_counts,
        "rating_distribution": rating_buckets,
        "splits_summary": {
            k: {"games": v["game_count"], "positions": v["position_count"]}
            for k, v in splits_info.items()
        },
        "zero_leakage_verified": True,
    }

    report_file = os.path.join(output_dir, "data_quality_report.json")
    with open(report_file, "w", encoding="utf-8") as f:
        json.dump(quality_report, f, indent=2)

    print(f"Data pipeline complete. Output saved to {output_dir}")
    print(f"Positions: train={splits_info['train']['position_count']}, val={splits_info['val']['position_count']}, test={splits_info['test']['position_count']}")
    return dataset_manifest


def main():
    parser = argparse.ArgumentParser(description="Streaming PGN ingestion and position sharding.")
    parser.add_argument("--input", required=True, help="Input PGN file path (.pgn or .pgn.gz)")
    parser.add_argument("--output-dir", default="research/data", help="Output directory")
    parser.add_argument("--source-name", default="Lichess CC0 Standard Games", help="Dataset origin name")
    parser.add_argument("--min-rating", type=int, default=None, help="Filter out games where either player is below Elo")
    parser.add_argument("--min-ply", type=int, default=10, help="Minimum game ply")
    parser.add_argument("--max-ply", type=int, default=None, help="Maximum game ply")
    parser.add_argument("--skip-opening-ply", type=int, default=4, help="Exclude first N plies from position output")
    parser.add_argument("--train-ratio", type=float, default=0.8, help="Train split fraction")
    parser.add_argument("--val-ratio", type=float, default=0.1, help="Validation split fraction")
    parser.add_argument("--test-ratio", type=float, default=0.1, help="Test split fraction")
    parser.add_argument("--max-games", type=int, default=None, help="Maximum accepted games to process")
    parser.add_argument("--seed", type=int, default=42, help="Random seed for splitting")
    parser.add_argument("--shard-size", type=int, default=50000, help="Number of positions per compressed shard")
    args = parser.parse_args()

    if not os.path.exists(args.input):
        print(f"Error: input file {args.input} does not exist", file=sys.stderr)
        sys.exit(1)

    process_pgn(
        pgn_path=args.input,
        output_dir=args.output_dir,
        source_name=args.source_name,
        train_ratio=args.train_ratio,
        val_ratio=args.val_ratio,
        test_ratio=args.test_ratio,
        min_rating=args.min_rating,
        min_ply=args.min_ply,
        max_ply=args.max_ply,
        skip_opening_ply=args.skip_opening_ply,
        max_games=args.max_games,
        seed=args.seed,
        shard_size_positions=args.shard_size,
    )


if __name__ == "__main__":
    main()
