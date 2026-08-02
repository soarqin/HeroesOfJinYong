import os
import importlib.util
from pathlib import Path
import unittest


@unittest.skipUnless(
    os.environ.get("HOJY_ORIGINAL_DATA_DIR") and importlib.util.find_spec("idapro"),
    "original data or idalib Python module not configured",
)
class IdaBackendTests(unittest.TestCase):
    def test_original_binaries_have_expected_battle_anchors(self):
        from ida_backend import IdaDatabase

        root = Path(os.environ["HOJY_ORIGINAL_DATA_DIR"])
        with IdaDatabase(root / "Z.COM") as zcom:
            self.assertEqual(zcom.processor_name(), "metapc")
            self.assertEqual(zcom.segment_count(), 1)
            self.assertEqual(zcom.function_count(), 3)

        with IdaDatabase(root / "Z.DAT") as zdat:
            self.assertEqual(zdat.processor_name(), "metapc")
            self.assertEqual(zdat.segment_count(), 2)
            self.assertGreaterEqual(zdat.function_count(), 570)
            war_sta = zdat.find_exact_string("war.sta")
            self.assertEqual(war_sta, 0x58A3C)
            refs = zdat.xrefs_to(war_sta)
            self.assertIn(0x31DAC, refs)
            function = zdat.containing_function(0x31DAC)
            self.assertEqual(function["start"], "0x31da0")


if __name__ == "__main__":
    unittest.main()
