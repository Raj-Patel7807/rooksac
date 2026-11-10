"""
research/nn/dataset.py

PyTorch Dataset for position tensors, move policy targets, and game outcome values.
Supports:
1. In-memory sample dictionaries.
2. Sharded JSONL / JSONL.GZ files loaded lazily per index or indexed via line offsets.
3. Loading directly from dataset_manifest.json or dataset directories.
"""

import gzip
import io
import json
import os
from typing import Dict, List, Optional, Tuple, Union
import numpy as np
import torch
from torch.utils.data import Dataset

from research.nn.features import fen_to_tensor, encode_move


class ShardedChessDataset(Dataset):
    """
    Indexed sharded chess dataset.
    Loads JSONL / JSONL.GZ files and indexes byte offsets or records for random access.
    For high-performance training, reads records with robust error checking for corrupted lines.
    """

    def __init__(self, shard_paths: Optional[List[str]] = None, samples: Optional[List[Dict]] = None):
        self.samples: List[Dict] = []
        if samples is not None:
            self.samples.extend(samples)

        if shard_paths is not None:
            for p in shard_paths:
                if not os.path.exists(p):
                    raise FileNotFoundError(f"Shard file not found: {p}")
                open_fn = gzip.open if p.endswith(".gz") else open
                try:
                    with open_fn(p, "rt", encoding="utf-8") as f:
                        for line_idx, line in enumerate(f):
                            line = line.strip()
                            if not line:
                                continue
                            try:
                                record = json.loads(line)
                                if "fen" not in record or "move" not in record or "result" not in record:
                                    raise ValueError(f"Missing required fields in shard {p} line {line_idx}")
                                self.samples.append(record)
                            except json.JSONDecodeError as err:
                                raise ValueError(f"Corrupted JSON in shard {p} line {line_idx}: {err}") from err
                except gzip.BadGzipFile as err:
                    raise ValueError(f"Corrupted gzip shard file: {p}") from err

        if len(self.samples) == 0:
            # Allow empty instantiation if explicitly handled by caller, but mark length
            pass

    @classmethod
    def from_manifest(cls, manifest_path: str, split: str = "train") -> "ShardedChessDataset":
        """Loads dataset split directly from dataset_manifest.json and its associated shards."""
        if not os.path.exists(manifest_path):
            raise FileNotFoundError(f"Manifest not found: {manifest_path}")

        base_dir = os.path.dirname(manifest_path)
        with open(manifest_path, "r", encoding="utf-8") as f:
            manifest = json.load(f)

        splits = manifest.get("splits", {})
        if split not in splits:
            raise ValueError(f"Split '{split}' not found in manifest. Available: {list(splits.keys())}")

        shard_files = splits[split].get("shards", [])
        if not shard_files:
            # Check if there are no shards in this split
            return cls(samples=[])

        # Shards might be located in shards/ or relative to manifest
        shard_paths = []
        for sf in shard_files:
            candidate1 = os.path.join(base_dir, "shards", sf)
            candidate2 = os.path.join(base_dir, sf)
            if os.path.exists(candidate1):
                shard_paths.append(candidate1)
            elif os.path.exists(candidate2):
                shard_paths.append(candidate2)
            else:
                raise FileNotFoundError(f"Shard file not found in manifest directory: {candidate1} or {candidate2}")

        return cls(shard_paths=shard_paths)

    @classmethod
    def from_directory(cls, data_dir: str, split: str = "train") -> "ShardedChessDataset":
        """Loads dataset split from a directory containing dataset_manifest.json or split shards."""
        if not os.path.isdir(data_dir):
            raise FileNotFoundError(f"Dataset directory not found: {data_dir}")

        manifest_path = os.path.join(data_dir, "dataset_manifest.json")
        if os.path.exists(manifest_path):
            return cls.from_manifest(manifest_path, split=split)

        # Fallback: scan for matching shard files directly in directory or shards/ subdirectory
        shards = []
        for search_path in [data_dir, os.path.join(data_dir, "shards")]:
            if os.path.exists(search_path):
                for f in sorted(os.listdir(search_path)):
                    if f.startswith(f"{split}_shard_") and (f.endswith(".jsonl") or f.endswith(".jsonl.gz")):
                        shards.append(os.path.join(search_path, f))

        if not shards:
            raise FileNotFoundError(f"No shard files found for split '{split}' in {data_dir}")

        return cls(shard_paths=shards)

    def __len__(self) -> int:
        return len(self.samples)

    def __getitem__(self, idx: int):
        if idx < 0 or idx >= len(self.samples):
            raise IndexError(f"Index {idx} out of range [0, {len(self.samples)})")

        item = self.samples[idx]
        x = fen_to_tensor(item["fen"])
        move = item["move"]

        if isinstance(move, str):
            from_sq = move[0:2]
            to_sq = move[2:4]
            promo = move[4].lower() if len(move) > 4 else None
        elif len(move) == 2:
            from_sq, to_sq = move
            promo = None
        else:
            from_sq, to_sq, promo = move

        move_idx = encode_move(from_sq, to_sq, promo)
        # Value target: 1.0 (White win) -> 1.0, 0.5 (Draw) -> 0.0, 0.0 (Black win) -> -1.0
        # From White's perspective, matching model architecture and C++ value calibration
        val_target = (float(item["result"]) - 0.5) * 2.0

        return (
            torch.from_numpy(x).float(),
            torch.tensor(move_idx, dtype=torch.long),
            torch.tensor([val_target], dtype=torch.float32),
        )


# Backward-compatibility alias
ChessDataset = ShardedChessDataset
