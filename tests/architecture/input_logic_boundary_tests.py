"""Static contracts for the platform-input to fixed-logic boundary."""

from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]


def source(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", "", text)


def function_body(path: str, signature: str) -> str:
    text = source(path)
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
    raise AssertionError(f"unterminated function {signature} in {path}")


class InputLogicBoundaryTests(unittest.TestCase):
    def test_window_dispatch_input_is_queue_only(self) -> None:
        body = strip_comments(function_body("src/scene/window_input.cc", "void Window::dispatchInput"))
        self.assertIn("sampledInputEvents_.push_back(event)", body)
        forbidden = (
            "makeIntent", "inputPort_", "deliverNext", "consume", "SDL_",
            "gWindow", "gSaveData", "gBag", "Renderer", "Texture",
            "defer", "requestDelete", "delete ", "new ",
        )
        for token in forbidden:
            self.assertNotIn(token, body, token)

    def test_scene_window_input_translation_has_no_platform_or_domain_side_effects(self) -> None:
        text = strip_comments(source("src/scene/window_input.cc"))
        for token in (
            "#include <SDL", "SDL_", "gWindow", "gSaveData", "gBag",
            "Renderer", "Texture", "Window::beginInput", "Window::setInputRect",
            "Window::endInput",
        ):
            self.assertNotIn(token, text, token)

    def test_platform_text_input_is_not_owned_by_scene_nodes(self) -> None:
        title = strip_comments(source("src/scene/title.cc"))
        for token in ("Window::beginInput", "Window::setInputRect", "Window::endInput"):
            self.assertNotIn(token, title, token)

        window_header = strip_comments(source("src/scene/window.hh"))
        for token in ("static void beginInput", "static void setInputRect", "static void endInput"):
            self.assertNotIn(token, window_header, token)

    def test_title_input_is_cached_and_logic_uses_polymorphic_mode(self) -> None:
        title_input = strip_comments(source("src/scene/title_input.cc"))
        title_logic = strip_comments(source("src/scene/title_logic.cc"))
        self.assertIn("pendingInput_", title_input)
        for token in ("gSaveData", "gBag", "Renderer", "Texture", "prepareNewGame", "postCommand"):
            self.assertNotIn(token, title_input, token)
        self.assertIn("inputMode_->keyAction", title_logic)
        self.assertIn("inputMode_->textAction", title_logic)
        self.assertNotIn("switch (mode_)", title_logic)

    def test_scene_input_implementations_cannot_reach_world_or_render_services(self) -> None:
        input_files = sorted((ROOT / "src" / "scene").rglob("*_input.cc"))
        self.assertTrue(input_files)
        forbidden = (
            "#include <SDL", "#include \"SDL", "SDL_", "gWindow", "gSaveData",
            "gBag", "Renderer", "Texture", "renderer_", "ttf()", "defer(",
            "applyDeferred", "requestDelete", "delete ",
        )
        for path in input_files:
            text = strip_comments(path.read_text(encoding="utf-8"))
            for token in forbidden:
                self.assertNotIn(token, text, f"{token} in {path.relative_to(ROOT)}")

    def test_input_adapters_do_not_own_movement_or_battle_state_transitions(self) -> None:
        warfield_input = strip_comments(source("src/scene/warfield_input.cc"))
        for method in (
            "Warfield::setStage", "Warfield::moveCursor", "Warfield::confirmMove", "Warfield::confirmAttack",
            "Warfield::cancelSelection", "Warfield::cancelAutoControl",
        ):
            self.assertNotIn(method, warfield_input, method)

        navigation = function_body(
            "src/scene/map_input.cc", "void MapWithEvent::consumeKeyIntent"
        )
        for token in ("move(", "setPosition(", "gWindow->", "enterSubMap", "showMainMenu"):
            self.assertNotIn(token, navigation, token)

        for path in sorted((ROOT / "src" / "scene").glob("*_input.cc")):
            text = strip_comments(path.read_text(encoding="utf-8"))
            for token in ("move(", "setPosition(", "tryMove(", "doInteract(", "checkEvent("):
                self.assertNotIn(token, text, f"{token} in {path.relative_to(ROOT)}")

    def test_scene_logic_input_adapter_is_renderer_free(self) -> None:
        text = strip_comments(source("src/scene/logic/input.cc"))
        for token in (
            "SDL", "Renderer", "Texture", "gWindow", "gSaveData", "gBag",
            "new ", "delete ", "requestDelete", "defer(",
        ):
            self.assertNotIn(token, text, token)

    def test_application_input_services_do_not_depend_on_scene_or_world_state(self) -> None:
        paths = sorted((ROOT / "src" / "app").glob("*input*.cc"))
        self.assertTrue(paths)
        for path in paths:
            text = strip_comments(path.read_text(encoding="utf-8"))
            for token in (
                "gWindow", "gSaveData", "gBag", "Renderer", "Texture",
                "SceneCommand", "namespace hojy::scene", "requestDelete", "defer(",
            ):
                self.assertNotIn(token, text, f"{token} in {path.relative_to(ROOT)}")

    def test_sdl_collector_is_the_only_scene_independent_event_reader(self) -> None:
        scene_root = ROOT / "src" / "scene"
        for path in sorted(scene_root.rglob("*.cc")):
            text = strip_comments(path.read_text(encoding="utf-8"))
            self.assertNotIn("SDL_PollEvent", text, str(path.relative_to(ROOT)))
        collector = strip_comments(source("src/app/sdl_input.cc"))
        self.assertIn("SDL_PollEvent", collector)

    def test_legacy_direct_input_handlers_are_removed(self) -> None:
        forbidden = (
            "handleKeyInput", "handleTextInput", "doHandleKeyInput",
            "dispatchKeyInput", "dispatchTextInput",
        )
        for path in sorted((ROOT / "src").rglob("*.cc")) + sorted((ROOT / "src").rglob("*.hh")):
            text = strip_comments(path.read_text(encoding="utf-8"))
            for token in forbidden:
                self.assertNotIn(token, text, f"{token} in {path.relative_to(ROOT)}")

    def test_input_contract_is_polymorphic_and_value_based(self) -> None:
        header = strip_comments(source("src/scene/logic/input.hh"))
        for declaration in (
            "class InputPort", "virtual ~InputPort", "virtual void enqueue",
            "class SceneInputIntent", "virtual ~SceneInputIntent",
            "virtual void deliver", "class InputConsumer", "virtual void consume",
            "class QueuedInputPort",
        ):
            self.assertIn(declaration, header, declaration)
        self.assertNotIn("SDL", header)
        self.assertNotIn("Renderer", header)
        self.assertNotIn("Texture", header)

    def test_popup_input_is_immediate_and_map_input_remains_fixed(self) -> None:
        immediate = strip_comments(function_body(
            "src/scene/window_interaction.cc", "void Window::updateInput"))
        self.assertIn("auto *target = popup_", immediate)
        self.assertIn("target->dispatchInputLogic()", immediate)
        self.assertIn("pendingInputEvents_.push_back", immediate)
        self.assertNotIn("map_->dispatchInputLogic", immediate)

        fixed = strip_comments(function_body(
            "src/scene/window.cc", "void Window::updateFixed"))
        self.assertIn("while (!pendingInputEvents_.empty() && !quitRequested_)", fixed)
        self.assertIn("auto *target = popup_ ? popup_ : map_", fixed)

    def test_quit_is_a_terminal_input_barrier(self) -> None:
        body = function_body("src/scene/window_interaction.cc", "void Window::updateInput")
        self.assertIn("while (!sampledInputEvents_.empty() && !quitRequested_)", body)
        quit_branch = body[body.index("InputAction::Quit"):]
        self.assertRegex(quit_branch, r"\b(?:break|clear)\s*\(")
        application = strip_comments(source("src/app/application.cc"))
        run = function_body("src/app/application.cc", "int Application::run")
        self.assertIn("if (window_.quitRequested()) { break; }", run)

    def test_input_consumers_only_record_intents(self) -> None:
        signatures = []
        for path in sorted((ROOT / "src" / "scene").rglob("*.cc")):
            text = strip_comments(path.read_text(encoding="utf-8"))
            for match in re.finditer(r"void\s+([A-Za-z_]\w*::consume(?:Key|Text)Intent)\s*\([^)]*\)\s*\{", text):
                brace = match.end() - 1
                depth = 0
                end = None
                for index in range(brace, len(text)):
                    if text[index] == "{":
                        depth += 1
                    elif text[index] == "}":
                        depth -= 1
                        if depth == 0:
                            end = index
                            break
                self.assertIsNotNone(end, f"unterminated {match.group(1)}")
                body = text[brace + 1:end]
                signatures.append(match.group(1))
                for token in (
                    "requestDelete(", "setDirty(", "postCommand(", "defer(", "new ", "delete ",
                    "renderer_->", "gSaveData", "gBag", "selectionController_->", "move(",
                    "doInteract(", "setStage(", "actMedic(", "actDepoison(", "useItem(",
                ):
                    self.assertNotIn(token, body, f"{token} in {match.group(1)} ({path.relative_to(ROOT)})")
        self.assertTrue(signatures)

    def test_map_input_is_polymorphic_and_pause_is_logic_only(self) -> None:
        input_text = source("src/scene/map_input.cc")
        contract = source("src/scene/logic/map_input.hh")
        logic_text = source("src/scene/map_logic.cc")
        header = source("src/scene/mapwithevent.hh")
        self.assertIn("MapInputAction", contract)
        self.assertIn("execute", logic_text)
        self.assertNotIn("PendingInputAction", input_text)
        self.assertNotIn("PendingInputAction", logic_text)
        self.assertNotIn("switch (action)", logic_text)
        self.assertNotIn("currEventPaused_", input_text)
        self.assertNotIn("consumeKeyIntent(Key key)", source("src/scene/submap.hh"))
        self.assertNotIn("PendingInputAction", header)

    def test_map_input_consumer_only_caches_raw_key(self) -> None:
        body = function_body(
            "src/scene/map_input.cc", "void MapWithEvent::consumeKeyIntent"
        )
        self.assertIn("pendingInputKey_", body)
        self.assertNotIn("translate(", body)
        self.assertNotIn("make_unique", body)

    def test_warfield_input_is_action_object_based_after_mode_translation(self) -> None:
        mode_header = source("src/scene/logic/warfield_input_mode.hh")
        logic = source("src/scene/warfield_input_logic.cc")
        input_logic = function_body(
            "src/scene/warfield_input_logic.cc", "void Warfield::applyInputLogic"
        )
        warfield_header = source("src/scene/warfield.hh")
        self.assertIn("class WarfieldInputAction", mode_header)
        self.assertIn("std::unique_ptr<WarfieldInputAction>", warfield_header)
        self.assertIn("action->execute", input_logic)
        self.assertNotIn("PendingInputAction", warfield_header)
        self.assertNotIn("switch (action)", input_logic)

    def test_menu_widgets_emit_typed_selections_without_business_callbacks(self) -> None:
        contract = strip_comments(source("src/scene/logic/menu.hh"))
        widget = strip_comments(source("src/scene/menu.hh"))
        menu_source = strip_comments(source("src/scene/window_menu.cc"))
        controller = menu_source[menu_source.index("void Window::showMainMenu"):]
        # The menu implementation is split into focused translation units;
        # bound the function at the next declaration instead of relying on a
        # removed legacy helper.
        next_declaration = controller.find("bool Window::runShop")
        if next_declaration < 0:
            next_declaration = controller.find("void Window::popupMessageBox")
        self.assertGreaterEqual(next_declaration, 0)
        controller = controller[:next_declaration]
        for declaration in (
            "enum class MenuGesture", "struct MenuSelection",
            "class MenuSelectionSink", "class ActionMenuController",
            "class MenuInputMode", "virtual void submit",
        ):
            self.assertIn(declaration, contract, declaration)
        self.assertNotIn("std::function", widget)
        self.assertNotIn("setHandler", widget)
        self.assertNotIn("switch (", controller)
        self.assertNotIn("currIndex()", controller)

    def test_extended_overlay_uses_typed_input_and_texture_ports(self) -> None:
        widget = strip_comments(source("src/scene/extendednode.hh"))
        overlay = strip_comments(source("src/scene/window_event_overlay.cc"))
        self.assertIn("ExtendedInputCompletionSink", widget)
        self.assertIn("ExtendedTextureProvider", widget)
        self.assertNotIn("setHandler", widget)
        self.assertNotIn("std::function", widget)
        self.assertNotIn("setTextureProvider", overlay)
        self.assertNotIn("switch (kind)", overlay)
        self.assertIn("setInputCompletionSink", overlay)

    def test_message_box_reports_typed_results_without_function_handlers(self) -> None:
        widget = strip_comments(source("src/scene/messagebox.hh"))
        implementation = strip_comments(source("src/scene/messagebox.cc"))
        self.assertIn("class MessageBoxResultSink", widget)
        self.assertIn("setResultSink", widget)
        self.assertNotIn("std::function", widget)
        self.assertIn("MessageBoxResult", implementation)
        self.assertNotIn("closeHandler_", implementation)
        self.assertNotIn("yesHandler_", implementation)
        self.assertNotIn("noHandler_", implementation)

    def test_direction_selection_reports_a_typed_result_sink(self) -> None:
        presentation = strip_comments(source("src/scene/warfield_presentation.cc"))
        self.assertIn("class DirectionSelectionResultSink", presentation)
        self.assertIn("setDirectionResultSink", presentation)
        self.assertNotIn("setDirectionHandler", presentation)
        self.assertNotIn("directionHandler_", presentation)
        self.assertNotIn("std::function<void(Map::Direction)>", presentation)


if __name__ == "__main__":
    unittest.main()
