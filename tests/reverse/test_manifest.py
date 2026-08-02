import json
import tempfile
import unittest
from pathlib import Path

from manifest import BinaryRecord, DependencyRecord, EvidenceRecord, Manifest, sha256_file


class ManifestTests(unittest.TestCase):
    def test_sha256_file_returns_lowercase_digest(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "sample.bin"
            path.write_bytes(b"abc")

            self.assertEqual(
                sha256_file(path),
                "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad",
            )

    def test_json_order_is_stable(self):
        manifest = Manifest(source_root="D:/DOS/LEGEND")
        manifest.add_binary(BinaryRecord("Z.DAT", 343217, "b" * 64))
        manifest.add_binary(BinaryRecord("Z.COM", 413, "a" * 64))
        manifest.add_dependency(DependencyRecord("Z.COM", "Z.DAT", "process-exec", "0x10195"))
        manifest.add_evidence(EvidenceRecord(
            "ENTRY-ZCOM-EXEC", "Z.COM", "0x10195", "process-exec",
            "Executes Z.DAT", "confirmed"))
        manifest.add_evidence(EvidenceRecord(
            "DATA-WAR-LOAD", "Z.DAT", "0x31DA0", "function",
            "Loads WAR.STA", "confirmed"))

        data = json.loads(manifest.to_json())

        self.assertEqual([item["path"] for item in data["binaries"]], ["Z.COM", "Z.DAT"])
        self.assertEqual(
            [item["evidence_id"] for item in data["evidence"]],
            ["DATA-WAR-LOAD", "ENTRY-ZCOM-EXEC"],
        )

    def test_duplicate_evidence_is_rejected(self):
        manifest = Manifest(source_root="D:/DOS/LEGEND")
        evidence = EvidenceRecord(
            "ENTRY-ZCOM-EXEC", "Z.COM", "0x10195", "process-exec",
            "Executes Z.DAT", "confirmed")
        manifest.add_evidence(evidence)

        with self.assertRaisesRegex(ValueError, "duplicate evidence id"):
            manifest.add_evidence(evidence)


if __name__ == "__main__":
    unittest.main()
