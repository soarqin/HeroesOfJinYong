import importlib.util
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[2]
CLI = PROJECT_ROOT / "tools" / "reverse" / "extract_battle_evidence.py"


_EXTRACTION_ENABLED = bool(
    os.environ.get("HOJY_ORIGINAL_DATA_DIR") and importlib.util.find_spec("idapro")
)


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

    @unittest.skipUnless(_EXTRACTION_ENABLED, "original data or idalib Python module not configured")
    def test_full_extraction_contains_battle_evidence(self):
        game_dir = os.environ["HOJY_ORIGINAL_DATA_DIR"]
        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "manifest.json"
            result = self.run_cli(
                "--game-dir", game_dir,
                "--output", str(output),
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            data = json.loads(output.read_text(encoding="utf-8"))

        self.assertEqual(
            [(item["parent"], item["child"], item["address"])
             for item in data["dependencies"]],
            [("Z.COM", "Z.DAT", "0x10195")],
        )
        evidence = {item["evidence_id"]: item for item in data["evidence"]}
        self.assertEqual(evidence["ENTRY-ZCOM-EXEC"]["address"], "0x10195")
        self.assertEqual(evidence["DATA-WAR-LOAD"]["function_start"], "0x31da0")
        self.assertEqual(evidence["ANIM-FIGHT-LOAD"]["function_start"], "0x3859e")
        self.assertNotIn("None", evidence["ENTRY-ZCOM-EXEC"]["pseudocode_summary"])

    @unittest.skipUnless(_EXTRACTION_ENABLED, "original data or idalib Python module not configured")
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
