"""
tools/export_model.py

Exports PyTorch PolicyValueNet model weights to RookSac native binary format (.rks).
Fuses batch normalization layers into convolutional weights/biases for zero-overhead inference:
  w_fused = w * (gamma / sqrt(var + eps))
  b_fused = (b - mean) * (gamma / sqrt(var + eps)) + beta
"""

import argparse
import struct
import sys
import torch
import torch.nn as nn
from research.nn.features import VOCAB_SIZE
from research.nn.model import PolicyValueNet

MAGIC = b"ROOKSAC1"
FORMAT_VERSION = 1


def fuse_conv_bn(conv: nn.Conv2d, bn: nn.BatchNorm2d):
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

    return w_fused.cpu().float().numpy(), b_fused.cpu().float().numpy()


def export_rks(model: PolicyValueNet, output_path: str):
    """Exports model parameters to a binary .rks file."""
    model.eval()

    in_channels = 18
    num_blocks = len(model.res_blocks)
    channels = model.conv_in.out_channels
    num_actions = model.policy_fc.out_features

    with open(output_path, "wb") as f:
        # Header (24 bytes)
        f.write(MAGIC)
        f.write(struct.pack("<IIII", FORMAT_VERSION, num_blocks, channels, num_actions))

        # 1. Conv In
        w, b = fuse_conv_bn(model.conv_in, model.bn_in)
        f.write(w.tobytes())
        f.write(b.tobytes())

        # 2. ResBlocks
        for block in model.res_blocks:
            w1, b1 = fuse_conv_bn(block.conv1, block.bn1)
            w2, b2 = fuse_conv_bn(block.conv2, block.bn2)
            f.write(w1.tobytes())
            f.write(b1.tobytes())
            f.write(w2.tobytes())
            f.write(b2.tobytes())

        # 3. Policy Head (1x1 conv + linear)
        w_pconv, b_pconv = fuse_conv_bn(model.policy_conv, model.policy_bn)
        f.write(w_pconv.tobytes())
        f.write(b_pconv.tobytes())
        f.write(model.policy_fc.weight.data.cpu().float().numpy().tobytes())
        f.write(model.policy_fc.bias.data.cpu().float().numpy().tobytes())

        # 4. Value Head (1x1 conv + linear 64 + linear 1)
        w_vconv, b_vconv = fuse_conv_bn(model.value_conv, model.value_bn)
        f.write(w_vconv.tobytes())
        f.write(b_vconv.tobytes())
        f.write(model.value_fc1.weight.data.cpu().float().numpy().tobytes())
        f.write(model.value_fc1.bias.data.cpu().float().numpy().tobytes())
        f.write(model.value_fc2.weight.data.cpu().float().numpy().tobytes())
        f.write(model.value_fc2.bias.data.cpu().float().numpy().tobytes())

    print(f"Successfully exported {output_path} (blocks={num_blocks}, channels={channels})")


def main():
    parser = argparse.ArgumentParser(description="Export PyTorch model to RookSac binary .rks format.")
    parser.add_argument("--checkpoint", default="research/checkpoints/policy_value_test.pt")
    parser.add_argument("--output", default="research/checkpoints/model_test.rks")
    args = parser.parse_args()

    # Load model
    checkpoint = torch.load(args.checkpoint, map_location="cpu", weights_only=False)
    meta = checkpoint.get("metadata", {})
    num_blocks = meta.get("num_blocks", 2)
    channels = meta.get("channels", 32)

    model = PolicyValueNet(in_channels=18, num_blocks=num_blocks, channels=channels)
    model.load_state_dict(checkpoint["model_state_dict"])
    export_rks(model, args.output)


if __name__ == "__main__":
    main()
