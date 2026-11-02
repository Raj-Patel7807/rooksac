"""
research/nn/model.py

Dual-headed policy-value convolutional residual neural network in PyTorch.
Predicts move probabilities (policy head) and game outcome in [-1, 1] (value head).
"""

import torch
import torch.nn as nn
import torch.nn.functional as F

class ResBlock(nn.Module):
    def __init__(self, channels: int):
        super().__init__()
        self.conv1 = nn.Conv2d(channels, channels, kernel_size=3, padding=1, bias=False)
        self.bn1 = nn.BatchNorm2d(channels)
        self.conv2 = nn.Conv2d(channels, channels, kernel_size=3, padding=1, bias=False)
        self.bn2 = nn.BatchNorm2d(channels)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        residual = x
        out = F.relu(self.bn1(self.conv1(x)))
        out = self.bn2(self.conv2(out))
        out += residual
        return F.relu(out)

class PolicyValueNet(nn.Module):
    def __init__(self, in_channels: int = 18, num_blocks: int = 4, channels: int = 64, num_actions: int = 4096):
        super().__init__()
        # Initial convolutional layer
        self.conv_in = nn.Conv2d(in_channels, channels, kernel_size=3, padding=1, bias=False)
        self.bn_in = nn.BatchNorm2d(channels)

        # Residual backbone
        self.res_blocks = nn.ModuleList([ResBlock(channels) for _ in range(num_blocks)])

        # Policy head: conv 1x1 -> bn -> relu -> linear 4096
        self.policy_conv = nn.Conv2d(channels, 16, kernel_size=1, bias=False)
        self.policy_bn = nn.BatchNorm2d(16)
        self.policy_fc = nn.Linear(16 * 8 * 8, num_actions)

        # Value head: conv 1x1 -> bn -> relu -> fc -> fc -> tanh
        self.value_conv = nn.Conv2d(channels, 4, kernel_size=1, bias=False)
        self.value_bn = nn.BatchNorm2d(4)
        self.value_fc1 = nn.Linear(4 * 8 * 8, 64)
        self.value_fc2 = nn.Linear(64, 1)

    def forward(self, x: torch.Tensor):
        out = F.relu(self.bn_in(self.conv_in(x)))
        for block in self.res_blocks:
            out = block(out)

        # Policy branch
        p = F.relu(self.policy_bn(self.policy_conv(out)))
        p = p.flatten(start_dim=1)
        policy_logits = self.policy_fc(p)

        # Value branch
        v = F.relu(self.value_bn(self.value_conv(out)))
        v = v.flatten(start_dim=1)
        v = F.relu(self.value_fc1(v))
        value = torch.tanh(self.value_fc2(v))

        return policy_logits, value

