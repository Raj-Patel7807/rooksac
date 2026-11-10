# ML and Neural Network Pipeline

RookSac includes a PyTorch-based supervised policy-value training pipeline located in `research/nn/` and a native zero-overhead C++ forward-pass inference engine in `engine/src/neural_evaluator.cpp`.

---

## 1. Feature Representation & Tensor Schema

Both Python (`research/nn/features.py`) and C++ (`rooksac::neural::encodePosition`) encode positions into an identical **$18 \times 8 \times 8$ float32 tensor** (1,152 floats total in C-major planar order: `channel * 64 + rank * 8 + file`).

### Plane Definitions

| Plane | Name / Description | Values |
| :--- | :--- | :--- |
| **0** | White Pawns | $1.0$ on squares occupied by White Pawns, else $0.0$ |
| **1** | White Knights | $1.0$ on squares occupied by White Knights, else $0.0$ |
| **2** | White Bishops | $1.0$ on squares occupied by White Bishops, else $0.0$ |
| **3** | White Rooks | $1.0$ on squares occupied by White Rooks, else $0.0$ |
| **4** | White Queens | $1.0$ on squares occupied by White Queens, else $0.0$ |
| **5** | White King | $1.0$ on the square occupied by White King, else $0.0$ |
| **6** | Black Pawns | $1.0$ on squares occupied by Black Pawns, else $0.0$ |
| **7** | Black Knights | $1.0$ on squares occupied by Black Knights, else $0.0$ |
| **8** | Black Bishops | $1.0$ on squares occupied by Black Bishops, else $0.0$ |
| **9** | Black Rooks | $1.0$ on squares occupied by Black Rooks, else $0.0$ |
| **10** | Black Queens | $1.0$ on squares occupied by Black Queens, else $0.0$ |
| **11** | Black King | $1.0$ on the square occupied by Black King, else $0.0$ |
| **12** | Side to Move | Entire $8 \times 8$ plane is $1.0$ if White to move; $0.0$ if Black to move |
| **13** | White Kingside Castling | Entire $8 \times 8$ plane is $1.0$ if White has K-side castling rights, else $0.0$ |
| **14** | White Queenside Castling | Entire $8 \times 8$ plane is $1.0$ if White has Q-side castling rights, else $0.0$ |
| **15** | Black Kingside Castling | Entire $8 \times 8$ plane is $1.0$ if Black has k-side castling rights, else $0.0$ |
| **16** | Black Queenside Castling | Entire $8 \times 8$ plane is $1.0$ if Black has q-side castling rights, else $0.0$ |
| **17** | En-Passant Target File | $1.0$ on all 8 ranks along the file of the en-passant square, else $0.0$ |

### Spatial Orientation
- **Rank (dimension 1)**: Ranks 1 to 8 map to row indices 0 through 7 ($0 = \text{Rank 1}, 7 = \text{Rank 8}$).
- **File (dimension 2)**: Files a to h map to column indices 0 through 7 ($0 = \text{a}, 7 = \text{h}$).
- **Square index**: $\text{sq} = \text{rank} \times 8 + \text{file}$ (e.g., $a1 = 0, h1 = 7, a8 = 56, h8 = 63$).

---

## 2. Action Vocabulary & Encoding Specification

**Vocabulary Version**: `2.0.0`  
**Total Actions**: `4240`

To ensure unambiguous policy targets during training and exact move ranking during C++ tree search, standard moves and underpromotions have distinct labels:

1. **Standard Moves and Queen Promotions (Indices $0 \dots 4095$)**:
   - Encoded as $\text{action} = \text{from\_idx} \times 64 + \text{to\_idx}$.
   - Queen promotions use this standard geometric transition index (since queen promotion is the default and most frequent move).
2. **White Underpromotions (Indices $4096 \dots 4167$, 72 slots)**:
   - Covers pawn moves from Rank 7 (index 6) to Rank 8 (index 7).
   - $\text{action} = 4096 + \text{from\_file} \times 9 + \text{piece\_idx} \times 3 + \text{dir\_idx}$.
   - $\text{from\_file} \in [0, 7]$ (Files a..h).
   - $\text{piece\_idx} \in \{0: \text{Knight}, 1: \text{Bishop}, 2: \text{Rook}\}$.
   - $\text{dir\_idx} \in \{0: \text{left capture } (-1), 1: \text{straight advance } (0), 2: \text{right capture } (+1)\}$.
3. **Black Underpromotions (Indices $4168 \dots 4239$, 72 slots)**:
   - Covers pawn moves from Rank 2 (index 1) to Rank 1 (index 0).
   - $\text{action} = 4168 + \text{from\_file} \times 9 + \text{piece\_idx} \times 3 + \text{dir\_idx}$.

### Error Handling & Strict Rejection
Both Python and C++ encoders:
- Strictly reject invalid algebraic squares (e.g. `e9`, `i4`).
- Strictly reject illegal promotion piece characters or types (anything other than `q`, `r`, `b`, `n`).
- Strictly reject underpromotion requests on non-promotion ranks or impossible file jumps ($|\Delta\text{file}| > 1$).
- Reject invalid action indices during decoding.

---

## 3. Architecture & Neural Pipeline

- **Dual-Headed ResNet (`PolicyValueNet`)**:
  - Backbone: initial $3 \times 3$ convolution to $N$ filters (default 64) with BatchNorm and ReLU, followed by residual blocks.
  - Policy Head: $1 \times 1$ conv (16 filters), BatchNorm, ReLU, and linear projection to **4,240 policy logits**.
  - Value Head: $1 \times 1$ conv (4 filters), BatchNorm, ReLU, linear layers with tanh activation yielding $v \in [-1, 1]$.
- **Training Loss**:
  $$\mathcal{L} = \mathcal{L}_{\text{CE}}(\mathbf{p}, \mathbf{\pi}) + c \cdot \mathcal{L}_{\text{MSE}}(v, z)$$
- **Value Calibration**:
  Converts $v \in [-1, 1]$ into integer centipawns:
  $$\text{cp} = -\frac{400}{K} \log_{10}\left(\frac{1 - v}{1 + v}\right)$$
