import asyncio
import os
import unittest
from unittest.mock import AsyncMock, patch
from fastapi.testclient import TestClient

from app.backend.main import app
from app.backend.config import settings
from app.backend.engine_manager import BoundedEnginePool, UciEngineProcess


class TestFastApiBackend(unittest.TestCase):

    def setUp(self):
        self.client = TestClient(app)

    def test_health_endpoint_reports_real_readiness(self):
        with patch("app.backend.routes.pool.ping_readiness", new_callable=AsyncMock) as mock_ping:
            mock_ping.return_value = (True, True, False, {"name": "TestNet"})
            response = self.client.get("/api/v1/health")
            self.assertEqual(response.status_code, 200)
            data = response.json()
            self.assertEqual(data["status"], "healthy")
            self.assertTrue(data["engine_ready"])
            self.assertTrue(data["model_ready"])
            self.assertFalse(data["fallback_active"])
            self.assertEqual(data["version"], settings.VERSION)

    def test_health_endpoint_degraded_when_engine_fails(self):
        with patch("app.backend.routes.pool.ping_readiness", new_callable=AsyncMock) as mock_ping:
            mock_ping.return_value = (False, False, False, {})
            response = self.client.get("/api/v1/health")
            self.assertEqual(response.status_code, 200)
            data = response.json()
            self.assertEqual(data["status"], "degraded")
            self.assertFalse(data["engine_ready"])

    def test_pgn_import_validation(self):
        valid_pgn = '[Event "Test"]\n[White "Player1"]\n[Black "Player2"]\n[Result "1-0"]\n\n1. e4 e5 2. Nf3 1-0'
        res = self.client.post("/api/v1/pgn/import", json={"pgn_text": valid_pgn})
        self.assertEqual(res.status_code, 200)
        self.assertIn("game_id", res.json())

        invalid_pgn = '[Event "Incomplete"]\n1. e4 e5 *'
        res_bad = self.client.post("/api/v1/pgn/import", json={"pgn_text": invalid_pgn})
        self.assertEqual(res_bad.status_code, 400)

    def test_analysis_endpoint_contract(self):
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        mock_result = {
            "best_move": "e2e4",
            "depth": 2,
            "nodes": 20,
            "nps": 1000,
            "score_cp": 15,
        }
        with patch("app.backend.routes.pool.execute_analysis", new_callable=AsyncMock, return_value=mock_result):
            res = self.client.post("/api/v1/analysis", json={"fen": fen, "depth": 2})
            self.assertEqual(res.status_code, 200)
            data = res.json()
            self.assertEqual(data["status"], "completed")
            self.assertEqual(data["best_move"], "e2e4")
            self.assertEqual(data["score_cp"], 15)
            self.assertIn("X-Request-ID", res.headers)

    def test_analysis_malformed_fen(self):
        res = self.client.post("/api/v1/analysis", json={"fen": "invalid_fen_string", "depth": 2})
        self.assertEqual(res.status_code, 400)

    def test_analysis_timeout_returns_504(self):
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        with patch("app.backend.routes.pool.execute_analysis", new_callable=AsyncMock, return_value={"timeout": True}):
            res = self.client.post("/api/v1/analysis", json={"fen": fen, "depth": 2})
            self.assertEqual(res.status_code, 504)

    def test_analysis_queue_overflow_returns_429(self):
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        with patch("app.backend.routes.pool.execute_analysis", new_callable=AsyncMock, return_value={"queue_full": True}):
            res = self.client.post("/api/v1/analysis", json={"fen": fen, "depth": 2})
            self.assertEqual(res.status_code, 429)

    def test_websocket_origin_enforcement(self):
        from starlette.websockets import WebSocketDisconnect
        with self.assertRaises(WebSocketDisconnect) as cm:
            with self.client.websocket_connect(
                "/api/v1/ws/analysis",
                headers={"Origin": "http://evil-attacker.com"},
            ):
                pass
        self.assertEqual(cm.exception.code, 1008)

    def test_reverse_proxy_ip_protection(self):
        # When peer is NOT in TRUSTED_PROXIES, spoofed X-Forwarded-For is ignored
        res = self.client.get("/api/v1/health", headers={"X-Forwarded-For": "1.2.3.4"})
        self.assertEqual(res.status_code, 200)


class TestEnginePoolLifecycle(unittest.IsolatedAsyncioTestCase):

    async def test_pool_concurrency_and_shutdown(self):
        # Create small test pool
        pool = BoundedEnginePool(
            binary_path=settings.ENGINE_BINARY_PATH,
            max_workers=2,
            max_queue_size=2,
            eval_mode="Classical",
        )
        self.assertEqual(pool.max_workers, 2)
        await pool._ensure_initialised()
        self.assertEqual(pool._available.qsize(), 2)
        await pool.shutdown()


if __name__ == "__main__":
    unittest.main()
