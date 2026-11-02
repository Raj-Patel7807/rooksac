"""
research/nn/dataset.py

PyTorch Dataset for position tensors, move policy targets, and game outcome values.
"""

import json
from typing import Dict, List
import numpy as np
import torch
from torch.utils.data import Dataset

from research.nn.features import fen_to_tensor, encode_move

class ChessDataset(Dataset):
    def __init__(self, samples: List[Dict]):
        """
        Each sample dict contains:
          - 'fen': str
          - 'move': (from_sq, to_sq) e.g. ('e2', 'e4')
          - 'result': float in [0.0, 1.0] (1.0 = White win, 0.5 = Draw, 0.0 = Black win)
        """
        self.samples = samples

    def __len__(self) -> int:
        return len(self.samples)

    def __getitem__(self, idx: int):
        item = self.samples[idx]
        x = fen_to_tensor(item['fen'])
        move_idx = encode_move(item['move'][0], item['move'][1])
        # Value target scaled to [-1, 1]: 1.0 -> 1.0, 0.5 -> 0.0, 0.0 -> -1.0
        val_target = (item['result'] - 0.5) * 2.0

        return (
            torch.from_numpy(x).float(),
            torch.tensor(move_idx, dtype=torch.long),
            torch.tensor([val_target], dtype=torch.float32)
        )

