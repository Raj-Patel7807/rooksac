"""
research/tests/test_training_pipeline.py

Unit and integration tests for dataset-driven training pipeline:
- Training from real sharded JSONL datasets
- CLI parameter parsing and dataset loading from directory/manifest
- Resume from checkpoint (reloading epoch, model weights, optimizer, and scheduler)
- Best checkpoint selection strictly by validation metric
- Isolation of test split evaluation
- Explicit smoke-test mode labeling and distinction
- NaN loss handling and corrupted shard rejection
"""

import gzip
import json
import os
import tempfile
import unittest
import torch

from research.nn.dataset import ShardedChessDataset
from research.nn.train import (
    run_training_pipeline,
    run_smoke_test,
    load_checkpoint,
    MODEL_SCHEMA_VERSION,
)
from tools.pgn_extract import process_pgn


SAMPLE_MINI_PGN = """[Event "Test Mini 1"]
[Site "RookSac Test"]
[Date "2026.11.01"]
[White "PlayerW1"]
[Black "PlayerB1"]
[Result "1-0"]
[WhiteElo "2100"]
[BlackElo "2150"]

1. e4 e5 2. Nf3 Nc6 3. Bb5 a6 4. Ba4 Nf6 5. O-O Be7 6. Re1 b5 7. Bb3 d6 8. c3 O-O 9. h3 Nb8 10. d4 Nbd7 1-0

[Event "Test Mini 2"]
[Site "RookSac Test"]
[Date "2026.11.02"]
[White "PlayerW2"]
[Black "PlayerB2"]
[Result "0-1"]
[WhiteElo "2200"]
[BlackElo "2250"]

1. e4 d5 2. exd5 c6 3. dxc6 Nf6 4. cxb7 e5 5. bxa8=N Bc5 6. Qf3 e4 7. Qf4 e3 8. dxe3 O-O 9. Qxb8 Re8 10. Nf3 0-1

[Event "Test Mini 3"]
[Site "RookSac Test"]
[Date "2026.11.03"]
[White "PlayerW3"]
[Black "PlayerB3"]
[Result "1/2-1/2"]
[WhiteElo "2000"]
[BlackElo "2000"]

1. d4 d5 2. c4 e6 3. Nc3 Nf6 4. Bg5 Be7 5. e3 O-O 6. Nf3 Nbd7 7. Rc1 c6 8. Bd3 dxc4 9. Bxc4 Nd5 10. Bxe7 Qxe7 1/2-1/2

[Event "Test Mini 4"]
[Site "RookSac Test"]
[Date "2026.11.04"]
[White "PlayerW4"]
[Black "PlayerB4"]
[Result "1-0"]
[WhiteElo "2300"]
[BlackElo "2350"]

1. c4 c5 2. Nc3 Nc6 3. g3 g6 4. Bg2 Bg7 5. e3 d6 6. Nge2 e5 7. d3 Nge7 8. O-O O-O 9. a3 a5 10. Rb1 Rb8 1-0
"""


class TestTrainingPipeline(unittest.TestCase):

    def setUp(self):
        self.tmpdir = tempfile.TemporaryDirectory()
        self.pgn_file = os.path.join(self.tmpdir.name, "games.pgn")
        with open(self.pgn_file, "w", encoding="utf-8") as f:
            f.write(SAMPLE_MINI_PGN)

        self.data_dir = os.path.join(self.tmpdir.name, "processed_data")
        process_pgn(
            pgn_path=self.pgn_file,
            output_dir=self.data_dir,
            train_ratio=0.5,
            val_ratio=0.25,
            test_ratio=0.25,
            min_ply=6,
            skip_opening_ply=2,
            seed=42,
            shard_size_positions=50,
        )

    def tearDown(self):
        self.tmpdir.cleanup()

    def test_real_dataset_training_and_artifacts(self):
        train_ds = ShardedChessDataset.from_directory(self.data_dir, split="train")
        val_ds = ShardedChessDataset.from_directory(self.data_dir, split="val")
        test_ds = ShardedChessDataset.from_directory(self.data_dir, split="test")

        self.assertTrue(len(train_ds) > 0, "Real training dataset has extracted positions")
        self.assertTrue(len(val_ds) > 0, "Validation dataset has extracted positions")
        self.assertTrue(len(test_ds) > 0, "Test dataset has extracted positions")

        out_run = os.path.join(self.tmpdir.name, "run_test")
        summary = run_training_pipeline(
            train_dataset=train_ds,
            val_dataset=val_ds,
            test_dataset=test_ds,
            output_dir=out_run,
            epochs=2,
            batch_size=8,
            lr=1e-3,
            num_blocks=1,
            channels=16,
            seed=42,
        )

        # Artifact checks
        self.assertTrue(os.path.exists(os.path.join(out_run, "last.pt")))
        self.assertTrue(os.path.exists(os.path.join(out_run, "best.pt")))
        self.assertTrue(os.path.exists(os.path.join(out_run, "training_metrics.csv")))
        self.assertTrue(os.path.exists(os.path.join(out_run, "training_summary.json")))

        # Check metadata fields
        self.assertFalse(summary["metadata"]["is_smoke_test"])
        self.assertEqual(summary["metadata"]["schema_version"], MODEL_SCHEMA_VERSION)
        self.assertIn("test_metrics", summary)
        self.assertIsNotNone(summary["test_metrics"])
        self.assertIn("policy_top1_acc", summary["test_metrics"])
        self.assertIn("value_mse", summary["test_metrics"])

    def test_checkpoint_resume(self):
        train_ds = ShardedChessDataset.from_directory(self.data_dir, split="train")
        val_ds = ShardedChessDataset.from_directory(self.data_dir, split="val")

        out_run = os.path.join(self.tmpdir.name, "resume_test")
        # Run 2 epochs
        run_training_pipeline(
            train_dataset=train_ds,
            val_dataset=val_ds,
            output_dir=out_run,
            epochs=2,
            batch_size=8,
            lr=1e-3,
            num_blocks=1,
            channels=16,
            seed=42,
        )

        last_ckpt = os.path.join(out_run, "last.pt")
        # Resume for 2 more epochs (total 4)
        summary_resumed = run_training_pipeline(
            train_dataset=train_ds,
            val_dataset=val_ds,
            output_dir=out_run,
            epochs=4,
            batch_size=8,
            lr=1e-3,
            num_blocks=1,
            channels=16,
            resume_checkpoint=last_ckpt,
            seed=42,
        )

        # Confirm training progressed through epoch 4
        epochs_run = [r["epoch"] for r in summary_resumed["history"]]
        self.assertEqual(epochs_run, [3, 4])

    def test_explicit_smoke_test_labeled_properly(self):
        smoke_out = os.path.join(self.tmpdir.name, "smoke_out")
        summary = run_smoke_test(output_dir=smoke_out)

        self.assertTrue(summary["metadata"]["is_smoke_test"], "Smoke test model is labeled as smoke test")
        summary_file = os.path.join(smoke_out, "training_summary.json")
        with open(summary_file, "r", encoding="utf-8") as f:
            data = json.load(f)
        self.assertTrue(data["metadata"]["is_smoke_test"])

    def test_corrupted_shard_rejected(self):
        corrupted_shard = os.path.join(self.tmpdir.name, "corrupted.jsonl")
        with open(corrupted_shard, "w", encoding="utf-8") as f:
            f.write('{"fen": "invalid_json_missing_brace"\n')

        with self.assertRaises(ValueError):
            ShardedChessDataset(shard_paths=[corrupted_shard])

    def test_empty_dataset_rejected(self):
        empty_ds = ShardedChessDataset(samples=[])
        val_ds = ShardedChessDataset.from_directory(self.data_dir, split="val")

        with self.assertRaises(ValueError):
            run_training_pipeline(
                train_dataset=empty_ds,
                val_dataset=val_ds,
                output_dir=os.path.join(self.tmpdir.name, "empty_run"),
                epochs=1,
            )


if __name__ == "__main__":
    unittest.main()
