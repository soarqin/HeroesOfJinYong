import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


class EventVmBoundaryTests(unittest.TestCase):
    def test_battle_command_validates_owner_and_event_session(self):
        runtime = (ROOT / "src/scene/map_event_runtime.cc").read_text(
            encoding="utf-8"
        )
        execute = runtime.index("MapWithEvent::executeLegacy")
        battle_start = runtime.index("case 6:", execute)
        battle = runtime[battle_start:runtime.index("case 50:", battle_start)]
        self.assertIn("const auto eventSession = eventSessionToken();", battle)
        self.assertIn("postOwnedSceneCommand", battle)
        self.assertGreaterEqual(battle.count("isCurrentEventSession(eventSession)"), 2)

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
        navigation = (ROOT / "src/scene/map_logic_movement.cc").read_text(
            encoding="utf-8"
        )
        self.assertIn("eventVm_.legacyActive()", navigation)
        self.assertIn("!eventVm_.legacyWaiting()", navigation)
        self.assertIn("continueEvents(false)", navigation)

    def test_extended_menu_waits_for_an_explicit_continuation(self):
        extended = (ROOT / "src/scene/map_event_extended.cc").read_text(
            encoding="utf-8"
        )
        menu_block = extended[extended.index("case 39:"):extended.index("case 41:")]
        self.assertIn("return waiting();", menu_block)
        self.assertNotIn("return completed();", menu_block)
        self.assertIn("continuation", menu_block.lower())

    def test_extended_menu_is_created_through_scene_command_boundary(self):
        extended = (ROOT / "src/scene/map_event_extended.cc").read_text(
            encoding="utf-8"
        )
        menu_block = extended[extended.index("case 39:"):extended.index("case 41:")]
        self.assertNotIn("new MenuTextList", menu_block)
        self.assertIn("postSceneCommand", menu_block)

    def test_ui_callbacks_cannot_write_vm_memory_or_resume_directly(self):
        extended = (ROOT / "src/scene/map_event_extended.cc").read_text(
            encoding="utf-8"
        )
        async_block = extended[extended.index("case 35:"):extended.index("case 38:")]
        self.assertNotIn("eventVm_.memory()", async_block)
        self.assertNotIn("continueEvents(false)", async_block)

    def test_event_menu_result_has_typed_command_contract(self):
        command_header = (ROOT / "src/scene/logic/command.hh").read_text(
            encoding="utf-8"
        )
        self.assertRegex(command_header, r"EventMenu|Menu.*Continuation|continuation")

    def test_loading_a_new_event_invalidates_old_presentation_results(self):
        runtime = (ROOT / "src/scene/map_event_runtime.cc").read_text(
            encoding="utf-8"
        )
        start = runtime.index("void MapWithEvent::runEvent")
        body = runtime[start:runtime.index("void MapWithEvent::onUseItem", start)]
        self.assertIn("activeEventContinuationToken_ = 0", body)
        self.assertLess(
            body.index("activeEventContinuationToken_ = 0"),
            body.index("eventVm_.loadLegacy"),
        )

    def test_event_overlay_is_owned_by_window_presentation_adapter(self):
        header = (ROOT / "src/scene/mapwithevent.hh").read_text(encoding="utf-8")
        overlay = (ROOT / "src/scene/window_event_overlay.cc").read_text(
            encoding="utf-8"
        )
        self.assertNotIn("ExtendedNode", header)
        for method in (
            "Window::showEventOverlay",
            "Window::clearEventPresentation",
            "Window::fadeEventIn",
            "Window::fadeEventOut",
            "Window::detachEventOverlay",
        ):
            self.assertIn(method, overlay)
        self.assertIn("isCurrentEventSession", overlay)
        self.assertIn("isCurrentTransition", overlay)
        self.assertIn("transformEventPoint", overlay)

    def test_legacy_fades_cross_the_typed_presentation_boundary(self):
        interaction = (ROOT / "src/scene/map_event_interaction.cc").read_text(
            encoding="utf-8"
        )
        bright = interaction[
            interaction.index("bool MapWithEvent::makeBright"):
            interaction.index("bool MapWithEvent::makeDim")
        ]
        dim = interaction[
            interaction.index("bool MapWithEvent::makeDim"):
            interaction.index("bool MapWithEvent::die")
        ]
        self.assertIn("fadeEventIn", bright)
        self.assertNotIn("map->fadeIn", bright)
        self.assertIn("fadeEventOut", dim)
        self.assertNotIn("map->fadeOut", dim)

    def test_event_cleanup_does_not_reach_into_node_fade_state(self):
        runtime = (ROOT / "src/scene/map_event_runtime.cc").read_text(
            encoding="utf-8"
        )
        cleanup = runtime[
            runtime.index("void MapWithEvent::cleanupEvents"):
            runtime.index("std::uint64_t MapWithEvent::beginEventContinuation")
        ]
        self.assertIn("clearEventPresentation", cleanup)
        self.assertNotIn("fadeNode_", cleanup)
        self.assertNotIn("fadePostAction_", cleanup)

    def test_window_event_cleanup_uses_typed_fade_request(self):
        overlay = (ROOT / "src/scene/window_event_overlay.cc").read_text(
            encoding="utf-8"
        )
        start = overlay.index("void Window::clearEventPresentation")
        end = overlay.index("void Window::fadeEventIn", start)
        cleanup = overlay[start:end]
        self.assertIn("requestFadeCleanup", cleanup)
        for token in ("fadeNode_", "fadePostAction_", "runFadePostAction_"):
            self.assertNotIn(token, cleanup)

    def test_window_battle_invalidation_only_requests_presentation_cleanup(self):
        window = (ROOT / "src/scene/window.cc").read_text(encoding="utf-8")
        start = window.index("void Window::invalidateBattleSession")
        end = window.index("bool Window::activateBattleSession", start)
        invalidation = window[start:end]
        self.assertIn("abortPresentationState", invalidation)
        self.assertNotIn("removeAllChildren", invalidation)


if __name__ == "__main__":
    unittest.main()
