import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[2]
CLI = PROJECT_ROOT / "tools" / "reverse" / "extract_battle_evidence.py"


class ExtractBattleEvidenceCliTests(unittest.TestCase):
    def run_cli(self, *arguments: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [sys.executable, str(CLI), *arguments],
            cwd=PROJECT_ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_help_succeeds(self):
        result = self.run_cli("--help")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("--game-dir", result.stdout)
        self.assertIn("--output", result.stdout)

    def test_missing_required_binary_returns_validation_error(self):
        with tempfile.TemporaryDirectory() as directory:
            result = self.run_cli(
                "--game-dir", directory,
                "--output", str(Path(directory) / "manifest.json"),
                "--validate-only",
            )

        self.assertEqual(result.returncode, 2)
        self.assertIn("missing required binary: Z.COM", result.stderr)

    def test_hash_mismatch_returns_distinct_error(self):
        with tempfile.TemporaryDirectory() as directory:
            game_dir = Path(directory)
            (game_dir / "Z.COM").write_bytes(b"not the approved loader")
            (game_dir / "Z.DAT").write_bytes(b"not the approved executable")
            result = self.run_cli(
                "--game-dir", str(game_dir),
                "--output", str(game_dir / "manifest.json"),
                "--validate-only",
            )

        self.assertEqual(result.returncode, 3)
        self.assertIn("hash mismatch: Z.COM", result.stderr)

    @unittest.skipUnless(os.environ.get("HOJY_ORIGINAL_DATA_DIR"), "original data not configured")
    def test_approved_binaries_validate(self):
        game_dir = os.environ["HOJY_ORIGINAL_DATA_DIR"]
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "manifest.json"
            result = self.run_cli(
                "--game-dir", game_dir,
                "--output", str(output),
                "--validate-only",
            )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
