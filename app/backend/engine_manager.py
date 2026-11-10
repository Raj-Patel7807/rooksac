import asyncio
import os
import re
from typing import Any, Dict, List, Optional, Tuple


class UciEngineProcess:
    """Manages an isolated UCI engine process via stdio with timeouts and safety checks."""

    def __init__(
        self,
        binary_path: str,
        eval_mode: str = "Classical",
        model_path: str = "",
        use_policy: bool = False,
        neural_weight: int = 50,
        hash_mb: int = 64,
    ):
        self.binary_path = binary_path
        self.eval_mode = eval_mode
        self.model_path = model_path
        self.use_policy = use_policy
        self.neural_weight = neural_weight
        self.hash_mb = hash_mb

        self.process: Optional[asyncio.subprocess.Process] = None
        self._lock = asyncio.Lock()
        self._ready = False
        self.model_loaded = False
        self.fallback_active = False
        self.model_details: Dict[str, Any] = {}

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

        # 1. Initialize UCI handshake
        await self.send_command("uci")
        while True:
            line = await self.read_line(timeout=5.0)
            if line is None or line.strip() == "uciok":
                break

        # 2. Configure Hash
        await self.send_command(f"setoption name Hash value {self.hash_mb}")

        # 3. Configure ModelPath first if provided
        if self.model_path and os.path.exists(self.model_path):
            await self.send_command(f"setoption name ModelPath value {os.path.abspath(self.model_path)}")

        # 4. Configure EvalMode (Classical, Neural, Hybrid)
        await self.send_command(f"setoption name EvalMode value {self.eval_mode}")

        # 5. Configure UsePolicy and NeuralWeight
        await self.send_command(f"setoption name UsePolicy value {'true' if self.use_policy else 'false'}")
        await self.send_command(f"setoption name NeuralWeight value {self.neural_weight}")

        # 6. Check readiness and parse model diagnostic output
        await self.send_command("isready")
        self.model_loaded = False
        self.fallback_active = False
        while True:
            line = await self.read_line(timeout=5.0)
            if line is None:
                break
            line_str = line.strip()
            if "model loaded:" in line_str or "loaded model" in line_str:
                self.model_loaded = True
                self.model_details["raw"] = line_str
            elif "model fallback active:" in line_str or "fallback active" in line_str:
                self.fallback_active = True
            elif line_str == "readyok":
                break

        # In strict Neural mode, if model didn't load, mark engine unready
        if self.eval_mode.lower() == "neural" and not self.model_loaded:
            self._ready = False
            return False

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

    async def drain_until_bestmove_or_terminate(self, drain_timeout: float = 1.0):
        """
        After sending 'stop', drains lines until 'bestmove' is received to prevent
        residual UCI output from poisoning the next command. If bestmove does not appear
        within drain_timeout, terminates and restarts the process.
        """
        try:
            start_t = asyncio.get_event_loop().time()
            while asyncio.get_event_loop().time() - start_t < drain_timeout:
                line = await self.read_line(timeout=0.2)
                if line and line.startswith("bestmove"):
                    return
        except Exception:
            pass

        # If still not drained cleanly, force restart process
        await self.close()
        await self.start()

    async def analyze_position(
        self, fen: str, depth: int = 8, movetime_ms: Optional[int] = None, timeout: float = 10.0
    ) -> Dict[str, Any]:
        async with self._lock:
            if not self.is_alive():
                started = await self.start()
                if not started:
                    return {"error": "Engine binary not available or failed to start with requested model"}

            # Reset engine state before analysis
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

            timed_out = False
            start_time = asyncio.get_event_loop().time()
            while True:
                elapsed = asyncio.get_event_loop().time() - start_time
                if elapsed > timeout:
                    timed_out = True
                    await self.send_command("stop")
                    await self.drain_until_bestmove_or_terminate()
                    break

                line = await self.read_line(timeout=timeout)
                if line is None:
                    # EOF or read timeout on dead process
                    timed_out = True
                    await self.close()
                    break

                if line.startswith("info"):
                    depth_m = re.search(r"depth (\d+)", line)
                    if depth_m:
                        depth_reached = int(depth_m.group(1))

                    score_m = re.search(r"score cp (-?\d+)", line)
                    if score_m:
                        score_cp = int(score_m.group(1))

                    nodes_m = re.search(r"nodes (\d+)", line)
                    if nodes_m:
                        nodes = int(nodes_m.group(1))

                    nps_m = re.search(r"nps (\d+)", line)
                    if nps_m:
                        nps = int(nps_m.group(1))

                elif line.startswith("bestmove"):
                    tokens = line.split()
                    if len(tokens) >= 2:
                        best_move = tokens[1]
                    break

            if timed_out and not best_move:
                return {"error": "Search timed out before receiving bestmove", "timeout": True}

            return {
                "fen": fen,
                "best_move": best_move,
                "depth": depth_reached,
                "nodes": nodes,
                "nps": nps,
                "score_cp": score_cp,
            }

    async def close(self):
        self._ready = False
        if self.process:
            try:
                await self.send_command("quit")
                await asyncio.wait_for(self.process.wait(), timeout=2.0)
            except Exception:
                try:
                    self.process.kill()
                except OSError:
                    pass
            finally:
                self.process = None


class BoundedEnginePool:
    """Bounded worker pool with persistent per-worker engine processes and bounded request queue."""

    def __init__(
        self,
        binary_path: str,
        max_workers: int = 4,
        max_queue_size: int = 100,
        eval_mode: str = "Classical",
        model_path: str = "",
        use_policy: bool = False,
        neural_weight: int = 50,
        hash_mb: int = 64,
    ):
        self.binary_path = binary_path
        self.max_workers = max(1, max_workers)
        self.max_queue_size = max_queue_size
        self.eval_mode = eval_mode
        self.model_path = model_path
        self.use_policy = use_policy
        self.neural_weight = neural_weight
        self.hash_mb = hash_mb

        self.active_count = 0
        self.waiting_count = 0
        self._workers: List[Optional[UciEngineProcess]] = [None] * self.max_workers
        self._available: asyncio.Queue  # type: ignore[type-arg]
        self._initialised = False
        self._init_lock = asyncio.Lock()

    async def _ensure_initialised(self):
        """Lazily initialize the worker index queue on first use (event-loop safe)."""
        if self._initialised:
            return
        async with self._init_lock:
            if self._initialised:
                return
            self._available = asyncio.Queue(maxsize=self.max_workers)
            for i in range(self.max_workers):
                await self._available.put(i)
            self._initialised = True

    async def ping_readiness(self) -> Tuple[bool, bool, bool, Dict[str, Any]]:
        """
        Checks real engine readiness by starting or probing a worker.
        Returns: (engine_ready, model_ready, fallback_active, model_details)
        """
        await self._ensure_initialised()
        slot = await self._available.get()
        try:
            worker = self._workers[slot]
            if worker is None or not worker.is_alive():
                if worker is not None:
                    await worker.close()
                worker = UciEngineProcess(
                    self.binary_path,
                    eval_mode=self.eval_mode,
                    model_path=self.model_path,
                    use_policy=self.use_policy,
                    neural_weight=self.neural_weight,
                    hash_mb=self.hash_mb,
                )
                started = await worker.start()
                if not started:
                    self._workers[slot] = None
                    return False, False, False, {}
                self._workers[slot] = worker

            return True, worker.model_loaded, worker.fallback_active, worker.model_details
        finally:
            await self._available.put(slot)

    async def execute_analysis(
        self, fen: str, depth: int, movetime_ms: Optional[int] = None, timeout: float = 10.0
    ) -> Dict[str, Any]:
        await self._ensure_initialised()

        # Reject if queue capacity is exceeded
        if self.waiting_count >= self.max_queue_size:
            return {"error": "Server busy: maximum analysis queue size reached", "queue_full": True}

        self.waiting_count += 1
        slot = -1
        try:
            # Wait for worker slot
            slot = await self._available.get()
        finally:
            self.waiting_count -= 1

        self.active_count += 1
        try:
            worker = self._workers[slot]
            if worker is None or not worker.is_alive():
                if worker is not None:
                    await worker.close()
                worker = UciEngineProcess(
                    self.binary_path,
                    eval_mode=self.eval_mode,
                    model_path=self.model_path,
                    use_policy=self.use_policy,
                    neural_weight=self.neural_weight,
                    hash_mb=self.hash_mb,
                )
                started = await worker.start()
                if not started:
                    self._workers[slot] = None
                    return {"error": "Engine binary not available or failed to start with requested model"}
                self._workers[slot] = worker

            res = await worker.analyze_position(fen, depth=depth, movetime_ms=movetime_ms, timeout=timeout)
            return res
        except Exception as exc:  # noqa: BLE001
            if self._workers[slot] is not None:
                await self._workers[slot].close()  # type: ignore[union-attr]
                self._workers[slot] = None
            return {"error": str(exc)}
        finally:
            self.active_count -= 1
            if slot != -1:
                await self._available.put(slot)

    async def shutdown(self):
        """Gracefully terminates all persistent worker processes."""
        await self._ensure_initialised()
        for i, worker in enumerate(self._workers):
            if worker is not None:
                await worker.close()
                self._workers[i] = None
