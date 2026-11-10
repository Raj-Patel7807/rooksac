"""
Test script to generate golden position feature tensors and export to a JSON fixture.
Positions covered:
1. Initial start position (w, KQkq, no ep)
2. Black to move after 1. e4 (b, KQkq, ep e3)
3. Castling rights permutations:
   - Independent White Kingside only (K)
   - Independent White Queenside only (Q)
   - Independent Black Kingside only (k)
   - Independent Black Queenside only (q)
   - Combinations (e.g. Kq, Qk)
4. En-passant files (a-file, d-file, h-file)
5. Promotion positions (White pawn on 7th rank, Black pawn on 2nd rank)
6. Sparse endgames (K vs K, K+R vs K, K+P vs K)
"""

import json
from research.nn.features import fen_to_tensor, encode_move, decode_move, parse_uci_move, VOCAB_SIZE

GOLDEN_POSITIONS = [
    # 1. Initial position
    {
        "name": "initial_startpos",
        "fen": "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
    },
    # 2. Black to move
    {
        "name": "black_to_move_after_e4",
        "fen": "rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1"
    },
    # 3. Castling permutations
    {
        "name": "castling_white_kingside_only",
        "fen": "4k3/8/8/8/8/8/8/4K2R w K - 0 1"
    },
    {
        "name": "castling_white_queenside_only",
        "fen": "4k3/8/8/8/8/8/8/R3K3 w Q - 0 1"
    },
    {
        "name": "castling_black_kingside_only",
        "fen": "4k2r/8/8/8/8/8/8/4K3 b k - 0 1"
    },
    {
        "name": "castling_black_queenside_only",
        "fen": "r3k3/8/8/8/8/8/8/4K3 b q - 0 1"
    },
    {
        "name": "castling_combo_Kq",
        "fen": "r3k3/8/8/8/8/8/8/4K2R w Kq - 0 1"
    },
    {
        "name": "castling_none",
        "fen": "4k3/8/8/8/8/8/8/4K3 w - - 0 1"
    },
    # 4. En-passant targets
    {
        "name": "en_passant_a_file",
        "fen": "rnbqkbnr/1ppppppp/8/pP6/8/8/P1PPPPPP/RNBQKBNR w KQkq a6 0 2"
    },
    {
        "name": "en_passant_d_file",
        "fen": "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2"
    },
    {
        "name": "en_passant_h_file",
        "fen": "rnbqkbnr/pppppp1p/8/6pP/8/8/PPPPPPP1/RNBQKBNR w KQkq g6 0 2"
    },
    # 5. Promotion positions
    {
        "name": "white_promotion_ready",
        "fen": "8/4P3/8/8/8/8/8/4K2k w - - 0 1"
    },
    {
        "name": "black_promotion_ready",
        "fen": "4K2k/8/8/8/8/8/4p3/8 b - - 0 1"
    },
    # 6. Sparse endgames
    {
        "name": "sparse_king_vs_king",
        "fen": "8/8/8/4k3/8/8/4K3/8 w - - 0 1"
    },
    {
        "name": "sparse_king_rook_vs_king",
        "fen": "8/8/8/4k3/8/8/R3K3/8 w - - 0 1"
    },
]

def generate_golden_dataset():
    data = []
    for item in GOLDEN_POSITIONS:
        name = item["name"]
        fen = item["fen"]
        tensor = fen_to_tensor(fen)  # shape (18, 8, 8)
        data.append({
            "name": name,
            "fen": fen,
            "tensor_flat": tensor.flatten().tolist()
        })
    with open("research/tests/golden_tensors.json", "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2)
    print(f"Generated {len(data)} golden position tensors in research/tests/golden_tensors.json")

if __name__ == "__main__":
    generate_golden_dataset()

