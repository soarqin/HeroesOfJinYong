import json
import importlib.util
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


PROJECT_ROOT = Path(__file__).resolve().parents[2]
CLI = PROJECT_ROOT / "tools" / "reverse" / "extract_battle_evidence.py"
_ORIGINAL_DATA = bool(os.environ.get("HOJY_ORIGINAL_DATA_DIR"))
_IDA_EXTRACTION_ENABLED = _ORIGINAL_DATA and importlib.util.find_spec("idapro") is not None


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

    @unittest.skipUnless(_IDA_EXTRACTION_ENABLED,
                         "original data and IDA Python are not configured")
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
        self.assertEqual(evidence["BATTLE-MAIN-LOOP"]["address"], "0x31eb9")
        self.assertEqual(evidence["BATTLE-AI-SELECT"]["address"], "0x33599")
        self.assertEqual(evidence["BATTLE-AI-HP-RECOVERY"]["address"], "0x33c4d")
        self.assertEqual(evidence["BATTLE-AI-DEPOISON-RECOVERY"]["address"], "0x33e93")
        self.assertEqual(evidence["BATTLE-AI-MEDIC-TARGET"]["address"], "0x341f6")
        self.assertEqual(evidence["BATTLE-AI-DEPOISON-TARGET"]["address"], "0x343da")
        self.assertEqual(evidence["BATTLE-AI-FOLLOWUP"]["address"], "0x34550")
        self.assertEqual(evidence["BATTLE-AI-RANDOM-SKILL"]["address"], "0x34c47")
        self.assertEqual(evidence["BATTLE-AI-RETREAT"]["address"], "0x34aec")
        self.assertEqual(evidence["BATTLE-AI-TARGET-STRATEGY"]["address"], "0x3505b")
        self.assertEqual(evidence["BATTLE-ACT-DAMAGE"]["address"], "0x3598c")
        self.assertEqual(evidence["BATTLE-AI-MEDIC-ACTION"]["address"], "0x36210")
        self.assertEqual(evidence["BATTLE-AI-DEPOISON-ACTION"]["address"], "0x363ac")
        self.assertEqual(evidence["BATTLE-AI-MOVE-TO-RANGE"]["address"], "0x3650e")
        self.assertEqual(evidence["BATTLE-SKILL-SELECT"]["address"], "0x37734")
        self.assertEqual(evidence["BATTLE-SKILL-DAMAGE"]["address"], "0x39188")
        self.assertEqual(evidence["BATTLE-ACT-POISON"]["address"], "0x39a45")
        self.assertEqual(evidence["BATTLE-ACT-DEPOISON"]["address"], "0x39da3")
        self.assertEqual(evidence["BATTLE-ACT-MEDIC"]["address"], "0x3a10c")
        self.assertEqual(evidence["BATTLE-ACT-THROW"]["address"], "0x3a30b")
        self.assertEqual(evidence["BATTLE-ACT-REST"]["address"], "0x3a8a4")
        self.assertEqual(evidence["BATTLE-ROUND-END"]["address"], "0x3c563")
        self.assertEqual(evidence["BATTLE-RANDOM-MOD"]["address"], "0x3d612")
        new_anchor_ids = (
            "BATTLE-AI-RANDOM-SKILL",
            "BATTLE-AI-TARGET-STRATEGY",
            "BATTLE-AI-RETREAT",
            "BATTLE-AI-MOVE-TO-RANGE",
            "BATTLE-ACT-POISON",
            "BATTLE-ACT-DEPOISON",
            "BATTLE-ACT-MEDIC",
            "BATTLE-ACT-THROW",
            "BATTLE-ACT-REST",
        )
        for evidence_id in new_anchor_ids:
            self.assertEqual(
                evidence[evidence_id]["function_start"],
                evidence[evidence_id]["address"],
            )
            self.assertTrue(evidence[evidence_id]["kind"])
        for record in evidence.values():
            self.assertEqual(record["pseudocode_summary"], [])

    @unittest.skipUnless(_ORIGINAL_DATA, "original data not configured")
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
