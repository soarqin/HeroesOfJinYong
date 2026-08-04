import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]


def function_body(path: str, signature: str) -> str:
    text = (ROOT / path).read_text(encoding="utf-8")
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for index in range(brace, len(text)):
        if text[index] == "{":
            depth += 1
        elif text[index] == "}":
            depth -= 1
            if depth == 0:
                return text[brace + 1:index]
    raise AssertionError(f"unterminated function {signature}")


class WarfieldBattleBoundaryTests(unittest.TestCase):
    def test_warfield_owns_transactional_battle_session(self):
        header = (ROOT / "src/scene/warfield.hh").read_text(encoding="utf-8")
        self.assertIn('#include "battle/engine.hh"', header)
        self.assertIn("battle::BattleEngine battleEngine_", header)
        self.assertIn("std::unique_ptr<battle::BattleParticipant>", header)

    def test_put_chars_and_end_check_use_engine_boundary(self):
        header = (ROOT / "src/scene/warfield.hh").read_text(encoding="utf-8")
        self.assertIn("bool putChars(", header)
        load_body = function_body("src/scene/warfield_load.cc", "bool Warfield::putChars")
        self.assertIn("std::vector<CharInfo> nextChars", load_body)
        self.assertIn("chars_ = std::move(nextChars)", load_body)
        self.assertIn("battleEngine_.begin", load_body)
        turns = function_body("src/scene/warfield_turns.cc", "bool Warfield::checkWarEnd")
        self.assertIn("battleEngine_.reconcile(battleInventorySnapshot())", turns)
        results = function_body("src/scene/warfield_results.cc", "void Warfield::endWar")
        self.assertIn("battleEngine_.finish(true)", results)

    def test_cleanup_rolls_back_before_destroying_character_storage(self):
        body = function_body("src/scene/warfield_load.cc", "void Warfield::cleanup")
        self.assertIn("discardBattleSession()", body)
        self.assertLess(body.index("discardBattleSession()"), body.index("chars_.clear()"))

    def test_window_does_not_fade_in_after_failed_character_setup(self):
        text = (ROOT / "src/scene/window.cc").read_text(encoding="utf-8")
        callback = text[text.index("const auto selectedChars"):text.index("}, []() -> bool", text.index("const auto selectedChars"))]
        self.assertIn("!wf->putChars(selectedChars)", callback)
        self.assertLess(callback.index("!wf->putChars(selectedChars)"), callback.index("map_->fadeIn()"))

    def test_legacy_single_target_step_is_not_double_applied_to_make_damage(self):
        body = function_body("src/scene/warfield_actions.cc", "void Warfield::makeDamage")
        self.assertNotIn("battleEngine_.step", body)

    def test_battle_inventory_mutations_use_session_working_copy(self):
        header = (ROOT / "src/scene/warfield.hh").read_text(encoding="utf-8")
        self.assertIn("Bag battleBag_", header)
        for path in (
            "src/scene/warfield_actions.cc",
            "src/scene/warfield_ai.cc",
            "src/scene/warfield_ai_skill.cc",
            "src/scene/warfield_results.cc",
        ):
            text = (ROOT / path).read_text(encoding="utf-8")
            self.assertNotIn("gBag", text, path)
        results = function_body("src/scene/warfield_results.cc", "void Warfield::endWar")
        self.assertIn("commitBattleBag()", results)

    def test_warfield_uses_one_persistent_recording_random_source(self):
        header = (ROOT / "src/scene/warfield.hh").read_text(encoding="utf-8")
        self.assertIn("battle::GameRandom battleGameRandom_", header)
        self.assertIn("battle::RecordingRandom battleRandom_", header)

        load = (ROOT / "src/scene/warfield_load.cc").read_text(encoding="utf-8")
        self.assertIn("battleRandom_.clear()", load)
        self.assertIn("&battleRandom_", load)

        ai = (ROOT / "src/scene/warfield_ai.cc").read_text(encoding="utf-8")
        self.assertNotIn("battle::GameRandom resourceRandom", ai)
        self.assertIn("battleRandom_", ai)

        actions = (ROOT / "src/scene/warfield_actions.cc").read_text(encoding="utf-8")
        for call in ("actDepoison", "actMedic", "actThrow", "actDamage", "postDamage"):
            self.assertIn(f"{call}(", actions)
        self.assertIn("battleRandom_", actions)

        turns = (ROOT / "src/scene/warfield_turns.cc").read_text(encoding="utf-8")
        self.assertIn("actRest", turns)
        self.assertIn("battleRandom_", turns)

        results = (ROOT / "src/scene/warfield_results.cc").read_text(encoding="utf-8")
        self.assertNotIn("util::gRandom", results)
        self.assertIn("actLevelup", results)
        self.assertIn("battleRandom_", results)

    def test_warfield_records_actions_at_execution_boundaries(self):
        header = (ROOT / "src/scene/warfield.hh").read_text(encoding="utf-8")
        self.assertIn("recordBattleAction(", header)
        self.assertIn("actionTargets_", header)

        actions = (ROOT / "src/scene/warfield_actions.cc").read_text(encoding="utf-8")
        self.assertIn("battle::SkillAction", actions)
        self.assertIn("battle::TechniqueAction", actions)
        self.assertIn("battle::ThrowAction", actions)
        self.assertIn("recordBattleAction", actions)

        turns = (ROOT / "src/scene/warfield_turns.cc").read_text(encoding="utf-8")
        self.assertIn("battle::MoveAction", turns)
        self.assertIn("battle::RestAction", turns)
        self.assertIn("battle::RoundEndAction", turns)
        self.assertIn("recordBattleAction", turns)

        ai = (ROOT / "src/scene/warfield_ai.cc").read_text(encoding="utf-8")
        self.assertIn("battle::ItemAction", ai)
        self.assertIn("recordBattleAction", ai)

    def test_settlement_randomness_is_inside_engine_finish_boundary(self):
        results = function_body("src/scene/warfield_results.cc", "void Warfield::endWar")
        self.assertIn("battleEngine_.reconcile(battleInventorySnapshot())", results)
        self.assertLess(
            results.index("actLevelup"),
            results.rindex("battleEngine_.finish(true)"),
        )
        self.assertLess(
            results.index("battleRandom_.next"),
            results.rindex("battleEngine_.finish(true)"),
        )


if __name__ == "__main__":
    unittest.main()
