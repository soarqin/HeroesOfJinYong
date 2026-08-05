"""Static contracts for logic-to-presentation requests."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]


def source(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", "", text)


class PresentationCommandBoundaryTests(unittest.TestCase):
    def test_battle_consumers_pin_their_concrete_presentation_stage(self) -> None:
        text = source("src/scene/window_presentation.cc")
        expected = {
            "showBattleDirectionSelection": "BattlePresentationStage::DirectionSelection",
            "showBattleSkillLevelUp": "BattlePresentationStage::SkillLevelUp",
            "showBattleItemResult": "BattlePresentationStage::ItemResult",
            "showBattleMenu": "BattlePresentationStage::PlayerMenu",
            "showBattleItemSelection": "BattlePresentationStage::ItemSelection",
            "showBattleStatusSelection": "BattlePresentationStage::StatusSelection",
            "showBattleFinishMessages": "BattlePresentationStage::FinishMessages",
        }
        for method, stage in expected.items():
            start = text.index(f"void Window::{method}")
            end = text.find("\nvoid ", start + 6)
            body = text[start:] if end < 0 else text[start:end]
            self.assertIn(stage, body, method)
            self.assertNotIn("request.expectedStage", body, method)

    def test_battle_presentation_callbacks_revalidate_owner_and_generation(self) -> None:
        text = strip_comments(source("src/scene/warfield_presentation.cc"))
        self.assertNotIn("[this", text)
        self.assertIn("postPresentationCommand", text)
        self.assertIn("matchesPresentationContext", text)
        self.assertNotIn("isCurrentPresentationSession(sessionToken)", text)

    def test_battle_logic_does_not_construct_or_draw_ui(self) -> None:
        text = strip_comments(source("src/scene/warfield_ui.cc"))
        for token in (
            "new MenuTextList", "new MessageBox", "new ItemView",
            "new StatusView", "new CharListMenu", "renderer_->",
        ):
            self.assertNotIn(token, text, token)

    def test_item_selection_policy_does_not_construct_ui_or_touch_live_bag(self) -> None:
        text = strip_comments(
            source("src/scene/item_selection_controller.cc")
            + source("src/scene/world_item_policy.cc")
        )
        for token in (
            "new MenuTextList", "new MessageBox", "new ItemView",
            "new StatusView", "new CharListMenu", "requestDelete(",
            "gBag", "gSaveData",
        ):
            self.assertNotIn(token, text, token)

    def test_item_and_character_views_only_consume_value_snapshots(self) -> None:
        item_view = strip_comments(source("src/scene/itemview.cc")
                                   + source("src/scene/itemview.hh"))
        for token in (
            "world/savedata.hh", "world/strings.hh", "gSaveData",
            "GETITEM", "GETCHAR", "GETSKILL", "compassPosition()",
        ):
            self.assertNotIn(token, item_view, token)

        character_view = strip_comments(source("src/scene/charlistmenu.cc")
                                        + source("src/scene/charlistmenu.hh"))
        for token in (
            "world/savedata.hh", "world/strings.hh", "gSaveData",
            "GETCHARNAME", "ValueType", "initWithTeamMembers",
        ):
            self.assertNotIn(token, character_view, token)

    def test_battle_presentation_adapter_does_not_requery_live_domain_state(self) -> None:
        text = strip_comments(source("src/scene/warfield_presentation.cc"))
        for token in (
            "world/savedata.hh", "gSaveData", "battleBag_",
            "currentActor_", "charQueue_", "chars_",
            "calcRealSkillLevel", "buildCharacterStatusSnapshot",
        ):
            self.assertNotIn(token, text, token)
        self.assertIn("request.entries", text)
        self.assertIn("request.items", text)
        self.assertIn("request.characters", text)

    def test_event_vm_logic_uses_typed_presentation_request(self) -> None:
        text = strip_comments(source("src/scene/map_event_extended.cc"))
        menu_block = text[text.index("case 39:"):text.index("case 41:")]
        self.assertNotIn("new MenuTextList", menu_block)
        self.assertIn("postSceneCommand", menu_block)
        self.assertIn("return waiting();", menu_block)

    def test_presentation_construction_is_confined_to_window_or_presentation_files(self) -> None:
        for path in sorted((ROOT / "src" / "scene").glob("*.cc")):
            if path.name in {
                "window.cc", "window_menu.cc", "window_transition.cc",
                "window_event.cc", "window_presentation.cc",
                "warfield_presentation.cc",
                # These are concrete view implementations.  They own their
                # child widgets; logic adapters only submit typed requests.
                "charlistmenu.cc", "messagebox.cc", "itemview.cc", "title.cc",
            }:
                continue
            text = strip_comments(path.read_text(encoding="utf-8"))
            self.assertNotRegex(
                text,
                r"new\s+(?:MenuTextList|MenuYesNo|MenuOption|MessageBox|ItemView|StatusView|CharListMenu)\b",
                path.name,
            )

    def test_item_yes_no_message_releases_its_popup_after_result(self) -> None:
        text = strip_comments(source("src/scene/window_presentation.cc"))
        block = text[text.index("if (type == MessageBox::YesNo)"):]
        self.assertIn("setResultSink", block)
        self.assertIn("result.accepted", block)
        self.assertGreaterEqual(block.count("messageBox->requestDelete()"), 1)


if __name__ == "__main__":
    unittest.main()
