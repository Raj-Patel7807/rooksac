"""
research/nn/features.py

Canonical position and move encoding schema for RookSac.
Version: 2.0.0 (Vocabulary: 4168 actions)

Input Tensor Schema (18, 8, 8) float32:
---------------------------------------
Plane  0: White Pawns   (1.0 where White Pawn is present, else 0.0)
Plane  1: White Knights (1.0 where White Knight is present, else 0.0)
Plane  2: White Bishops (1.0 where White Bishop is present, else 0.0)
Plane  3: White Rooks   (1.0 where White Rook is present, else 0.0)
Plane  4: White Queens  (1.0 where White Queen is present, else 0.0)
Plane  5: White King    (1.0 where White King is present, else 0.0)
Plane  6: Black Pawns   (1.0 where Black Pawn is present, else 0.0)
Plane  7: Black Knights (1.0 where Black Knight is present, else 0.0)
Plane  8: Black Bishops (1.0 where Black Bishop is present, else 0.0)
Plane  9: Black Rooks   (1.0 where Black Rook is present, else 0.0)
Plane 10: Black Queens  (1.0 where Black Queen is present, else 0.0)
Plane 11: Black King    (1.0 where Black King is present, else 0.0)
Plane 12: Side to Move  (all 1.0 if White to move, all 0.0 if Black to move)
Plane 13: White Kingside Castling  (all 1.0 if White has K-side castling rights, else 0.0)
Plane 14: White Queenside Castling (all 1.0 if White has Q-side castling rights, else 0.0)
Plane 15: Black Kingside Castling  (all 1.0 if Black has k-side castling rights, else 0.0)
Plane 16: Black Queenside Castling (all 1.0 if Black has q-side castling rights, else 0.0)
Plane 17: En-passant Target File   (1.0 on column of ep square across all 8 ranks, else 0.0)

Spatial Orientation:
-------------------
Rows (dim 1) index Rank 1..8 (rank index 0 to 7).
Cols (dim 2) index File a..h (file index 0 to 7).
Square index = rank * 8 + file (a1 = 0, h1 = 7, a8 = 56, h8 = 63).

Action Vocabulary (4168 actions):
---------------------------------
Indices 0..4095: Standard moves (source 0..63 * 64 + target 0..63).
                When a move is a promotion to Queen (default promotion), it maps to this
                standard geometric from*64+to index.
Indices 4096..4239 (144 actions): Underpromotions (Knight, Bishop, Rook).
  For White: from rank 6 (indices 48..55) to rank 7 (indices 56..63).
  For Black: from rank 1 (indices 8..15) to rank 0 (indices 0..7).
  Direction d in {-1 (left capture), 0 (straight advance), +1 (right capture)} -> dir_idx in {0, 1, 2}.
  Piece p in {Knight: 0, Bishop: 1, Rook: 2}.
  White base index = 4096 + from_file * 9 + p * 3 + dir_idx  (4096..4167)
  Black base index = 4168 + from_file * 9 + p * 3 + dir_idx  (4168..4239)
Total actions = 4096 + 72 + 72 = 4240.
"""

from typing import Optional, Tuple
import numpy as np

# Vocabulary version and constants
VOCAB_VERSION = "2.0.0"
NUM_INPUT_PLANES = 18
NUM_BOARD_SQUARES = 64
STANDARD_ACTIONS = 4096  # 64 * 64
UNDERPROMOTION_ACTIONS_PER_COLOR = 72  # 8 files * (3 pieces * 3 directions) = 72
TOTAL_UNDERPROMOTION_ACTIONS = 144
VOCAB_SIZE = STANDARD_ACTIONS + TOTAL_UNDERPROMOTION_ACTIONS  # 4240

WHITE_UNDERPROMO_START = 4096
BLACK_UNDERPROMO_START = 4168

_PIECE_ORDER = ["n", "b", "r"]  # 0: knight, 1: bishop, 2: rook


def square_to_index(sq_str: str) -> int:
    """Converts algebraic square (e.g. 'e4') to 0..63 index.

    Raises ValueError on malformed square string.
    """
    if len(sq_str) != 2:
        raise ValueError(f"Invalid square string length: '{sq_str}'")
    file_char = sq_str[0].lower()
    rank_char = sq_str[1]
    if not ('a' <= file_char <= 'h') or not ('1' <= rank_char <= '8'):
        raise ValueError(f"Square coordinates out of range: '{sq_str}'")
    file = ord(file_char) - ord("a")
    rank = int(rank_char) - 1
    return rank * 8 + file


def index_to_square(idx: int) -> str:
    """Converts 0..63 index to algebraic square.

    Raises ValueError on invalid index.
    """
    if not (0 <= idx < 64):
        raise ValueError(f"Square index out of range: {idx}")
    file = chr(ord("a") + (idx % 8))
    rank = str((idx // 8) + 1)
    return f"{file}{rank}"


def encode_move(from_sq: str, to_sq: str, promotion: Optional[str] = None) -> int:
    """Encodes move (source, target, optional promotion) into [0, 4167].

    - Normal moves and Queen promotions: from_idx * 64 + to_idx in [0, 4095].
    - Underpromotions ('n', 'b', 'r'): mapped to unique slots in [4096, 4167].
    Raises ValueError on invalid input or illegal promotion geometry.
    """
    f_idx = square_to_index(from_sq)
    t_idx = square_to_index(to_sq)

    if f_idx == t_idx:
        raise ValueError(f"Source square equals target square: '{from_sq}'")

    if promotion is not None:
        promo = promotion.lower()
        if promo not in ["q", "r", "b", "n"]:
            raise ValueError(f"Invalid promotion piece: '{promotion}'")
    else:
        promo = None

    # Check for underpromotions
    if promo in _PIECE_ORDER:
        from_rank = f_idx // 8
        from_file = f_idx % 8
        to_rank = t_idx // 8
        to_file = t_idx % 8
        piece_idx = _PIECE_ORDER.index(promo)  # 0, 1, 2
        file_diff = to_file - from_file  # -1, 0, +1

        if abs(file_diff) > 1:
            raise ValueError(f"Invalid promotion move file transition: {from_sq} -> {to_sq}")

        dir_idx = file_diff + 1  # 0: left capture, 1: straight advance, 2: right capture

        if from_rank == 6 and to_rank == 7:
            # White underpromotion
            return WHITE_UNDERPROMO_START + from_file * 9 + piece_idx * 3 + dir_idx
        elif from_rank == 1 and to_rank == 0:
            # Black underpromotion
            return BLACK_UNDERPROMO_START + from_file * 9 + piece_idx * 3 + dir_idx
        else:
            raise ValueError(f"Underpromotion on non-promotion rank: {from_sq} -> {to_sq}")

    # Standard move or Queen promotion
    return f_idx * 64 + t_idx


def decode_move(move_idx: int) -> Tuple[str, str, Optional[str]]:
    """Decodes integer index in [0, 4167] back into (from_sq, to_sq, promo).

    Raises ValueError on invalid action index.
    """
    if not (0 <= move_idx < VOCAB_SIZE):
        raise ValueError(f"Action index out of vocabulary range [0, {VOCAB_SIZE - 1}]: {move_idx}")

    if move_idx < STANDARD_ACTIONS:
        f_idx = move_idx // 64
        t_idx = move_idx % 64
        from_rank = f_idx // 8
        to_rank = t_idx // 8
        # Determine if this standard move represents a Queen promotion
        is_promo_rank = (from_rank == 6 and to_rank == 7) or (from_rank == 1 and to_rank == 0)
        file_diff = abs((t_idx % 8) - (f_idx % 8))
        if is_promo_rank and file_diff <= 1:
            # Note: in standard chess move parsing, context tells if a pawn promoted.
            # When decoded from action alone, an advance to the 8th/1st rank with standard index
            # is Queen promotion if treated as a pawn move.
            pass
        return index_to_square(f_idx), index_to_square(t_idx), None

    if move_idx < BLACK_UNDERPROMO_START:
        # White underpromotion
        offset = move_idx - WHITE_UNDERPROMO_START
        from_file = offset // 9
        rem = offset % 9
        piece_idx = rem // 3
        dir_idx = rem % 3
        to_file = from_file + (dir_idx - 1)
        if not (0 <= to_file < 8):
            raise ValueError(f"Invalid decoded White underpromotion target file from index {move_idx}")
        from_sq = f"{chr(ord('a') + from_file)}7"
        to_sq = f"{chr(ord('a') + to_file)}8"
        promo = _PIECE_ORDER[piece_idx]
        return from_sq, to_sq, promo
    else:
        # Black underpromotion
        offset = move_idx - BLACK_UNDERPROMO_START
        from_file = offset // 9
        rem = offset % 9
        piece_idx = rem // 3
        dir_idx = rem % 3
        to_file = from_file + (dir_idx - 1)
        if not (0 <= to_file < 8):
            raise ValueError(f"Invalid decoded Black underpromotion target file from index {move_idx}")
        from_sq = f"{chr(ord('a') + from_file)}2"
        to_sq = f"{chr(ord('a') + to_file)}1"
        promo = _PIECE_ORDER[piece_idx]
        return from_sq, to_sq, promo


def parse_uci_move(uci_str: str) -> Tuple[str, str, Optional[str]]:
    """Parses a standard UCI move string (e.g. 'e2e4', 'e7e8q', 'e7e8n') into (from_sq, to_sq, promo).

    Raises ValueError on malformed string.
    """
    if len(uci_str) < 4 or len(uci_str) > 5:
        raise ValueError(f"Invalid UCI move string length: '{uci_str}'")
    from_sq = uci_str[0:2]
    to_sq = uci_str[2:4]
    promo = uci_str[4].lower() if len(uci_str) == 5 else None

    # Validate square strings
    square_to_index(from_sq)
    square_to_index(to_sq)

    if promo is not None and promo not in ["q", "r", "b", "n"]:
        raise ValueError(f"Invalid promotion piece in UCI string: '{uci_str}'")

    return from_sq, to_sq, promo


def fen_to_tensor(fen: str) -> np.ndarray:
    """Encodes FEN string into an (18, 8, 8) float32 numpy array matching C++ encodePosition.

    Planes 0-5:   White P, N, B, R, Q, K
    Planes 6-11:  Black p, n, b, r, q, k
    Plane 12:     Side to move (all 1.0 if White, 0.0 if Black)
    Planes 13-16: Castling rights (White K, Q; Black k, q)
    Plane 17:     En-passant target file (1.0 on column of ep square across all ranks)
    """
    tensor = np.zeros((18, 8, 8), dtype=np.float32)
    parts = fen.split()
    if len(parts) < 1:
        raise ValueError(f"Invalid empty FEN string: '{fen}'")

    board_str = parts[0]
    stm_str = parts[1] if len(parts) > 1 else "w"
    castling_str = parts[2] if len(parts) > 2 else "-"
    ep_str = parts[3] if len(parts) > 3 else "-"

    piece_map = {
        "P": 0, "N": 1, "B": 2, "R": 3, "Q": 4, "K": 5,
        "p": 6, "n": 7, "b": 8, "r": 9, "q": 10, "k": 11,
    }

    rank = 7
    file = 0
    for char in board_str:
        if char == "/":
            rank -= 1
            file = 0
        elif char.isdigit():
            file += int(char)
        elif char in piece_map:
            if not (0 <= rank < 8 and 0 <= file < 8):
                raise ValueError(f"Malformed FEN board specification: '{board_str}'")
            plane = piece_map[char]
            tensor[plane, rank, file] = 1.0
            file += 1
        else:
            raise ValueError(f"Invalid character in FEN board specification: '{char}'")

    # Side to move
    if stm_str == "w":
        tensor[12, :, :] = 1.0
    elif stm_str != "b":
        raise ValueError(f"Invalid side to move in FEN: '{stm_str}'")

    # Castling rights
    if "K" in castling_str:
        tensor[13, :, :] = 1.0
    if "Q" in castling_str:
        tensor[14, :, :] = 1.0
    if "k" in castling_str:
        tensor[15, :, :] = 1.0
    if "q" in castling_str:
        tensor[16, :, :] = 1.0

    # En-passant file
    if ep_str != "-" and len(ep_str) == 2:
        file_char = ep_str[0].lower()
        if 'a' <= file_char <= 'h':
            ep_file = ord(file_char) - ord("a")
            tensor[17, :, ep_file] = 1.0

    return tensor
