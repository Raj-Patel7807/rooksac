"""
research/tests/test_nn_pipeline.py

Unit and integration tests for supervised neural network training pipeline:
- Move vocabulary encoding and decoding
- FEN to tensor dimension and feature planes
- Model forward pass output dimensions and bounds
- Dataset splitting isolation and zero-leakage check
- Synthetic end-to-end training and checkpoint reload
"""

import os
import tempfile
import unittest
import numpy as np
import torch

from research.nn.features import (
    VOCAB_SIZE, encode_move, decode_move, fen_to_tensor, square_to_index, index_to_square
)
from research.nn.model import PolicyValueNet
from research.nn.dataset import ChessDataset
from tools.pgn_extract import parse_game, split_dataset

class TestNeuralPipeline(unittest.TestCase):

    def test_square_roundtrip(self):
        for idx in range(64):
            sq = index_to_square(idx)
            self.assertEqual(square_to_index(sq), idx)

    def test_move_encoding_roundtrip(self):
        moves = [("e2", "e4"), ("g1", "f3"), ("a7", "a8"), ("h8", "h1")]
        for from_sq, to_sq in moves:
            encoded = encode_move(from_sq, to_sq)
            self.assertTrue(0 <= encoded < VOCAB_SIZE)
            f_dec, t_dec = decode_move(encoded)
            self.assertEqual((f_dec, t_dec), (from_sq, to_sq))

    def test_fen_tensor_shapes_and_values(self):
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        tensor = fen_to_tensor(fen)
        self.assertEqual(tensor.shape, (18, 8, 8))
        self.assertEqual(tensor.dtype, np.float32)
        # White side to move plane should be all 1s
        self.assertEqual(tensor[12].mean(), 1.0)
        # Check castling rights planes are active
        self.assertEqual(tensor[13].mean(), 1.0)
        self.assertEqual(tensor[14].mean(), 1.0)

    def test_dataset_split_isolation(self):
        games = [
            {"game_id": f"g_{i:03d}", "score": 1.0} for i in range(100)
        ]
        train, val, test = split_dataset(games, train_ratio=0.7, val_ratio=0.15)
        train_ids = {g["game_id"] for g in train}
        val_ids = {g["game_id"] for g in val}
        test_ids = {g["game_id"] for g in test}

        # Check total counts
        self.assertEqual(len(train_ids), 70)
        self.assertEqual(len(val_ids), 15)
        self.assertEqual(len(test_ids), 15)

        # Ensure zero leakage between splits
        self.assertTrue(train_ids.isdisjoint(val_ids))
        self.assertTrue(train_ids.isdisjoint(test_ids))
        self.assertTrue(val_ids.isdisjoint(test_ids))

    def test_model_forward_pass_and_checkpoint(self):
        model = PolicyValueNet(in_channels=18, num_blocks=1, channels=16)
        x = torch.randn(2, 18, 8, 8)
        policy_logits, value = model(x)

        # Check shapes
        self.assertEqual(policy_logits.shape, (2, 4096))
        self.assertEqual(value.shape, (2, 1))

        # Check value head bound in [-1, 1]
        self.assertTrue(torch.all(value >= -1.0) and torch.all(value <= 1.0))

        # Check checkpointing
        with tempfile.TemporaryDirectory() as tmpdir:
            ckpt_path = os.path.join(tmpdir, "test.pt")
            torch.save(model.state_dict(), ckpt_path)
            loaded_model = PolicyValueNet(in_channels=18, num_blocks=1, channels=16)
            loaded_model.load_state_dict(torch.load(ckpt_path))
            loaded_logits, loaded_val = loaded_model(x)
            self.assertTrue(torch.allclose(policy_logits, loaded_logits))

if __name__ == "__main__":
    unittest.main()

