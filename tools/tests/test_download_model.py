import hashlib
import os
import tempfile
import unittest
from unittest.mock import MagicMock, patch

from tools.download_model import compute_sha256, download_and_verify


class TestDownloadModel(unittest.TestCase):
    def test_compute_sha256(self):
        with tempfile.NamedTemporaryFile(delete=False) as f:
            f.write(b"RookSac model test bytes")
            f_path = f.name
        try:
            expected = hashlib.sha256(b"RookSac model test bytes").hexdigest()
            self.assertEqual(compute_sha256(f_path), expected)
        finally:
            if os.path.exists(f_path):
                os.remove(f_path)

    @patch("tools.download_model.urllib.request.urlopen")
    def test_download_and_verify_success(self, mock_urlopen):
        payload = b"Sample .rks file payload for testing"
        expected_hash = hashlib.sha256(payload).hexdigest()

        mock_resp = MagicMock()
        mock_resp.read.side_effect = [payload, b""]
        mock_urlopen.return_value.__enter__.return_value = mock_resp

        with tempfile.TemporaryDirectory() as tmpdir:
            out_file = os.path.join(tmpdir, "downloaded.rks")
            download_and_verify(
                repo_id="test/repo",
                filename="model.rks",
                revision="1234567890abcdef1234567890abcdef12345678",
                output_path=out_file,
                expected_sha256=expected_hash,
            )
            self.assertTrue(os.path.exists(out_file))
            with open(out_file, "rb") as f:
                self.assertEqual(f.read(), payload)

    @patch("tools.download_model.urllib.request.urlopen")
    def test_download_and_verify_integrity_mismatch(self, mock_urlopen):
        payload = b"Tampered .rks payload"
        mock_resp = MagicMock()
        mock_resp.read.side_effect = [payload, b""]
        mock_urlopen.return_value.__enter__.return_value = mock_resp

        with tempfile.TemporaryDirectory() as tmpdir:
            out_file = os.path.join(tmpdir, "downloaded.rks")
            with self.assertRaises(ValueError):
                download_and_verify(
                    repo_id="test/repo",
                    filename="model.rks",
                    revision="1234567890abcdef1234567890abcdef12345678",
                    output_path=out_file,
                    expected_sha256="0000000000000000000000000000000000000000000000000000000000000000",
                )
            self.assertFalse(os.path.exists(out_file))


if __name__ == "__main__":
    unittest.main()
