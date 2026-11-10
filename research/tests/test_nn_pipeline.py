"""
research/tests/test_nn_pipeline.py

Unit and integration tests for supervised neural network training pipeline:
- Move vocabulary encoding and decoding (including distinct promotions)
- Malformed inputs rejection (invalid squares, UCI moves, indices)
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
    STANDARD_ACTIONS,
    WHITE_UNDERPROMO_START,
    BLACK_UNDERPROMO_START,
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

    def test_square_invalid_rejected(self):
        with self.assertRaises(ValueError):
            square_to_index("e9")
        with self.assertRaises(ValueError):
            square_to_index("i4")
        with self.assertRaises(ValueError):
            square_to_index("e")
        with self.assertRaises(ValueError):
            index_to_square(64)
        with self.assertRaises(ValueError):
            index_to_square(-1)

    def test_move_encoding_roundtrip(self):
        moves = [("e2", "e4"), ("g1", "f3"), ("a7", "a8"), ("h8", "h1")]
        for from_sq, to_sq in moves:
            encoded = encode_move(from_sq, to_sq)
            self.assertTrue(0 <= encoded < VOCAB_SIZE)
            f_dec, t_dec, promo = decode_move(encoded)
            self.assertEqual((f_dec, t_dec), (from_sq, to_sq))
            self.assertIsNone(promo)

    def test_distinct_promotion_actions(self):
        # White promotions
        q_idx = encode_move("e7", "e8", "q")
        r_idx = encode_move("e7", "e8", "r")
        b_idx = encode_move("e7", "e8", "b")
        n_idx = encode_move("e7", "e8", "n")

        # Distinct check
        indices = {q_idx, r_idx, b_idx, n_idx}
        self.assertEqual(len(indices), 4, "All 4 White promotion pieces must have distinct indices")
        self.assertTrue(q_idx < STANDARD_ACTIONS, "Queen promotion is mapped to standard from*64+to")
        self.assertTrue(r_idx >= WHITE_UNDERPROMO_START)
        self.assertTrue(b_idx >= WHITE_UNDERPROMO_START)
        self.assertTrue(n_idx >= WHITE_UNDERPROMO_START)

        # Decoding round-trip
        f_q, t_q, p_q = decode_move(q_idx)
        self.assertEqual((f_q, t_q), ("e7", "e8"))

        f_r, t_r, p_r = decode_move(r_idx)
        self.assertEqual((f_r, t_r, p_r), ("e7", "e8", "r"))

        f_b, t_b, p_b = decode_move(b_idx)
        self.assertEqual((f_b, t_b, p_b), ("e7", "e8", "b"))

        f_n, t_n, p_n = decode_move(n_idx)
        self.assertEqual((f_n, t_n, p_n), ("e7", "e8", "n"))

        # Black promotions
        bq_idx = encode_move("e2", "e1", "q")
        br_idx = encode_move("e2", "e1", "r")
        bb_idx = encode_move("e2", "e1", "b")
        bn_idx = encode_move("e2", "e1", "n")

        b_indices = {bq_idx, br_idx, bb_idx, bn_idx}
        self.assertEqual(len(b_indices), 4, "All 4 Black promotion pieces must have distinct indices")
        self.assertTrue(bq_idx < STANDARD_ACTIONS)
        self.assertTrue(br_idx >= BLACK_UNDERPROMO_START)
        self.assertTrue(bb_idx >= BLACK_UNDERPROMO_START)
        self.assertTrue(bn_idx >= BLACK_UNDERPROMO_START)

        bf_n, bt_n, bp_n = decode_move(bn_idx)
        self.assertEqual((bf_n, bt_n, bp_n), ("e2", "e1", "n"))

        # Black underpromotions must not overlap with White underpromotions
        self.assertTrue(indices.isdisjoint(b_indices))

    def test_promotion_captures(self):
        # White captures to d8 and f8
        left_n = encode_move("e7", "d8", "n")
        straight_n = encode_move("e7", "e8", "n")
        right_n = encode_move("e7", "f8", "n")

        self.assertEqual(len({left_n, straight_n, right_n}), 3)

        self.assertEqual(decode_move(left_n), ("e7", "d8", "n"))
        self.assertEqual(decode_move(straight_n), ("e7", "e8", "n"))
        self.assertEqual(decode_move(right_n), ("e7", "f8", "n"))

    def test_invalid_moves_and_promotions_rejected(self):
        # Same square
        with self.assertRaises(ValueError):
            encode_move("e4", "e4")
        # Invalid promo piece
        with self.assertRaises(ValueError):
            encode_move("e7", "e8", "k")
        # Underpromotion from non-promotion rank
        with self.assertRaises(ValueError):
            encode_move("e2", "e4", "n")
        # Underpromotion with file jump > 1
        with self.assertRaises(ValueError):
            encode_move("e7", "c8", "n")
        # Invalid action index to decode
        with self.assertRaises(ValueError):
            decode_move(-1)
        with self.assertRaises(ValueError):
            decode_move(VOCAB_SIZE)

    def test_parse_uci_move(self):
        from_sq, to_sq, promo = parse_uci_move("e7e8q")
        self.assertEqual(from_sq, "e7")
        self.assertEqual(to_sq, "e8")
        self.assertEqual(promo, "q")

        from_sq, to_sq, promo = parse_uci_move("e2e4")
        self.assertEqual(from_sq, "e2")
        self.assertEqual(to_sq, "e4")
        self.assertIsNone(promo)

        # Malformed UCI moves rejected
        with self.assertRaises(ValueError):
            parse_uci_move("e2")
        with self.assertRaises(ValueError):
            parse_uci_move("e2e4x")
        with self.assertRaises(ValueError):
            parse_uci_move("e2e9")

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
        model = PolicyValueNet(in_channels=18, num_blocks=1, channels=16, num_actions=VOCAB_SIZE)
        x = torch.randn(2, 18, 8, 8)
        policy_logits, value = model(x)

        # Check shapes
        self.assertEqual(policy_logits.shape, (2, VOCAB_SIZE))
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
            self.assertEqual(loaded["metadata"]["vocab_size"], VOCAB_SIZE)


if __name__ == "__main__":
    unittest.main()
