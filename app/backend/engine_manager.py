import asyncio
import os
import re
from typing import Dict, List, Optional

class UciEngineProcess:
    """Manages an isolated UCI engine process via stdio with timeouts and safety checks."""

    def __init__(self, binary_path: str):
        self.binary_path = binary_path
        self.process: Optional[asyncio.subprocess.Process] = None
        self._lock = asyncio.Lock()
        self._ready = False

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

        # Configure hash table size for better performance
        await self.send_command("setoption name Hash value 64")
        self._ready = True
        return True

    def is_alive(self) -> bool:
        """Returns True if the process is running and ready."""
        return self._ready and self.process is not None and self.process.returncode is None

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
            if not self.is_alive():
                started = await self.start()
                if not started:
                    return {"error": "Engine binary not available or failed to start"}

            # Reset engine state before each analysis to avoid position bleed
            await self.send_command("ucinewgame")

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
        self._ready = False
        if self.process:
            try:
                await self.send_command("quit")
                await asyncio.wait_for(self.process.wait(), timeout=2.0)
            except Exception:
                self.process.kill()
            finally:
                self.process = None


class BoundedEnginePool:
    """Bounded worker pool with persistent per-worker engine processes.

    Each worker slot holds a long-lived UciEngineProcess that is reused across
    analysis requests, eliminating the ~800 ms engine startup overhead per move.
    Workers are lazy-started on first use and automatically restarted on failure.
    """

    def __init__(self, binary_path: str, max_workers: int = 4):
        self.binary_path = binary_path
        self.max_workers = max_workers
        self.semaphore = asyncio.Semaphore(max_workers)
        self.active_count = 0
        # Persistent worker processes; created lazily and protected by _pool_lock
        self._workers: List[Optional[UciEngineProcess]] = [None] * max_workers
        self._available: asyncio.Queue  # type: ignore[type-arg]
        self._initialised = False
        self._init_lock = asyncio.Lock()

    async def _ensure_initialised(self):
        """Lazily initialise the worker index queue on first use (event-loop safe)."""
        if self._initialised:
            return
        async with self._init_lock:
            if self._initialised:
                return
            self._available = asyncio.Queue(maxsize=self.max_workers)
            for i in range(self.max_workers):
                await self._available.put(i)
            self._initialised = True

    async def execute_analysis(
        self, fen: str, depth: int, movetime_ms: Optional[int] = None, timeout: float = 10.0
    ) -> Dict:
        await self._ensure_initialised()
        # Acquire a worker slot index from the queue (blocks if all workers are busy)
        slot: int = await self._available.get()
        self.active_count += 1
        try:
            worker = self._workers[slot]
            if worker is None or not worker.is_alive():
                # Lazily create or restart a persistent worker
                if worker is not None:
                    await worker.close()
                worker = UciEngineProcess(self.binary_path)
                started = await worker.start()
                if not started:
                    self._workers[slot] = None
                    return {"error": "Engine binary not available or failed to start"}
                self._workers[slot] = worker

            res = await worker.analyze_position(fen, depth=depth, movetime_ms=movetime_ms, timeout=timeout)
            return res
        except Exception as exc:  # noqa: BLE001
            # On any unexpected error, close the worker so the slot gets a fresh one next time
            if self._workers[slot] is not None:
                await self._workers[slot].close()  # type: ignore[union-attr]
                self._workers[slot] = None
            return {"error": str(exc)}
        finally:
            self.active_count -= 1
            await self._available.put(slot)

    async def shutdown(self):
        """Gracefully terminates all persistent worker processes."""
        await self._ensure_initialised()
        for i, worker in enumerate(self._workers):
            if worker is not None:
                await worker.close()
                self._workers[i] = None
