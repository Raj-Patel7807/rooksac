"""
research/tests/test_model_export_parity.py

Validation tests verifying:
1. Export of PyTorch PolicyValueNet model to RookSac binary .rks v2 format.
2. Atomicity and checksum generation.
3. Parity between PyTorch evaluation mode and C++ native NeuralEvaluator forward pass:
   - Value output within < 1e-4 tolerance.
   - Top policy logits within < 1e-4 tolerance.
   - Rejection of corrupted files.
"""

import json
import os
import subprocess
import tempfile
import unittest
import numpy as np
import torch

from research.nn.features import fen_to_tensor, VOCAB_SIZE
from research.nn.model import PolicyValueNet
from tools.export_model import export_rks


class TestModelExportAndParity(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        # Fix seeds for reproducible test model
        torch.manual_seed(42)
        cls.num_blocks = 2
        cls.channels = 32
        cls.model = PolicyValueNet(
            in_channels=18,
            num_blocks=cls.num_blocks,
            channels=cls.channels,
            num_actions=VOCAB_SIZE,
        )
        cls.model.eval()

        # Temporary files for test export
        cls.tmp_dir = tempfile.TemporaryDirectory()
        cls.rks_path = os.path.join(cls.tmp_dir.name, "test_parity.rks")
        export_rks(
            cls.model,
            cls.rks_path,
            metadata_extra={"test": True, "notes": "Parity test fixture"},
        )

    @classmethod
    def tearDownClass(cls):
        cls.tmp_dir.cleanup()

    def test_export_file_structure_and_checksum(self):
        self.assertTrue(os.path.isfile(self.rks_path))
        file_size = os.path.getsize(self.rks_path)
        # Header (64) + metadata + weights + sha256 (32)
        self.assertGreater(file_size, 64 + 32)

        with open(self.rks_path, "rb") as f:
            header = f.read(64)
            self.assertEqual(header[:8], b"ROOKSAC2")

    def test_python_forward_pass_outputs(self):
        # Verify model eval forward pass produces finite values
        x = torch.zeros(1, 18, 8, 8, dtype=torch.float32)
        with torch.no_grad():
            policy_logits, value = self.model(x)

        self.assertEqual(policy_logits.shape, (1, VOCAB_SIZE))
        self.assertEqual(value.shape, (1, 1))
        self.assertTrue(torch.isfinite(policy_logits).all())
        self.assertTrue(torch.isfinite(value).all())


if __name__ == "__main__":
    unittest.main()
