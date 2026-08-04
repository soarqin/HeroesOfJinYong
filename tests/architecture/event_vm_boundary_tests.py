import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class EventVmBoundaryTests(unittest.TestCase):
    def test_map_uses_vm_owned_legacy_program_state(self):
        header = (ROOT / "src/scene/mapwithevent.hh").read_text(encoding="utf-8")
        self.assertIn("public event::LegacyVmHost", header)
        self.assertNotIn("currEventIndex_", header)
        self.assertNotIn("currEventSize_", header)
        self.assertNotIn("currEventAdvTrue_", header)
        self.assertNotIn("currEventAdvFalse_", header)
        self.assertNotIn("currEventList_", header)

    def test_ordinary_dispatch_runs_through_pauseable_vm(self):
        runtime = (ROOT / "src/scene/map_event_runtime.cc").read_text(
            encoding="utf-8"
        )
        self.assertNotIn("runFunc", runtime)
        self.assertIn("MapWithEvent::decodeLegacy", runtime)
        self.assertIn("MapWithEvent::executeLegacy", runtime)
        self.assertIn("eventVm_.runLegacy", runtime)
        self.assertIn("eventVm_.resumeLegacy", runtime)

    def test_extended_self_modification_uses_vm_relative_patch(self):
        extended = (ROOT / "src/scene/map_event_extended.cc").read_text(
            encoding="utf-8"
        )
        self.assertIn("eventVm_.patchLegacyRelative", extended)
        self.assertNotIn("currEventList_", extended)

    def test_fixed_update_resumes_budget_yielded_event_programs(self):
        navigation = (ROOT / "src/scene/map_navigation.cc").read_text(
            encoding="utf-8"
        )
        self.assertIn("eventVm_.legacyActive()", navigation)
        self.assertIn("!eventVm_.legacyWaiting()", navigation)
        self.assertIn("continueEvents(false)", navigation)


if __name__ == "__main__":
    unittest.main()
