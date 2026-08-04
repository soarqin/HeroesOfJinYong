from pathlib import Path
import unittest


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
    raise AssertionError(f"unterminated function {signature} in {path}")


class AudioCallbackPurityTests(unittest.TestCase):
    def test_callback_does_not_perform_lifecycle_or_clock_work(self) -> None:
        body = function_body("src/audio/mixer.cc", "void Mixer::callback")
        for forbidden in ("SDL_GetTicks", "new ", "delete ", ".reset(", ".resize(",
                          "filenameNext", "dynamic_cast", ".load(", ".start()",
                          "readData"):
            self.assertNotIn(forbidden, body)

    def test_lifecycle_work_is_on_main_thread_service(self) -> None:
        body = function_body("src/audio/mixer.cc", "void Mixer::service()")
        self.assertIn("SDL_GetTicks", body)
        self.assertIn("prepareChannelLocked", body)
        self.assertIn("fillChannelLocked", body)

        play_body = function_body(
            "src/audio/mixer.cc",
            "void Mixer::play(size_t channelId, const std::string &filename",
        )
        self.assertIn("loadFilenameLocked", play_body)

    def test_fade_and_reload_keep_a_candidate_state(self) -> None:
        header = (ROOT / "src/audio/mixer.hh").read_text(encoding="utf-8")
        body = (ROOT / "src/audio/mixer.cc").read_text(encoding="utf-8")
        self.assertIn("fadeOutVolumeStart", header)
        self.assertIn("std::unique_ptr<Channel> candidate", body)


if __name__ == "__main__":
    unittest.main()
