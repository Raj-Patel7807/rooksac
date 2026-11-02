import asyncio
import os
import unittest
from fastapi.testclient import TestClient

from app.backend.main import app
from app.backend.config import settings

class TestFastApiBackend(unittest.TestCase):

    def setUp(self):
        self.client = TestClient(app)

    def test_health_endpoint(self):
        response = self.client.get("/api/v1/health")
        self.assertEqual(response.status_code, 200)
        data = response.json()
        self.assertEqual(data["status"], "healthy")
        self.assertEqual(data["version"], settings.VERSION)

    def test_pgn_import_validation(self):
        # Valid standard PGN
        valid_pgn = '[Event "Test"]\n[White "Player1"]\n[Black "Player2"]\n[Result "1-0"]\n\n1. e4 e5 2. Nf3 1-0'
        res = self.client.post("/api/v1/pgn/import", json={"pgn_text": valid_pgn})
        self.assertEqual(res.status_code, 200)
        self.assertIn("game_id", res.json())

        # Invalid PGN missing outcome
        invalid_pgn = '[Event "Incomplete"]\n1. e4 e5 *'
        res_bad = self.client.post("/api/v1/pgn/import", json={"pgn_text": invalid_pgn})
        self.assertEqual(res_bad.status_code, 400)

    def test_analysis_endpoint_integration(self):
        # Position search on startpos
        fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"
        res = self.client.post("/api/v1/analysis", json={"fen": fen, "depth": 2})
        self.assertEqual(res.status_code, 200)
        data = res.json()
        self.assertEqual(data["status"], "completed")
        self.assertIsNotNone(data["best_move"])
        self.assertIsNotNone(data["score_cp"])

    def test_analysis_malformed_fen(self):
        res = self.client.post("/api/v1/analysis", json={"fen": "invalid_fen_string", "depth": 2})
        self.assertEqual(res.status_code, 400)

if __name__ == "__main__":
    unittest.main()
