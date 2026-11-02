# ADR-022 — Supervised policy-value neural network and training pipeline

- **Status:** Accepted
- **Context / problem:** Modern chess engines leverage neural representations for evaluation and move suggestion. Supervised learning on human and master game databases predicts both move choice (policy) and outcome (value). To maintain modularity, the deep learning pipeline must remain strictly offline in Python (`research/nn/`), decoupled from the C++ search runtime.
- **Options considered:**
  1. Integrating heavy C++ inference engines (e.g. ONNX Runtime, libtorch) directly into the core engine.
  2. Building an offline Python PyTorch training and feature generation pipeline that exports validated model architectures and manifests without touching the native engine hot path.
- **Decision:** Implement option 2:
  - **Feature Representation:** Fixed $18 \times 8 \times 8$ bitplane tensor encoding 12 piece types, side-to-move, 4 castling privileges, and en-passant column.
  - **Move Vocabulary:** Explicit $64 \times 64 = 4096$ square-to-square action space with exact bidirectional encoding and decoding.
  - **Model Architecture:** Lightweight convolutional residual network (`PolicyValueNet`) with configurable residual blocks, batch normalization, policy head (4096 logits), and value head ($\tanh \in [-1, 1]$).
  - **Objective Function:** Joint policy cross-entropy and value mean squared error loss:
    $$\mathcal{L} = \mathcal{L}_{\text{CE}}(\mathbf{p}, \mathbf{\pi}) + c \cdot \mathcal{L}_{\text{MSE}}(v, z)$$
  - **Leak-Free Whole-Game Splitting:** Dataset partitioning strictly grouped by unique game identifiers (70% train, 15% val, 15% test).
  - **Isolation Boundary:** Neural network training code is confined to `research/nn/` and `tools/`, preserving the lightweight C++ engine independence.
- **Consequences:**
  - Fast, modular supervised neural training without impacting C++ engine build times or native dependencies.
  - Provable zero-leakage dataset validation and reproducible training seeds.

