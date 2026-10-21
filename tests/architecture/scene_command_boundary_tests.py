"""Static contracts for fixed-logic scene commands and their barrier."""

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


class SceneCommandBoundaryTests(unittest.TestCase):
    def test_owner_sensitive_commands_use_weak_node_lifetimes(self) -> None:
        node = strip_comments(source("src/scene/node.hh"))
        helper = strip_comments(source("src/scene/window_command.hh"))
        runtime = strip_comments(source("src/scene/map_event_runtime.cc"))
        title = strip_comments(source("src/scene/title_logic.cc"))

        self.assertIn("LifetimeHandle", node)
        self.assertIn("postOwnedSceneCommand", helper)

        execute = runtime.index("MapWithEvent::executeLegacy")
        battle_start = runtime.index("case 6:", execute)
        battle = runtime[battle_start:runtime.index("case 50:", battle_start)]
        self.assertIn("postOwnedSceneCommand", battle)
        self.assertNotIn("postSceneCommand(this, [this", battle)

        load_start = title.index("void Title::executeActivateLoadSelection")
        load = title[load_start:title.index("void Title::executeReturnToMainMenu", load_start)]
        self.assertIn("postOwnedSceneCommand", load)
        self.assertNotIn("postCommand([this", load)

    def test_scene_command_has_an_explicit_execution_context(self) -> None:
        header = strip_comments(source("src/scene/logic/command.hh"))
        context_sources = "\n".join(
            strip_comments(path.read_text(encoding="utf-8"))
            for path in sorted((ROOT / "src" / "scene" / "logic").rglob("*.hh"))
        )
        self.assertRegex(context_sources, r"\b(?:class|struct)\s+SceneCommandContext\b")
        self.assertRegex(header, r"execute\s*\(\s*SceneCommandContext\s*&")
        self.assertNotIn("std::function<void()>", header)
        self.assertNotRegex(
            header,
            r"push\s*\(\s*std::function\s*<\s*void\s*\(\s*\)\s*>",
        )

        command_implementations = "\n".join(
            strip_comments(path.read_text(encoding="utf-8"))
            for path in sorted((ROOT / "src" / "scene").rglob("*.hh"))
        ) + "\n" + "\n".join(
            strip_comments(path.read_text(encoding="utf-8"))
            for path in sorted((ROOT / "src" / "scene").rglob("*.cc"))
        )
        self.assertRegex(
            command_implementations,
            r"class\s+[A-Za-z_]\w*Command\b[^\{]*:\s*public\s+SceneCommand",
        )

    def test_window_owns_the_command_queue(self) -> None:
        header = strip_comments(source("src/scene/window.hh"))
        self.assertIn("SceneCommandQueue", header)
        self.assertRegex(header, r"(?:deferredCommands_|commandQueue_)\s*;")

    def test_each_command_barrier_executes_exactly_one_generation(self) -> None:
        header = strip_comments(source("src/scene/logic/command.hh"))
        implementation = strip_comments(source("src/scene/logic/command.cc"))
        barrier = strip_comments(function_body(
            "src/scene/window.cc", "void Window::applyDeferredCommands"))
        self.assertNotIn("executeAllGenerations", header)
        self.assertNotIn("executeAllGenerations", implementation)
        self.assertIn("executeGeneration", barrier)
        self.assertNotIn("executeAllGenerations", barrier)

    def test_defer_only_enqueues_and_never_executes_inline(self) -> None:
        text = strip_comments(source("src/scene/window.cc"))
        if "void Window::defer" not in text:
            return
        body = strip_comments(function_body("src/scene/window.cc", "void Window::defer"))
        self.assertRegex(body, r"(?:deferredCommands_|commandQueue_)\s*\.push")
        self.assertNotRegex(body, r"\bcommand\s*\(\s*\)")
        self.assertNotIn("execute", body)

    def test_fixed_logic_subtransactions_have_ordered_node_and_command_barriers(self) -> None:
        body = strip_comments(function_body("src/scene/window.cc", "void Window::updateFixed"))
        self.assertIn("applyDeferredNodes", body)
        self.assertIn("applyDeferredCommands", body)
        self.assertLess(body.index("applyDeferredNodes"), body.index("applyDeferredCommands"))
        self.assertGreaterEqual(body.count("applyDeferredNodes"), 2)
        self.assertEqual(body.count("applyDeferredNodes"), body.count("applyDeferredCommands"))

        compatibility = strip_comments(function_body(
            "src/scene/window.cc", "void Window::compatibilityUpdate"))
        self.assertIn("applyDeferredNodes", compatibility)
        self.assertIn("applyDeferredCommands", compatibility)
        self.assertLess(
            compatibility.index("applyDeferredNodes"),
            compatibility.index("applyDeferredCommands"),
        )

        render = strip_comments(function_body("src/scene/window.cc", "void Window::render() const"))
        prepare = strip_comments(function_body("src/scene/window.cc", "bool Window::prepareRender"))
        for phase_body in (prepare, render):
            for token in ("applyDeferredNodes", "applyDeferredCommands", "executeGeneration", "executeAllGenerations"):
                self.assertNotIn(token, phase_body, token)

    def test_each_input_intent_reselects_focus_after_its_barrier(self) -> None:
        body = strip_comments(function_body("src/scene/window.cc", "void Window::updateFixed"))
        start = body.index("if (!inputPort_.empty())")
        end = body.index("audio::gMixer.service()", start)
        input_phase = body[start:end]

        self.assertRegex(input_phase, r"while\s*\(\s*!inputPort_\.empty\(\)")
        loop_start = input_phase.index("while")
        loop_body_start = input_phase.index("{", loop_start)
        depth = 0
        loop_body_end = None
        for index in range(loop_body_start, len(input_phase)):
            if input_phase[index] == "{":
                depth += 1
            elif input_phase[index] == "}":
                depth -= 1
                if depth == 0:
                    loop_body_end = index
                    break
        self.assertIsNotNone(loop_body_end)
        loop_body = input_phase[loop_body_start + 1:loop_body_end]
        self.assertIn("auto *target = popup_ ? popup_ : map_", loop_body)
        self.assertLess(
            loop_body.index("auto *target = popup_ ? popup_ : map_"),
            loop_body.index("inputPort_.deliverNext(*target)"),
        )
        self.assertIn("applyDeferredNodes()", loop_body)
        self.assertIn("applyDeferredCommands()", loop_body)

    def test_command_barrier_is_not_reached_from_render_or_flush(self) -> None:
        text = strip_comments(source("src/scene/window.cc"))
        render_start = text.index("void Window::render() const")
        flush_start = text.index("bool Window::flush")
        render = function_body("src/scene/window.cc", "void Window::render() const")
        flush = function_body("src/scene/window.cc", "bool Window::flush")
        for phase_body in (render, flush):
            self.assertNotIn("applyDeferredNodes", phase_body)
            self.assertNotIn("applyDeferredCommands", phase_body)
            self.assertNotIn("deferredCommands_", phase_body)
        self.assertLess(render_start, flush_start)

    def test_direct_window_side_effects_are_not_called_from_logic_or_input_files(self) -> None:
        paths = (
            "src/scene/map_logic_movement.cc",
            "src/scene/map_input.cc",
            "src/scene/map_render.cc",
            "src/scene/warfield_input.cc",
            "src/scene/warfield_actions.cc",
            "src/scene/warfield_ai.cc",
            "src/scene/warfield_ai_skill.cc",
            "src/scene/warfield_turns.cc",
            "src/scene/warfield_results.cc",
            "src/scene/warfield_ui.cc",
            "src/scene/item_selection_controller.cc",
            "src/scene/itemview.cc",
            "src/scene/menu.cc",
            "src/scene/title.cc",
            "src/scene/window_menu.cc",
        )
        forbidden = (
            "gWindow->title", "gWindow->endscreen", "gWindow->newGame",
            "gWindow->loadGame", "gWindow->saveGame", "gWindow->forceQuit",
            "gWindow->exitToGlobalMap", "gWindow->enterSubMap", "gWindow->enterWar",
            "gWindow->endWar", "gWindow->playerDie", "gWindow->useQuestItem",
            "gWindow->forceEvent", "gWindow->closePopup", "gWindow->endPopup",
            "gWindow->showMainMenu", "gWindow->runTalk", "gWindow->runShop",
            "gWindow->popupMessageBox", "gWindow->playMusic", "gWindow->playAtkSound",
            "gWindow->playEffectSound", "Window::beginInput", "Window::setInputRect",
            "Window::endInput", "defer(",
        )
        for relative in paths:
            text = strip_comments(source(relative))
            for token in forbidden:
                self.assertNotIn(token, text, f"{token} in {relative}")

    def test_scene_command_queue_is_the_only_deferred_command_owner(self) -> None:
        command_header = strip_comments(source("src/scene/logic/command.hh"))
        self.assertIn("class SceneCommandQueue", command_header)
        for path in sorted((ROOT / "src" / "scene").rglob("*.cc")):
            relative = path.relative_to(ROOT).as_posix()
            text = strip_comments(path.read_text(encoding="utf-8"))
            if (
                relative.startswith("src/scene/logic/")
                or path.stem.endswith("command")
                or relative in {
                    "src/scene/window.cc",
                    "src/scene/window_interaction.cc",
                }
            ):
                continue
            self.assertNotIn("applyDeferredCommands", text, relative)
            self.assertNotIn("executeAllGenerations", text, relative)


if __name__ == "__main__":
    unittest.main()
