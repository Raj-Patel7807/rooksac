"""
research/tests/test_nn_pipeline.py

Unit and integration tests for supervised neural network training pipeline:
- Move vocabulary encoding and decoding (including promotions)
- FEN to tensor dimension and feature planes
- Model forward pass output dimensions and bounds
- Dataset splitting isolation and zero-leakage check
- Synthetic end-to-end training and checkpoint reload with metadata
"""

import json
import os
import tempfile
import unittest
import numpy as np
import torch

from research.nn.features import (
    VOCAB_SIZE,
    encode_move,
    decode_move,
    parse_uci_move,
    fen_to_tensor,
    square_to_index,
    index_to_square,
)
from research.nn.model import PolicyValueNet
from research.nn.dataset import ChessDataset
from research.nn.train import train_model
from tools.pgn_extract import parse_game, split_dataset, verify_no_split_leakage


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

    def test_parse_uci_move(self):
        from_sq, to_sq, promo = parse_uci_move("e7e8q")
        self.assertEqual(from_sq, "e7")
        self.assertEqual(to_sq, "e8")
        self.assertEqual(promo, "q")

        from_sq, to_sq, promo = parse_uci_move("e2e4")
        self.assertEqual(from_sq, "e2")
        self.assertEqual(to_sq, "e4")
        self.assertIsNone(promo)

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

    def test_dataset_split_isolation_and_no_leakage(self):
        games = [{"game_id": f"g_{i:03d}", "score": 1.0} for i in range(100)]
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
        self.assertTrue(verify_no_split_leakage(train, val, test))

    def test_model_forward_pass_and_checkpoint(self):
        model = PolicyValueNet(in_channels=18, num_blocks=1, channels=16)
        x = torch.randn(2, 18, 8, 8)
        policy_logits, value = model(x)

        # Check shapes
        self.assertEqual(policy_logits.shape, (2, 4096))
        self.assertEqual(value.shape, (2, 1))

        # Check value head bound in [-1, 1]
        self.assertTrue(torch.all(value >= -1.0) and torch.all(value <= 1.0))

    def test_end_to_end_training_and_metadata(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            save_path = os.path.join(tmpdir, "test_model.pt")
            meta_path = os.path.join(tmpdir, "test_meta.json")

            samples = [
                {"fen": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "move": ("e2", "e4"), "result": 1.0},
                {"fen": "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq - 0 1", "move": ("e7", "e5"), "result": 0.5},
                {"fen": "rnbqkbnr/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/RNBQKBNR w KQkq - 0 2", "move": ("g1", "f3"), "result": 1.0},
                {"fen": "r1bqkbnr/pppp1ppp/2n5/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 2 3", "move": ("f1", "b5"), "result": 0.0},
            ]

            meta = train_model(
                train_samples=samples[:2],
                val_samples=samples[2:],
                epochs=2,
                batch_size=2,
                num_blocks=1,
                channels=16,
                seed=123,
                save_path=save_path,
                metadata_path=meta_path,
            )

            self.assertTrue(os.path.exists(save_path))
            self.assertTrue(os.path.exists(meta_path))

            # Reload model weights and confirm integrity
            loaded = torch.load(save_path, weights_only=False)
            self.assertIn("model_state_dict", loaded)
            self.assertIn("metadata", loaded)
            self.assertEqual(loaded["metadata"]["architecture"], "PolicyValueNet-ResNet")


if __name__ == "__main__":
    unittest.main()
