# ML and neural network pipeline

RookSac includes an offline PyTorch-based supervised policy-value training pipeline located in `research/nn/`.

## Architecture

The policy-value network is an AlphaZero-style convolutional residual architecture with separate policy and value output heads:

- **Input Tensor:** $18 \times 8 \times 8$ bitplane representation:
  - 12 piece planes (P, N, B, R, Q, K for White; p, n, b, r, q, k for Black)
  - 1 side-to-move plane (all 1s if White, all 0s if Black)
  - 4 castling right planes (White Kingside, White Queenside, Black Kingside, Black Queenside)
  - 1 en-passant file plane
- **Backbone:** Initial $3 \times 3$ convolution to $N$ filters (default 64) with batch normalization and ReLU, followed by a sequence of residual blocks.
- **Policy Head:** $1 \times 1$ convolution, batch normalization, ReLU, and linear projection to 4,096 policy logits (source square $0..63 \times$ destination square $0..63$).
- **Value Head:** $1 \times 1$ convolution, batch normalization, ReLU, linear layers with tanh activation yielding a scalar $v \in [-1, 1]$ representing expected outcome from White's perspective.

## Training Pipeline

1. **Position Extraction:** `tools/pgn_extract.py` reads PGN games, extracts moves and game results, and splits data by whole game IDs into train (70%), validation (15%), and test (15%) manifests.
2. **Feature Encoding:** `research/nn/features.py` provides bidirectional mapping between FEN/moves and tensor representations.
3. **Model & Loss:** `research/nn/model.py` and `research/nn/train.py` train with joint loss:
   $$\mathcal{L} = \mathcal{L}_{\text{CE}}(\mathbf{p}, \mathbf{\pi}) + c \cdot \mathcal{L}_{\text{MSE}}(v, z)$$
   where $\mathbf{\pi}$ is the target move distribution and $z \in [-1, 1]$ is the final game result.
4. **Validation:** Evaluates top-1 policy accuracy and value Mean Squared Error.
