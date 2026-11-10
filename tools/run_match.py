"""
tools/run_match.py

Automated A/B engine match runner for RookSac.
Plays paired-opening matches between two engine configurations over standard UCI.
Supports:
- Classical RookSac
- Neural Value Evaluation
- Neural Value + Policy Ordering
- Hybrid Evaluation

Collects:
- Game results, W/D/L
- PGN file generation
- SPRT statistical test evaluation (LLR, Wald bounds, Elo estimate, 95% CI)
- Engine hardware/build metadata and opening suite SHA-256
- Benchmark metrics (NPS, nodes, depth, latency)
"""

import argparse
import hashlib
import json
import os
import subprocess
import sys
import time
from datetime import datetime, timezone
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

import chess
import chess.pgn

from tools.sprt_match import compute_llr, compute_elo_and_ci


def hash_file(filepath: str) -> str:
    """Computes SHA-256 hash of a file."""
    if not os.path.exists(filepath):
        return "not_found"
    hasher = hashlib.sha256()
    with open(filepath, "rb") as f:
        while chunk := f.read(65536):
            hasher.update(chunk)
    return hasher.hexdigest()


def get_git_commit() -> str:
    """Returns current git commit hash, or 'unknown'."""
    try:
        res = subprocess.run(["git", "rev-parse", "HEAD"], capture_output=True, text=True, check=False)
        if res.returncode == 0:
            return res.stdout.strip()
    except Exception:
        pass
    return "unknown"


class UciEngine:
    """Encapsulates a synchronous subprocess connection to a UCI engine."""

    def __init__(self, binary_path: str, options: Dict[str, Any]):
        self.binary_path = binary_path
        self.options = options
        self.process: Optional[subprocess.Popen] = None

    def start(self):
        self.process = subprocess.Popen(
            [self.binary_path],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        self.send_command("uci")
        while True:
            line = self.read_line()
            if not line or line.strip() == "uciok":
                break

        for k, v in self.options.items():
            self.send_command(f"setoption name {k} value {v}")

        self.send_command("isready")
        while True:
            line = self.read_line()
            if not line or line.strip() == "readyok":
                break

    def send_command(self, cmd: str):
        if self.process and self.process.stdin:
            self.process.stdin.write(cmd + "\n")
            self.process.stdin.flush()

    def read_line(self) -> str:
        if self.process and self.process.stdout:
            return self.process.stdout.readline()
        return ""

    def ucinewgame(self):
        self.send_command("ucinewgame")
        self.send_command("isready")
        while True:
            line = self.read_line()
            if not line or line.strip() == "readyok":
                break

    def search(
        self,
        fen: str,
        moves: List[str],
        depth: Optional[int] = None,
        movetime_ms: Optional[int] = None,
        wtime_ms: Optional[int] = None,
        btime_ms: Optional[int] = None,
        winc_ms: Optional[int] = None,
        binc_ms: Optional[int] = None,
    ) -> Tuple[str, Dict[str, Any]]:
        """Sends position and go, returns (bestmove, search_metrics)."""
        pos_cmd = f"position fen {fen}"
        if moves:
            pos_cmd += " moves " + " ".join(moves)
        self.send_command(pos_cmd)

        go_parts = ["go"]
        if depth is not None:
            go_parts.append(f"depth {depth}")
        if movetime_ms is not None:
            go_parts.append(f"movetime {movetime_ms}")
        if wtime_ms is not None:
            go_parts.append(f"wtime {wtime_ms}")
        if btime_ms is not None:
            go_parts.append(f"btime {btime_ms}")
        if winc_ms is not None:
            go_parts.append(f"winc {winc_ms}")
        if binc_ms is not None:
            go_parts.append(f"binc {binc_ms}")

        t0 = time.perf_counter()
        self.send_command(" ".join(go_parts))

        bestmove = "0000"
        metrics: Dict[str, Any] = {"depth": 0, "nodes": 0, "nps": 0, "score": 0}

        while True:
            line = self.read_line()
            if not line:
                break
            line_str = line.strip()
            if line_str.startswith("info"):
                tokens = line_str.split()
                for i in range(len(tokens) - 1):
                    if tokens[i] == "depth":
                        try:
                            metrics["depth"] = int(tokens[i + 1])
                        except ValueError:
                            pass
                    elif tokens[i] == "nodes":
                        try:
                            metrics["nodes"] = int(tokens[i + 1])
                        except ValueError:
                            pass
                    elif tokens[i] == "nps":
                        try:
                            metrics["nps"] = int(tokens[i + 1])
                        except ValueError:
                            pass
                    elif tokens[i] == "cp":
                        try:
                            metrics["score"] = int(tokens[i + 1])
                        except ValueError:
                            pass
            elif line_str.startswith("bestmove"):
                tokens = line_str.split()
                if len(tokens) >= 2:
                    bestmove = tokens[1]
                break

        elapsed = time.perf_counter() - t0
        metrics["elapsed_s"] = elapsed
        return bestmove, metrics

    def quit(self):
        if self.process:
            try:
                self.send_command("quit")
                self.process.communicate(timeout=2)
            except Exception:
                self.process.kill()
            self.process = None


def play_game(
    engine_white: UciEngine,
    engine_black: UciEngine,
    initial_fen: str,
    time_control: Dict[str, Any],
    max_plies: int = 200,
) -> Tuple[str, str, List[str], Dict[str, Any]]:
    """Plays a single game between two engines from an initial FEN."""
    board = chess.Board(initial_fen)
    moves: List[str] = []
    engine_white.ucinewgame()
    engine_black.ucinewgame()

    clocks = {
        chess.WHITE: time_control.get("initial_time_ms", 10000),
        chess.BLACK: time_control.get("initial_time_ms", 10000),
    }
    inc_ms = time_control.get("inc_ms", 100)
    depth = time_control.get("depth", None)
    movetime_ms = time_control.get("movetime_ms", None)

    total_nodes = {chess.WHITE: 0, chess.BLACK: 0}
    total_time_s = {chess.WHITE: 0.0, chess.BLACK: 0.0}

    termination = ""
    for _ in range(max_plies):
        if board.is_checkmate():
            termination = "Checkmate"
            break
        if board.is_stalemate():
            termination = "Stalemate"
            break
        if board.is_insufficient_material():
            termination = "Insufficient material"
            break
        if board.can_claim_fifty_moves():
            termination = "Fifty-move rule"
            break
        if board.can_claim_threefold_repetition():
            termination = "Threefold repetition"
            break

        turn = board.turn
        engine = engine_white if turn == chess.WHITE else engine_black

        move_str, metrics = engine.search(
            fen=initial_fen,
            moves=moves,
            depth=depth,
            movetime_ms=movetime_ms,
            wtime_ms=clocks[chess.WHITE] if depth is None and movetime_ms is None else None,
            btime_ms=clocks[chess.BLACK] if depth is None and movetime_ms is None else None,
            winc_ms=inc_ms if depth is None and movetime_ms is None else None,
            binc_ms=inc_ms if depth is None and movetime_ms is None else None,
        )

        total_nodes[turn] += metrics.get("nodes", 0)
        total_time_s[turn] += metrics.get("elapsed_s", 0.0)

        if depth is None and movetime_ms is None:
            elapsed_ms = int(metrics.get("elapsed_s", 0.0) * 1000)
            clocks[turn] = max(0, clocks[turn] - elapsed_ms + inc_ms)
            if clocks[turn] <= 0:
                termination = "Time forfeit"
                break

        try:
            m = chess.Move.from_uci(move_str)
            if m in board.legal_moves:
                board.push(m)
                moves.append(move_str)
            else:
                termination = f"Illegal move: {move_str}"
                break
        except Exception:
            termination = f"Malformed move: {move_str}"
            break

    result = board.result(claim_draw=True)
    if not termination:
        termination = "Max plies reached"

    game_stats = {
        "white_nodes": total_nodes[chess.WHITE],
        "black_nodes": total_nodes[chess.BLACK],
        "white_time_s": total_time_s[chess.WHITE],
        "black_time_s": total_time_s[chess.BLACK],
        "plies": len(moves),
    }
    return result, termination, moves, game_stats


def load_openings(path: str, max_openings: Optional[int] = None) -> List[str]:
    """Loads FENs from an EPD file."""
    fens = []
    if not os.path.exists(path):
        return [chess.STARTING_FEN]
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            fen = line.split("|")[0].strip()
            if fen:
                fens.append(fen)
                if max_openings and len(fens) >= max_openings:
                    break
    return fens if fens else [chess.STARTING_FEN]


def run_ab_match(
    binary_path: str,
    config_a: Dict[str, Any],
    config_b: Dict[str, Any],
    openings_path: str,
    output_dir: str,
    pairs: int = 10,
    time_control: Optional[Dict[str, Any]] = None,
    elo0: float = 0.0,
    elo1: float = 5.0,
) -> Dict[str, Any]:
    """
    Runs a paired A/B match between engine configuration A and B.
    Each opening is played twice (A as White, then B as White) to eliminate color bias.
    """
    if time_control is None:
        time_control = {"depth": 3}

    out_path = Path(output_dir)
    out_path.mkdir(parents=True, exist_ok=True)

    openings = load_openings(openings_path, max_openings=pairs)
    print(f"Starting A/B Match: {config_a['name']} (Candidate) vs {config_b['name']} (Baseline)")
    print(f"Pairs: {len(openings)} ({len(openings) * 2} games total), TimeControl: {time_control}")

    engine_a = UciEngine(binary_path, config_a.get("options", {}))
    engine_b = UciEngine(binary_path, config_b.get("options", {}))
    engine_a.start()
    engine_b.start()

    wins_a = 0
    losses_a = 0
    draws = 0

    pgn_path = out_path / "match.pgn"
    pgn_file = open(pgn_path, "w", encoding="utf-8")

    game_num = 1
    total_nodes_a = 0
    total_time_a = 0.0
    total_nodes_b = 0
    total_time_b = 0.0

    t_start = time.perf_counter()

    try:
        for fen in openings:
            # Game 1: Engine A as White, Engine B as Black
            res1, term1, moves1, stats1 = play_game(engine_a, engine_b, fen, time_control)
            total_nodes_a += stats1["white_nodes"]
            total_time_a += stats1["white_time_s"]
            total_nodes_b += stats1["black_nodes"]
            total_time_b += stats1["black_time_s"]

            if res1 == "1-0":
                wins_a += 1
            elif res1 == "0-1":
                losses_a += 1
            else:
                draws += 1

            write_pgn_game(pgn_file, game_num, config_a["name"], config_b["name"], fen, res1, term1, moves1)
            game_num += 1

            # Game 2: Engine B as White, Engine A as Black (Color swapped)
            res2, term2, moves2, stats2 = play_game(engine_b, engine_a, fen, time_control)
            total_nodes_b += stats2["white_nodes"]
            total_time_b += stats2["white_time_s"]
            total_nodes_a += stats2["black_nodes"]
            total_time_a += stats2["black_time_s"]

            if res2 == "0-1":
                wins_a += 1
            elif res2 == "1-0":
                losses_a += 1
            else:
                draws += 1

            write_pgn_game(pgn_file, game_num, config_b["name"], config_a["name"], fen, res2, term2, moves2)
            game_num += 1

            # Intermediate SPRT Check
            llr, lower, upper = compute_llr(wins_a, losses_a, draws, elo0, elo1)
            games_played = wins_a + losses_a + draws
            score_pct = (wins_a + 0.5 * draws) / games_played * 100.0
            print(f"[{games_played:02d}/{len(openings)*2}] Score: {score_pct:.1f}% (+{wins_a} ={draws} -{losses_a}) LLR: {llr:.3f} [{lower:.2f}, {upper:.2f}]")

    finally:
        pgn_file.close()
        engine_a.quit()
        engine_b.quit()

    total_duration = time.perf_counter() - t_start
    total_games = wins_a + losses_a + draws

    llr, lower, upper = compute_llr(wins_a, losses_a, draws, elo0, elo1)
    elo_diff, ci_lo, ci_hi = compute_elo_and_ci(wins_a, losses_a, draws)

    if llr >= upper:
        decision = "PASS"
        decision_desc = f"ACCEPT H1 (Candidate is stronger, +{elo1} Elo)"
    elif llr <= lower:
        decision = "FAIL"
        decision_desc = f"ACCEPT H0 (Candidate rejected, <= {elo0} Elo)"
    else:
        decision = "INCONCLUSIVE"
        decision_desc = "CONTINUE (Inconclusive - more games needed)"

    nps_a = int(total_nodes_a / total_time_a) if total_time_a > 0 else 0
    nps_b = int(total_nodes_b / total_time_b) if total_time_b > 0 else 0
    games_per_hour = round(total_games / (total_duration / 3600.0), 1) if total_duration > 0 else 0.0

    report: Dict[str, Any] = {
        "candidate": config_a["name"],
        "baseline": config_b["name"],
        "candidate_config": config_a,
        "baseline_config": config_b,
        "git_commit": get_git_commit(),
        "opening_suite_hash": hash_file(openings_path),
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "time_control": time_control,
        "games_played": total_games,
        "wins": wins_a,
        "losses": losses_a,
        "draws": draws,
        "score_pct": round((wins_a + 0.5 * draws) / total_games * 100.0, 2) if total_games > 0 else 0.0,
        "elo_diff": round(elo_diff, 2),
        "ci_95": [round(ci_lo, 2), round(ci_hi, 2)],
        "llr": round(llr, 3),
        "sprt_bounds": [round(lower, 3), round(upper, 3)],
        "sprt_hypotheses": {"elo0": elo0, "elo1": elo1},
        "sprt_decision": decision,
        "sprt_decision_description": decision_desc,
        "metrics": {
            "candidate_nps": nps_a,
            "baseline_nps": nps_b,
            "candidate_nodes": total_nodes_a,
            "baseline_nodes": total_nodes_b,
            "duration_s": round(total_duration, 2),
            "games_per_hour": games_per_hour,
        },
    }

    report_path = out_path / "match_report.json"
    with open(report_path, "w", encoding="utf-8") as f:
        json.dump(report, f, indent=2)

    print("=" * 60)
    print(f"Match Finished: {decision_desc}")
    print(f"Elo: {elo_diff:+.1f} [{ci_lo:+.1f}, {ci_hi:+.1f}] 95% CI | Score: {report['score_pct']}%")
    print(f"Saved artifacts to {out_path} (match.pgn, match_report.json)")
    print("=" * 60)
    return report


def write_pgn_game(
    f,
    round_num: int,
    white: str,
    black: str,
    fen: str,
    result: str,
    termination: str,
    moves: List[str],
):
    f.write(f'[Event "RookSac Match Validation"]\n')
    f.write(f'[Site "Local"]\n')
    f.write(f'[Date "{datetime.now().strftime("%Y.%m.%d")}"]\n')
    f.write(f'[Round "{round_num}"]\n')
    f.write(f'[White "{white}"]\n')
    f.write(f'[Black "{black}"]\n')
    f.write(f'[Result "{result}"]\n')
    f.write(f'[FEN "{fen}"]\n')
    f.write(f'[SetUp "1"]\n')
    f.write(f'[Termination "{termination}"]\n\n')

    b = chess.Board(fen)
    for i, m_str in enumerate(moves):
        m = chess.Move.from_uci(m_str)
        san = b.san(m)
        b.push(m)
        if i % 2 == 0:
            f.write(f"{i // 2 + 1}. {san} ")
        else:
            f.write(f"{san} ")
    f.write(f"{result}\n\n")
    f.flush()


def main():
    parser = argparse.ArgumentParser(description="Run RookSac reproducible A/B strength validation.")
    parser.add_argument("--binary", default="build/rooksac.exe" if os.name == "nt" else "build/rooksac")
    parser.add_argument("--mode-a", choices=["classical", "neural", "neural_policy", "hybrid"], default="neural")
    parser.add_argument("--mode-b", choices=["classical", "neural", "neural_policy", "hybrid"], default="classical")
    parser.add_argument("--model", default="research/checkpoints/model_test.rks")
    parser.add_argument("--openings", default="tools/openings.epd")
    parser.add_argument("--pairs", type=int, default=5, help="Number of opening pairs (2 * pairs total games)")
    parser.add_argument("--depth", type=int, default=3, help="Search depth per move")
    parser.add_argument("--movetime", type=int, default=None, help="Fixed move time in milliseconds")
    parser.add_argument("--output-dir", default="research/experiments/match_001")
    parser.add_argument("--elo0", type=float, default=0.0)
    parser.add_argument("--elo1", type=float, default=5.0)
    args = parser.parse_args()

    configs = {
        "classical": {
            "name": "RookSac-Classical",
            "options": {"EvalMode": "Classical", "UsePolicy": "false", "Hash": "16"},
        },
        "neural": {
            "name": "RookSac-NeuralValue",
            "options": {"EvalMode": "Neural", "ModelPath": os.path.abspath(args.model), "UsePolicy": "false", "Hash": "16"},
        },
        "neural_policy": {
            "name": "RookSac-NeuralValuePolicy",
            "options": {"EvalMode": "Neural", "ModelPath": os.path.abspath(args.model), "UsePolicy": "true", "Hash": "16"},
        },
        "hybrid": {
            "name": "RookSac-Hybrid",
            "options": {"EvalMode": "Hybrid", "ModelPath": os.path.abspath(args.model), "UsePolicy": "false", "NeuralWeight": "50", "Hash": "16"},
        },
    }

    tc = {}
    if args.movetime:
        tc["movetime_ms"] = args.movetime
    else:
        tc["depth"] = args.depth

    run_ab_match(
        binary_path=args.binary,
        config_a=configs[args.mode_a],
        config_b=configs[args.mode_b],
        openings_path=args.openings,
        output_dir=args.output_dir,
        pairs=args.pairs,
        time_control=tc,
        elo0=args.elo0,
        elo1=args.elo1,
    )


if __name__ == "__main__":
    main()

