"""
tools/export_model.py

Exports PyTorch PolicyValueNet model weights to RookSac native binary format (.rks v2).
Fuses batch normalization layers into convolutional weights/biases for zero-overhead inference:
  w_fused = w * (gamma / sqrt(var + eps))
  b_fused = (b - mean) * (gamma / sqrt(var + eps)) + beta

Features:
- Validates weights for NaN / Inf
- Computes SHA-256 integrity checksum over Header + Metadata + Weights
- Performs atomic file write (temporary file -> rename)
- Embeds provenance JSON metadata (commit, checkpoint hash, architecture, schemas)
"""

import argparse
import hashlib
import json
import os
import struct
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, Optional, Tuple

import numpy as np
import torch
import torch.nn as nn

from research.nn.features import VOCAB_SIZE
from research.nn.model import PolicyValueNet

MAGIC = b"ROOKSAC2"
FORMAT_VERSION = 2
INPUT_CHANNELS = 18
EXPECTED_VOCAB_SIZE = VOCAB_SIZE  # 4240


def get_git_commit() -> str:
    """Returns current git commit hash, or 'unknown' if not in a git repo."""
    try:
        res = subprocess.run(
            ["git", "rev-parse", "HEAD"],
            capture_output=True,
            text=True,
            check=False,
        )
        if res.returncode == 0:
            return res.stdout.strip()
    except Exception:
        pass
    return "unknown"


def fuse_conv_bn(conv: nn.Conv2d, bn: nn.BatchNorm2d) -> Tuple[np.ndarray, np.ndarray]:
    """Fuses Conv2d and BatchNorm2d parameters into an equivalent (weight, bias) pair."""
    w = conv.weight.data
    mean = bn.running_mean
    var = bn.running_var
    eps = bn.eps
    gamma = bn.weight.data if bn.weight is not None else torch.ones_like(mean)
    beta = bn.bias.data if bn.bias is not None else torch.zeros_like(mean)

    std = torch.sqrt(var + eps)
    scale = gamma / std

    # Weight: [out_channels, in_channels, kH, kW]
    w_fused = w * scale.view(-1, 1, 1, 1)

    # Bias: [out_channels]
    if conv.bias is not None:
        b = conv.bias.data
        b_fused = (b - mean) * scale + beta
    else:
        b_fused = -mean * scale + beta

    w_np = w_fused.cpu().float().numpy().astype("<f4")
    b_np = b_fused.cpu().float().numpy().astype("<f4")

    if not np.all(np.isfinite(w_np)) or not np.all(np.isfinite(b_np)):
        raise ValueError("Fused Conv-BN weights contain NaN or Inf values")

    return w_np, b_np


def linear_to_numpy(linear: nn.Linear) -> Tuple[np.ndarray, np.ndarray]:
    """Extracts weight and bias from Linear layer as float32 little-endian, verifying finiteness."""
    w_np = linear.weight.data.cpu().float().numpy().astype("<f4")
    b_np = linear.bias.data.cpu().float().numpy().astype("<f4")

    if not np.all(np.isfinite(w_np)) or not np.all(np.isfinite(b_np)):
        raise ValueError("Linear weights contain NaN or Inf values")

    return w_np, b_np


def export_rks(
    model: PolicyValueNet,
    output_path: str,
    metadata_extra: Optional[Dict[str, Any]] = None,
    checkpoint_path: Optional[str] = None,
) -> str:
    """
    Exports model parameters to a binary .rks v2 file atomically.
    Validates all tensors and writes an appended SHA-256 integrity checksum.
    """
    model.eval()

    num_blocks = len(model.res_blocks)
    channels = model.conv_in.out_channels
    num_actions = model.policy_fc.out_features

    if num_blocks < 1 or num_blocks > 32:
        raise ValueError(f"Invalid num_blocks={num_blocks}, must be between 1 and 32")
    if channels < 8 or channels > 512:
        raise ValueError(f"Invalid channels={channels}, must be between 8 and 512")
    if num_actions != EXPECTED_VOCAB_SIZE:
        raise ValueError(f"Invalid num_actions={num_actions}, expected {EXPECTED_VOCAB_SIZE}")

    # Build weights payload in memory/bytearray
    payload_parts = []

    # 1. Conv In
    w, b = fuse_conv_bn(model.conv_in, model.bn_in)
    payload_parts.extend([w.tobytes(), b.tobytes()])

    # 2. ResBlocks
    for block in model.res_blocks:
        w1, b1 = fuse_conv_bn(block.conv1, block.bn1)
        w2, b2 = fuse_conv_bn(block.conv2, block.bn2)
        payload_parts.extend([w1.tobytes(), b1.tobytes(), w2.tobytes(), b2.tobytes()])

    # 3. Policy Head
    w_pconv, b_pconv = fuse_conv_bn(model.policy_conv, model.policy_bn)
    w_pfc, b_pfc = linear_to_numpy(model.policy_fc)
    payload_parts.extend([w_pconv.tobytes(), b_pconv.tobytes(), w_pfc.tobytes(), b_pfc.tobytes()])

    # 4. Value Head
    w_vconv, b_vconv = fuse_conv_bn(model.value_conv, model.value_bn)
    w_vfc1, b_vfc1 = linear_to_numpy(model.value_fc1)
    w_vfc2, b_vfc2 = linear_to_numpy(model.value_fc2)
    payload_parts.extend([
        w_vconv.tobytes(),
        b_vconv.tobytes(),
        w_vfc1.tobytes(),
        b_vfc1.tobytes(),
        w_vfc2.tobytes(),
        b_vfc2.tobytes(),
    ])

    weights_bytes = b"".join(payload_parts)
    payload_bytes_len = len(weights_bytes)

    # Compute checkpoint hash if provided
    ckpt_hash = ""
    if checkpoint_path and os.path.exists(checkpoint_path):
        with open(checkpoint_path, "rb") as cf:
            ckpt_hash = hashlib.sha256(cf.read()).hexdigest()

    # Build metadata dict
    meta_dict: Dict[str, Any] = {
        "checkpoint_hash": ckpt_hash,
        "source_git_commit": get_git_commit(),
        "architecture": f"PolicyValueNet-ResNet({num_blocks}b{channels}c)",
        "input_schema": "(18, 8, 8) [pieces: 12, stm: 1, castling: 4, ep: 1]",
        "action_schema": f"v2.0.0 ({num_actions} actions)",
        "num_blocks": num_blocks,
        "channels": channels,
        "num_actions": num_actions,
        "export_timestamp": datetime.now(timezone.utc).isoformat(),
    }
    if metadata_extra:
        meta_dict.update(metadata_extra)

    meta_json = json.dumps(meta_dict, indent=2, sort_keys=True).encode("utf-8")
    meta_bytes_len = len(meta_json)

    # 64-byte Header:
    # 0x00: magic (8)
    # 0x08: format_version (uint32)
    # 0x0C: input_channels (uint32)
    # 0x10: num_blocks (uint32)
    # 0x14: channels (uint32)
    # 0x18: num_actions (uint32)
    # 0x1C: meta_bytes (uint32)
    # 0x20: payload_bytes (uint64)
    # 0x28: reserved (24 bytes)
    header = bytearray(64)
    header[0:8] = MAGIC
    struct.pack_into(
        "<IIIIIIQ",
        header,
        8,
        FORMAT_VERSION,
        INPUT_CHANNELS,
        num_blocks,
        channels,
        num_actions,
        meta_bytes_len,
        payload_bytes_len,
    )
    # bytes 40..63 are reserved zeros

    # Compute SHA-256 over Header + Metadata + Weights
    hasher = hashlib.sha256()
    hasher.update(header)
    hasher.update(meta_json)
    hasher.update(weights_bytes)
    digest = hasher.digest()

    out_path = Path(output_path).resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)

    # Atomic write via temporary file
    temp_fd, temp_file_path = tempfile.mkstemp(dir=out_path.parent, prefix="model_export_", suffix=".tmp")
    try:
        with os.fdopen(temp_fd, "wb") as f:
            f.write(header)
            f.write(meta_json)
            f.write(weights_bytes)
            f.write(digest)
            f.flush()
            os.fsync(f.fileno())

        # Validate temporary file size before replacing
        expected_size = 64 + meta_bytes_len + payload_bytes_len + 32
        actual_size = os.path.getsize(temp_file_path)
        if actual_size != expected_size:
            raise IOError(f"Export file size mismatch: expected {expected_size}, got {actual_size}")

        os.replace(temp_file_path, out_path)
    except Exception:
        if os.path.exists(temp_file_path):
            try:
                os.remove(temp_file_path)
            except OSError:
                pass
        raise

    print(f"Successfully exported {out_path} (blocks={num_blocks}, channels={channels}, sha256={hasher.hexdigest()[:16]}...)")
    return hasher.hexdigest()


def main():
    parser = argparse.ArgumentParser(description="Export PyTorch model to RookSac binary .rks format.")
    parser.add_argument("--checkpoint", required=True, help="Path to input PyTorch .pt checkpoint")
    parser.add_argument("--output", required=True, help="Path to output .rks file")
    args = parser.parse_args()

    checkpoint_path = Path(args.checkpoint)
    if not checkpoint_path.is_file():
        sys.exit(f"Error: checkpoint file '{checkpoint_path}' does not exist")

    checkpoint = torch.load(checkpoint_path, map_location="cpu", weights_only=False)

    # Detect if checkpoint is an explicit smoke test
    config = checkpoint.get("config", {})
    is_smoke_test = checkpoint.get("is_smoke_test", config.get("is_smoke_test", False))
    if is_smoke_test:
        print("Note: Exporting a SMOKE-TEST model fixture (not a production trained model).")

    meta = checkpoint.get("metadata", {})
    num_blocks = config.get("num_blocks", meta.get("num_blocks", 2))
    channels = config.get("channels", meta.get("channels", 32))
    num_actions = meta.get("vocab_size", meta.get("num_actions", EXPECTED_VOCAB_SIZE))

    model = PolicyValueNet(in_channels=18, num_blocks=num_blocks, channels=channels, num_actions=num_actions)
    if "model_state_dict" in checkpoint:
        model.load_state_dict(checkpoint["model_state_dict"])
    else:
        model.load_state_dict(checkpoint)

    extra_meta = {
        "is_smoke_test": is_smoke_test,
        "dataset_manifest_checksum": checkpoint.get("dataset_manifest_checksum", ""),
        "training_metrics": checkpoint.get("metrics", {}),
    }

    export_rks(model, args.output, metadata_extra=extra_meta, checkpoint_path=str(checkpoint_path))


if __name__ == "__main__":
    main()
