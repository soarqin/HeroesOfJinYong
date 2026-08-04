import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from typing import List


MAKEDATA = Path(sys.argv[1]).resolve()

REQUIRED_DATA_FILES = (
    "ALLDEF.GRP", "ALLDEF.IDX", "ALLSIN.GRP", "ALLSIN.IDX",
    "BUILDING.002", "BUILDX.002", "BUILDY.002", "EARTH.002", "SURFACE.002",
    "CLOUD.GRP", "CLOUD.IDX", "DEAD.BIG", "EFT.GRP", "EFT.IDX",
    "ENDCOL.COL", "ENDWORD.GRP", "ENDWORD.IDX", "KEND.GRP", "KEND.IDX",
    "HDGRP.GRP", "HDGRP.IDX", "KDEF.GRP", "KDEF.IDX",
    "MMAP.COL", "MMAP.GRP", "MMAP.IDX", "RANGER.GRP", "RANGER.IDX",
    "TALK.GRP", "TALK.IDX", "TITLE.BIG", "TITLE.GRP", "TITLE.IDX",
    "WAR.STA", "WARFLD.GRP", "WARFLD.IDX", "Z.DAT",
)


def write_grp(directory: Path, index_name: str, group_name: str, entries: List[bytes]) -> None:
    offset = 0
    index = bytearray()
    group = bytearray()
    for entry in entries:
        offset += len(entry)
        index += struct.pack("<I", offset)
        group += entry
    (directory / index_name).write_bytes(index)
    (directory / group_name).write_bytes(group)


class MakeDataTests(unittest.TestCase):
    def test_generates_complete_target_layout(self) -> None:
        with tempfile.TemporaryDirectory(prefix="hojy-makedata-") as temporary:
            root = Path(temporary)
            source = root / "original"
            target = root / "target"
            font = root / "Noto Test.otf"
            source.mkdir()

            for name in REQUIRED_DATA_FILES:
                (source / name).write_bytes(name.encode("ascii"))
            (source / "ALLDEF.GRP").write_bytes(b"resource")
            (source / "FIGHT007.IDX").write_bytes(b"index")
            (source / "FIGHT007.GRP").write_bytes(b"group")
            (source / "IGNORED.TXT").write_bytes(b"ignored")
            write_grp(source, "SDX000", "SMP000", [b"A"])
            write_grp(source, "SDX001", "SMP001", [b"A", b"B"])
            write_grp(source, "WDX003", "WMP003", [b"C"])
            font.write_bytes(b"font")

            result = subprocess.run(
                [str(MAKEDATA), str(source), str(target), str(font)],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)

            data = target / "data"
            self.assertEqual((data / "ALLDEF.GRP").read_bytes(), b"resource")
            self.assertEqual((data / "FIGHT007.IDX").read_bytes(), b"index")
            self.assertEqual((data / "FIGHT007.GRP").read_bytes(), b"group")
            self.assertFalse((data / "IGNORED.TXT").exists())
            self.assertFalse((data / "SDX000").exists())
            self.assertFalse((data / "SMP000").exists())
            self.assertEqual((data / "SDX").read_bytes(), struct.pack("<II", 1, 2))
            self.assertEqual((data / "SMP").read_bytes(), b"AB")
            self.assertEqual((data / "WDX").read_bytes(), struct.pack("<I", 1))
            self.assertEqual((data / "WMP").read_bytes(), b"C")
            self.assertEqual((data / "font" / font.name).read_bytes(), b"font")
            self.assertEqual(
                (data / "strings.toml").read_text(encoding="utf-8"),
                (Path(__file__).parents[2] / "src" / "strings.toml").read_text(encoding="utf-8"),
            )

            config = (target / "config.toml").read_text(encoding="utf-8")
            self.assertIn('data_path = ["data"]', config)
            self.assertIn('music_path = "data"', config)
            self.assertIn('sound_path = "data"', config)
            self.assertIn('save_path = "data"', config)
            font_path = "data" + "/font/Noto Test.otf"
            self.assertIn(f'fonts = "{font_path}"', config)
            self.assertIn("[window]", config)

            (target / "keep.txt").write_text("keep", encoding="utf-8")
            rerun = subprocess.run(
                [str(MAKEDATA), str(source), str(target), str(font)],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertEqual(rerun.returncode, 0, rerun.stderr)
            self.assertEqual((target / "keep.txt").read_text(encoding="utf-8"), "keep")

    def test_rejects_corrupt_map_before_creating_target(self) -> None:
        with tempfile.TemporaryDirectory(prefix="hojy-makedata-bad-") as temporary:
            root = Path(temporary)
            source = root / "original"
            target = root / "target"
            font = root / "font.otf"
            source.mkdir()
            (source / "SDX000").write_bytes(b"bad")
            (source / "SMP000").write_bytes(b"map")
            font.write_bytes(b"font")

            result = subprocess.run(
                [str(MAKEDATA), str(source), str(target), str(font)],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("invalid IDX length", result.stderr)
            self.assertFalse(target.exists())


if __name__ == "__main__":
    sys.argv = [sys.argv[0]]
    unittest.main()
