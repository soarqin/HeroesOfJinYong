import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
MAX_IMPLEMENTATION_LINES = 600


class SourceLayoutTests(unittest.TestCase):
    def test_scene_implementation_files_stay_within_size_budget(self):
        scene_root = ROOT / "src" / "scene"
        oversized = {}
        for path in sorted(scene_root.rglob("*.cc")):
            line_count = len(path.read_text(encoding="utf-8").splitlines())
            if line_count > MAX_IMPLEMENTATION_LINES:
                oversized[str(path.relative_to(ROOT))] = line_count
        self.assertEqual(oversized, {})


if __name__ == "__main__":
    unittest.main()
