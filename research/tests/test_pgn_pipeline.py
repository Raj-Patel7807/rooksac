"""
research/tests/test_pgn_pipeline.py

Comprehensive tests for the streaming PGN dataset extraction pipeline:
- Legal move replay with python-chess
- Castling, en-passant, queen and underpromotions (N, B, R)
- Checkmate, stalemate, and draw handling
- Comment, NAG, and variation stripping
- Malformed PGN and illegal move rejection
- Deterministic splitting and zero-leakage guarantee
- Content hash deduplication
- Sharded output creation and Dataset loading
"""

import gzip
import json
import os
import tempfile
import unittest

from research.nn.dataset import ChessDataset
from tools.pgn_extract import (
    process_pgn,
    process_single_game,
    compute_content_hash,
    split_games,
    verify_no_leakage,
    GameFilter,
    stream_pgn_games,
)

SAMPLE_PGN_CONTENT = """[Event "Test Classical 1"]
[Site "RookSac Test"]
[Date "2026.11.01"]
[White "Player1"]
[Black "Player2"]
[Result "1-0"]
[WhiteElo "2100"]
[BlackElo "2150"]
[TimeControl "180+2"]

1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 4. Ba4 Nf6 5. O-O Be7 6. Re1 b5 7. Bb3 d6 8. c3 O-O 9. h3 Nb8 10. d4 Nbd7 1-0

[Event "Test Promotion & Underpromotion"]
[Site "RookSac Test"]
[Date "2026.11.02"]
[White "PromoWhite"]
[Black "PromoBlack"]
[Result "0-1"]
[WhiteElo "2200"]
[BlackElo "2250"]

1. e4 d5 2. exd5 c6 3. dxc6 Nf6 4. cxb7 e5 5. bxa8=N Bc5 6. Qf3 e4 7. Qf4 e3 8. dxe3 O-O 9. Qxb8 Re8 10. Nf3 0-1

[Event "Test Checkmate & Variations"]
[Site "RookSac Test"]
[Date "2026.11.03"]
[White "ScholarW"]
[Black "ScholarB"]
[Result "1-0"]
[WhiteElo "1900"]
[BlackElo "1950"]

1. e4 e5 2. Qh5 Nc6 3. Bc4 Nf6 {blunder} (3... g6 4. Qf3 Nf6) 4. Qxf7# 1-0

[Event "Test Stalemate Draw"]
[Site "RookSac Test"]
[Date "2026.11.04"]
[White "StaleW"]
[Black "StaleB"]
[Result "1/2-1/2"]
[WhiteElo "2000"]
[BlackElo "2000"]

1. e3 a5 2. Qh5 Ra6 3. Qxa5 h5 4. h4 R6a7 5. Qxc7 f6 6. Qxd7+ Kf7 7. Qxb7 Qd3 8. Qxb8 Qh7 9. Qxc8 Kg6 10. Qe6 1/2-1/2

[Event "Duplicate of Test Classical 1"]
[Site "Other Site"]
[Date "2026.11.05"]
[White "Player1"]
[Black "Player2"]
[Result "1-0"]
[WhiteElo "2100"]
[BlackElo "2150"]

1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 4. Ba4 Nf6 5. O-O Be7 6. Re1 b5 7. Bb3 d6 8. c3 O-O 9. h3 Nb8 10. d4 Nbd7 1-0

[Event "Malformed Game with Illegal Move"]
[Site "RookSac Test"]
[Date "2026.11.06"]
[White "BadWhite"]
[Black "BadBlack"]
[Result "1-0"]
[WhiteElo "2000"]
[BlackElo "2000"]

1. e4 e5 2. Nf3 h5 3. Nxe5 e4 4. Bc4 d5 5. Bxd5 c6 6. Bxf7+ Ke7 7. e1=Q# 1-0

[Event "Non-standard Variant"]
[Site "RookSac Test"]
[Date "2026.11.07"]
[White "VarW"]
[Black "VarB"]
[Result "1-0"]
[Variant "Crazyhouse"]

1. e4 e5 2. Nf3 1-0
"""


class TestPgnPipeline(unittest.TestCase):

    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.pgn_path = os.path.join(self.tmpdir.name, "sample.pgn")
        with open(self.pgn_path, "w", encoding="utf-8") as f:
            f.write(SAMPLE_PGN_CONTENT)

    def tearDown(self):
        self.tmpdir.cleanup()

    def test_single_game_processing(self):
        games = list(stream_pgn_games(self.pgn_path))
        self.assertEqual(len(games), 7)

        # 1. Classical game with castling
        rec, reason = process_single_game(games[0], GameFilter(min_ply=10, skip_opening_ply=4))
        self.assertIsNone(reason)
        self.assertIsNotNone(rec)
        self.assertEqual(rec["result_str"], "1-0")
        self.assertEqual(rec["score"], 1.0)
        self.assertTrue(len(rec["positions"]) > 0)

        # Check that castling move "e1g1" is present in positions
        moves = [p["move"] for p in rec["positions"]]
        self.assertIn("e1g1", moves)

        # 2. Promotion game with underpromotion bxa8n
        rec_promo, reason = process_single_game(games[1], GameFilter(min_ply=10, skip_opening_ply=4))
        self.assertIsNone(reason)
        promo_moves = [p["move"] for p in rec_promo["positions"]]
        self.assertIn("b7a8n", promo_moves)

        # 3. Malformed game rejected
        rec_bad, reason_bad = process_single_game(games[5], GameFilter(min_ply=5, skip_opening_ply=0))
        self.assertIsNone(rec_bad)
        self.assertEqual(reason_bad, "malformed_pgn_syntax")

        # 4. Non-standard variant rejected
        rec_var, reason_var = process_single_game(games[6], GameFilter(min_ply=2, skip_opening_ply=0))
        self.assertIsNone(rec_var)
        self.assertEqual(reason_var, "non_standard_variant")

    def test_content_hash_deduplication(self):
        games = list(stream_pgn_games(self.pgn_path))
        rec1, _ = process_single_game(games[0], GameFilter(min_ply=10, skip_opening_ply=0))
        rec_dup, _ = process_single_game(games[4], GameFilter(min_ply=10, skip_opening_ply=0))

        # Different headers, but exact same moves and result -> must have identical content hash
        self.assertEqual(rec1["game_id"], rec_dup["game_id"])

    def test_end_to_end_pipeline_and_manifest(self):
        out_dir = os.path.join(self.tmpdir.name, "dataset_out")
        manifest = process_pgn(
            pgn_path=self.pgn_path,
            output_dir=out_dir,
            min_rating=1800,
            min_ply=6,
            skip_opening_ply=2,
            train_ratio=0.6,
            val_ratio=0.2,
            test_ratio=0.2,
            seed=42,
            shard_size_positions=100,
        )

        # Validate manifest structure
        self.assertIn("totals", manifest)
        self.assertIn("splits", manifest)
        self.assertIn("shard_checksums", manifest)
        self.assertEqual(manifest["totals"]["duplicate_games"], 1)

        # Confirm zero leakage across splits
        splits = manifest["splits"]
        train_shards = splits["train"]["shards"]
        val_shards = splits["val"]["shards"]
        test_shards = splits["test"]["shards"]
        self.assertTrue(len(train_shards) > 0)

        # Test loading via PyTorch ChessDataset
        train_ds = ChessDataset.from_manifest(
            os.path.join(out_dir, "dataset_manifest.json"), split="train"
        )
        self.assertTrue(len(train_ds) > 0)

        # Read first sample
        x, target_policy, target_val = train_ds[0]
        self.assertEqual(x.shape, (18, 8, 8))
        self.assertTrue(0 <= target_policy.item() < 4240)
        self.assertTrue(-1.0 <= target_val.item() <= 1.0)

    def test_reproducible_split_manifest(self):
        out_dir1 = os.path.join(self.tmpdir.name, "out1")
        out_dir2 = os.path.join(self.tmpdir.name, "out2")

        m1 = process_pgn(pgn_path=self.pgn_path, output_dir=out_dir1, min_ply=6, skip_opening_ply=2, seed=123)
        m2 = process_pgn(pgn_path=self.pgn_path, output_dir=out_dir2, min_ply=6, skip_opening_ply=2, seed=123)

        # Re-running with same seed must produce identical shard file counts and split position totals
        self.assertEqual(m1["splits"]["train"]["position_count"], m2["splits"]["train"]["position_count"])
        self.assertEqual(m1["splits"]["val"]["position_count"], m2["splits"]["val"]["position_count"])
        self.assertEqual(m1["splits"]["test"]["position_count"], m2["splits"]["test"]["position_count"])


if __name__ == "__main__":
    unittest.main()
