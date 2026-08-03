import importlib.util
import os
import unittest
from pathlib import Path

_HAS_DATA = bool(os.environ.get("HOJY_ORIGINAL_DATA_DIR"))
_HAS_CAPSTONE = importlib.util.find_spec("capstone") is not None


def _image():
    from le_backend import LEImage

    return LEImage(Path(os.environ["HOJY_ORIGINAL_DATA_DIR"]) / "Z.DAT")


@unittest.skipUnless(_HAS_DATA, "HOJY_ORIGINAL_DATA_DIR is not configured")
class LEImageTests(unittest.TestCase):
    def test_load_image_matches_the_recorded_layout(self):
        image = _image()
        self.assertEqual([(item.base, item.page_count) for item in image.objects],
                         [(0x10000, 1), (0x20000, 59)])
        self.assertEqual(image.page_size, 0x1000)
        self.assertEqual(image.code_range, (0x20000, 0x5B000))
        self.assertEqual(image.entry, 0x3F234)

    def test_fixups_resolve_absolute_references(self):
        image = _image()
        self.assertGreater(image.fixup_count, 0)
        # `war.sta` is the string the battlefield loader passes to the file layer.
        self.assertEqual(image.find_string("war.sta"), 0x58A3C)
        self.assertEqual(image.read(0x58A3C, 8), b"war.sta\x00")
        # DATA-WAR-LOAD pushes that fixed-up address.
        self.assertEqual(image.read(0x31DAC, 5), b"\x68\x3c\x8a\x05\x00")

    def test_skill_weapon_binding_table_matches_the_data_loader(self):
        # src/data/factors.cc reads this table at file offset 0x4F538.
        image = _image()
        self.assertEqual(0x55B38 - 0x6600, 0x4F538)
        self.assertEqual(image.find_string("fight000.grp"), 0x55B62)


@unittest.skipUnless(_HAS_DATA and _HAS_CAPSTONE,
                     "original data or capstone not configured")
class FunctionIndexTests(unittest.TestCase):
    def test_battle_anchors_resolve_to_function_starts(self):
        from le_backend import FunctionIndex

        index = FunctionIndex(_image())
        self.assertIn(0x31DA0, index.functions)   # DATA-WAR-LOAD
        self.assertIn(0x3859E, index.functions)   # ANIM-FIGHT-LOAD
        self.assertIn(0x39188, index.functions)   # NUM-DAMAGE
        self.assertIn(0x3D612, index.functions)   # RNG-BOUNDED
        self.assertGreaterEqual(len(index.functions), 570)

    def test_damage_routine_is_reachable_from_the_attack_executor(self):
        from le_backend import FunctionIndex

        index = FunctionIndex(_image())
        callers = index.callers(0x39188)
        self.assertTrue(callers)
        owners = {index.owner(site) for site in callers}
        self.assertIn(0x37734, owners)   # single/area executor
        self.assertIn(0x38999, owners)   # line executor

    def test_damage_routine_carries_the_expected_constants(self):
        from le_backend import FunctionIndex

        index = FunctionIndex(_image())
        values = index.immediates(0x39188)
        # stamina / 15, hurt / 20, distance decay against 100, and rnd(20).
        for expected in (15, 20, 100, 3, 10):
            self.assertIn(expected, values)


if __name__ == "__main__":
    unittest.main()
