"""Contracts for invalidating stale scene-transition callbacks."""

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def source(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


class TransitionLifecycleTests(unittest.TestCase):
    def test_submap_tips_callback_uses_weak_owner(self):
        transition = source("src/scene/window_transition.cc")
        start = transition.index("void Window::enterSubMap")
        body = transition[start:transition.index("bool Window::enterWar", start)]
        self.assertIn("tips->lifetimeHandle()", body)
        self.assertIn("tipsLifetime.lock()", body)
        self.assertNotIn("[this, token, expected, tips,", body)

    def test_battle_music_command_captures_session_token_by_value(self):
        transition = source("src/scene/window_transition.cc")
        start = transition.index("bool Window::enterWar")
        body = transition[start:transition.index("void Window::endWar", start)]
        self.assertIn("const auto presentationSessionToken = battleSessionToken_;", body)
        self.assertIn("[this, wf, presentationSessionToken]", body)
        self.assertIn("wf->presentationOwnerHandle(), presentationSessionToken", body)
        self.assertNotIn("wf->presentationOwnerHandle(), battleSessionToken_", body)

    def test_scene_replacement_invalidates_battle_sessions(self):
        transition = source("src/scene/window_transition.cc")
        persistence = source("src/scene/window_persistence.cc")
        for method in (
            "void Window::title",
            "void Window::endscreen",
            "void Window::exitToGlobalMap",
            "void Window::enterSubMap",
            "void Window::playerDie",
        ):
            start = transition.index(method)
            end = transition.find("\nvoid Window::", start + 6)
            body = transition[start:] if end < 0 else transition[start:end]
            self.assertIn("invalidateBattleSession()", body, method)
        for method in ("bool Window::startNewGame", "bool Window::loadGame"):
            start = persistence.index(method)
            end = persistence.find("\n", start)
            body = persistence[start:]
            self.assertIn("invalidateBattleSession()", body, method)

    def test_battle_terminal_paths_use_common_abort(self):
        load = source("src/scene/warfield_load.cc")
        results = source("src/scene/warfield_results.cc")
        self.assertIn("abortPresentationState()", load[load.index("bool Warfield::recordBattleAction"):])
        body = results[results.index("void Warfield::endWar"):]
        self.assertIn("abortPresentationState()", body)
        self.assertIn("void Warfield::abortPresentationState()", load)

    def test_engine_failure_posts_a_guarded_window_transition(self):
        load = source("src/scene/warfield_load.cc")
        transition = source("src/scene/window_transition.cc")
        self.assertIn("queueBattleAbortTransition()", load[load.index("bool Warfield::recordBattleAction"):])
        self.assertIn("context.abortBattle", load)
        self.assertIn("void Window::abortBattle", transition)
        self.assertIn("ownsBattleSession", transition)
        self.assertIn("map_ != warfield_", transition)

    def test_window_owns_transition_generation_and_popup_replacement_boundary(self):
        header = source("src/scene/window.hh")
        transition = source("src/scene/window_transition.cc")
        self.assertIn("transitionGeneration_", header)
        self.assertIn("beginTransition()", header)
        self.assertIn("isCurrentTransition", header)
        self.assertIn("replacePopup", header)
        for method in ("Window::title", "Window::endscreen", "Window::playerDie"):
            start = transition.index(method)
            body = transition[start:]
            self.assertIn("replacePopup", body, method)

    def test_map_fade_callbacks_validate_generation_and_source(self):
        transition = source("src/scene/window_transition.cc")
        persistence = source("src/scene/window_persistence.cc")
        combined = transition + persistence
        self.assertGreaterEqual(combined.count("isCurrentTransition"), 5)
        self.assertIn("map_ != source", combined)
        self.assertIn("map_ != expected", combined)

    def test_window_fade_callbacks_use_weak_window_lifetime(self):
        files = (
            "src/scene/window_event_overlay.cc",
            "src/scene/window_transition.cc",
            "src/scene/window_persistence.cc",
        )
        combined = "\n".join(source(path) for path in files)
        self.assertIn("WindowLifetimeHandle", source("src/scene/window.hh"))
        self.assertGreaterEqual(combined.count("windowLifetimeHandle"), 5)
        for path in files:
            text = source(path)
            for marker in ("->fadeIn([", "->fadeOut(["):
                position = 0
                while True:
                    position = text.find(marker, position)
                    if position < 0:
                        break
                    end = text.find("});", position)
                    self.assertNotEqual(end, -1, f"unterminated fade callback in {path}")
                    block = text[position:end]
                    self.assertNotIn("[this", block, f"raw Window capture in {path}")
                    self.assertIn("windowLifetimeHandle", block, path)
                    position = end + 3

    def test_fade_callbacks_queue_presentation_and_audio_work(self):
        transition = source("src/scene/window_transition.cc")
        enter_start = transition.index(
            "source->fadeOut([windowLifetimeHandle, token, source, subMap, subMapId,")
        enter_end = transition.index("    });\n}", enter_start)
        enter = transition[enter_start:enter_end]
        self.assertNotIn("new MessageBox", enter)
        self.assertNotIn("deferredCommands_", enter)
        self.assertIn("postCommand", enter)

        for signature in ("void Window::endWar(BattleEndRequest request)",
                          "void Window::abortBattle(BattleAbortRequest request)"):
            body = transition[transition.index(signature):]
            fade_start = body.index("source->fadeIn([")
            fade_end = body.index("    });", fade_start)
            fade = body[fade_start:fade_end]
            self.assertNotIn("->continueEvents", fade)
            self.assertNotIn("->playMusic", fade)
            self.assertIn("postCommand", fade)

    def test_persistence_transitions_and_save_are_guarded(self):
        persistence = source("src/scene/window_persistence.cc")
        self.assertIn("beginTransition()", persistence)
        self.assertIn("isCurrentTransition", persistence)
        self.assertIn("initSubMapId", persistence)
        self.assertIn("subMapInfo.size()", persistence)
        save_start = persistence.index("bool Window::saveGame")
        save_body = persistence[save_start:]
        self.assertIn("if (!ready_", save_body)
        self.assertIn("dynamic_cast<GlobalMap *>(globalMap_)", save_body)
        self.assertIn("dynamic_cast<MapWithEvent *>(map_)", save_body)

    def test_movement_event_paths_validate_serialized_pointers_and_ranges(self):
        movement = source("src/scene/map_logic_movement.cc")
        self.assertIn("subMapLayerInfo.size()", movement)
        self.assertIn("subMapEventInfo.size()", movement)
        self.assertIn("eventId < 0", movement)
        self.assertIn("animEventId_[i] >=", movement)
        self.assertIn("animEventId_[i] <", movement)

    def test_set_position_rolls_back_when_move_validation_rejects_candidate(self):
        movement = source("src/scene/map_logic_movement.cc")
        start = movement.index("void MapWithEvent::setPosition")
        body = movement[start:movement.index("void MapWithEvent::move", start)]
        self.assertIn("const auto oldX", body)
        self.assertIn("if (!moved)", body)
        self.assertIn("currX_ = oldX", body)
        self.assertIn("cameraX_ = oldCameraX", body)

    def test_submap_transition_explicitly_restores_standing_pose(self):
        transition = source("src/scene/window_transition.cc")
        start = transition.index("void Window::completeSubMapTransition")
        body = transition[start:transition.index("bool Window::enterWar", start)]
        self.assertEqual(body.count("resetMainCharStance()"), 2)
        self.assertLess(
            body.index("setPosition(request.x, request.y, false)"),
            body.index("resetMainCharStance()"),
        )
        callback_position = body.index("setPosition(x, y)")
        callback_stance = body.index("resetMainCharStance()", callback_position)
        self.assertLess(callback_position, callback_stance)

        movement = source("src/scene/map_logic_movement.cc")
        reset_start = movement.index("void MapWithEvent::resetMainCharStance")
        reset_body = movement[
            reset_start:movement.index("void MapWithEvent::move", reset_start)
        ]
        self.assertIn("currMainCharFrame_ = 0", reset_body)
        self.assertIn("updateMainCharSpriteId()", reset_body)


if __name__ == "__main__":
    unittest.main()
