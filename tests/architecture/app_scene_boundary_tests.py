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


class AppSceneBoundaryTests(unittest.TestCase):
    def test_scene_window_does_not_own_sdl_input_collection(self):
        header = (ROOT / 'src' / 'scene' / 'window.hh').read_text(encoding='utf-8')
        implementation = (
            ROOT / 'src' / 'scene' / 'window_input.cc'
        ).read_text(encoding='utf-8')

        self.assertNotIn('app/input_repeat.hh', header)
        self.assertNotIn('InputRepeater inputRepeater_', header)
        self.assertNotIn('collectEvents(', header)
        self.assertNotIn('processEvents(', header)
        self.assertNotIn('SDL_PollEvent', implementation)

    def test_application_owns_the_sdl_input_collector(self):
        header = (ROOT / 'src' / 'app' / 'application.hh').read_text(encoding='utf-8')
        implementation = (
            ROOT / 'src' / 'app' / 'application.cc'
        ).read_text(encoding='utf-8')

        self.assertIn('#include "sdl_input.hh"', header)
        self.assertIn('SdlInputCollector inputCollector_', header)
        self.assertIn('inputCollector_.collect(inputQueue_)', implementation)

    def test_title_prepares_a_world_candidate_and_defers_scene_activation(self):
        header = (ROOT / "src/scene/title.hh").read_text(encoding="utf-8")
        logic = (ROOT / "src/scene/title_logic.cc").read_text(encoding="utf-8")
        command = (ROOT / "src/scene/logic/command.hh").read_text(encoding="utf-8")
        persistence = (ROOT / "src/scene/window_persistence.cc").read_text(encoding="utf-8")

        self.assertIn("TitleInputExecutionContext", header)
        self.assertIn("TitleScreenSnapshot", header)
        self.assertIn("NewGameCandidate", header)
        self.assertIn("prepareNewGameCandidate", logic)
        self.assertIn("setIdentity", logic)
        self.assertIn("StartNewGameCommand", logic)
        self.assertIn("startNewGame", command)
        self.assertIn("activateNewGameCandidate", persistence)
        self.assertIn("finalize()", persistence)

        title = (ROOT / "src/scene/title.cc").read_text(encoding="utf-8")
        self.assertNotIn("world/character.hh", title)
        self.assertNotIn("gSaveData.newGame()", title)


if __name__ == '__main__':
    unittest.main()
