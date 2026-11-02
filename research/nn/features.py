"""
research/nn/features.py

Position and move encoding schema for supervised neural network training.
Maps board states to an (18, 8, 8) float tensor representation and moves to vocabulary indices.
"""

from typing import Dict, Optional, Tuple
import numpy as np

# Move vocabulary: 64 from-squares * 64 to-squares = 4096 actions
VOCAB_SIZE = 4096

def square_to_index(sq_str: str) -> int:
    """Converts algebraic square (e.g. 'e4') to 0..63 index."""
    file = ord(sq_str[0].lower()) - ord('a')
    rank = int(sq_str[1]) - 1
    return rank * 8 + file

def index_to_square(idx: int) -> str:
    """Converts 0..63 index to algebraic square."""
    file = chr(ord('a') + (idx % 8))
    rank = str((idx // 8) + 1)
    return f"{file}{rank}"

def encode_move(from_sq: str, to_sq: str) -> int:
    """Encodes move (source, target) into integer in [0, 4095]."""
    f_idx = square_to_index(from_sq)
    t_idx = square_to_index(to_sq)
    return f_idx * 64 + t_idx

def decode_move(move_idx: int) -> Tuple[str, str]:
    """Decodes integer index back into (from_sq, to_sq)."""
    f_idx = move_idx // 64
    t_idx = move_idx % 64
    return index_to_square(f_idx), index_to_square(t_idx)

def fen_to_tensor(fen: str) -> np.ndarray:
    """
    Encodes FEN string into an (18, 8, 8) float32 numpy array:
    Planes 0-5:   White P, N, B, R, Q, K
    Planes 6-11:  Black p, n, b, r, q, k
    Plane 12:     Side to move (all 1 if White, 0 if Black)
    Planes 13-16: Castling rights (White K, Q; Black k, q)
    Plane 17:     En-passant target file (1 on target column)
    """
    tensor = np.zeros((18, 8, 8), dtype=np.float32)
    parts = fen.split()
    board_str = parts[0]
    stm_str = parts[1] if len(parts) > 1 else 'w'
    castling_str = parts[2] if len(parts) > 2 else '-'
    ep_str = parts[3] if len(parts) > 3 else '-'

    piece_map = {
        'P': 0, 'N': 1, 'B': 2, 'R': 3, 'Q': 4, 'K': 5,
        'p': 6, 'n': 7, 'b': 8, 'r': 9, 'q': 10, 'k': 11
    }

    rank = 7
    file = 0
    for char in board_str:
        if char == '/':
            rank -= 1
            file = 0
        elif char.isdigit():
            file += int(char)
        elif char in piece_map:
            plane = piece_map[char]
            tensor[plane, rank, file] = 1.0
            file += 1

    # Side to move
    if stm_str == 'w':
        tensor[12, :, :] = 1.0

    # Castling rights
    if 'K' in castling_str: tensor[13, :, :] = 1.0
    if 'Q' in castling_str: tensor[14, :, :] = 1.0
    if 'k' in castling_str: tensor[15, :, :] = 1.0
    if 'q' in castling_str: tensor[16, :, :] = 1.0

    # En-passant file
    if ep_str != '-' and len(ep_str) == 2:
        ep_file = ord(ep_str[0].lower()) - ord('a')
        if 0 <= ep_file < 8:
            tensor[17, :, ep_file] = 1.0

    return tensor

