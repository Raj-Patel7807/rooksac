import asyncio
import os
import re
from typing import Dict, Optional, Tuple

class UciEngineProcess:
    """Manages an isolated UCI engine process via stdio with timeouts and safety checks."""

    def __init__(self, binary_path: str):
        self.binary_path = binary_path
        self.process: Optional[asyncio.subprocess.Process] = None
        self._lock = asyncio.Lock()

    async def start(self) -> bool:
        if not os.path.exists(self.binary_path):
            return False

        # Direct argument array execution; strictly avoids shell=True
        self.process = await asyncio.create_subprocess_exec(
            self.binary_path,
            stdin=asyncio.subprocess.PIPE,
            stdout=asyncio.subprocess.PIPE,
            stderr=asyncio.subprocess.PIPE,
        )

        # Initialize UCI handshake
        await self.send_command("uci")
        while True:
            line = await self.read_line()
            if not line or line.strip() == "uciok":
                break

        await self.send_command("isready")
        while True:
            line = await self.read_line()
            if not line or line.strip() == "readyok":
                break

        return True

    async def send_command(self, cmd: str):
        if self.process and self.process.stdin:
            self.process.stdin.write(f"{cmd}\n".encode())
            await self.process.stdin.drain()

    async def read_line(self, timeout: float = 5.0) -> Optional[str]:
        if not self.process or not self.process.stdout:
            return None
        try:
            line = await asyncio.wait_for(self.process.stdout.readline(), timeout=timeout)
            return line.decode().strip()
        except asyncio.TimeoutError:
            return None

    async def analyze_position(
        self, fen: str, depth: int = 8, movetime_ms: Optional[int] = None, timeout: float = 10.0
    ) -> Dict:
        async with self._lock:
            if not self.process or self.process.returncode is not None:
                started = await self.start()
                if not started:
                    return {"error": "Engine binary not available or failed to start"}

            await self.send_command(f"position fen {fen}")
            if movetime_ms:
                await self.send_command(f"go movetime {movetime_ms}")
            else:
                await self.send_command(f"go depth {depth}")

            best_move = None
            nodes = None
            nps = None
            score_cp = None
            depth_reached = None

            start_time = asyncio.get_event_loop().time()
            while True:
                elapsed = asyncio.get_event_loop().time() - start_time
                if elapsed > timeout:
                    await self.send_command("stop")
                    break

                line = await self.read_line(timeout=timeout)
                if not line:
                    break

                if line.startswith("info"):
                    # Parse info tokens
                    depth_m = re.search(r"depth (\d+)", line)
                    if depth_m: depth_reached = int(depth_m.group(1))

                    score_m = re.search(r"score cp (-?\d+)", line)
                    if score_m: score_cp = int(score_m.group(1))

                    nodes_m = re.search(r"nodes (\d+)", line)
                    if nodes_m: nodes = int(nodes_m.group(1))

                    nps_m = re.search(r"nps (\d+)", line)
                    if nps_m: nps = int(nps_m.group(1))

                elif line.startswith("bestmove"):
                    tokens = line.split()
                    if len(tokens) >= 2:
                        best_move = tokens[1]
                    break

            return {
                "fen": fen,
                "best_move": best_move,
                "depth": depth_reached,
                "nodes": nodes,
                "nps": nps,
                "score_cp": score_cp
            }

    async def close(self):
        if self.process:
            try:
                await self.send_command("quit")
                await asyncio.wait_for(self.process.wait(), timeout=2.0)
            except Exception:
                self.process.kill()
            finally:
                self.process = None

class BoundedEnginePool:
    """Bounded worker pool providing concurrency caps and queued engine work."""

    def __init__(self, binary_path: str, max_workers: int = 4):
        self.binary_path = binary_path
        self.max_workers = max_workers
        self.semaphore = asyncio.Semaphore(max_workers)
        self.active_count = 0

    async def execute_analysis(self, fen: str, depth: int, movetime_ms: Optional[int] = None, timeout: float = 10.0) -> Dict:
        async with self.semaphore:
            self.active_count += 1
            worker = UciEngineProcess(self.binary_path)
            try:
                await worker.start()
                res = await worker.analyze_position(fen, depth=depth, movetime_ms=movetime_ms, timeout=timeout)
                return res
            finally:
                await worker.close()
                self.active_count -= 1
