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


class RenderPurityTests(unittest.TestCase):
    def test_map_render_does_not_advance_compatibility_logic(self) -> None:
        body = function_body("src/scene/map.cc", "void Map::render()")
        self.assertNotIn("frameUpdate", body)
        self.assertNotIn("++frames_", body)

    def test_mask_render_only_draws_precomputed_alpha(self) -> None:
        body = function_body("src/scene/mask.cc", "void Mask::render()")
        self.assertNotIn("fadeEnd", body)
        self.assertNotIn("currTime", body)

    def test_end_screen_render_does_not_advance_animation(self) -> None:
        body = function_body("src/scene/endscreen.cc", "void EndScreen::render()")
        self.assertNotIn("++frame_", body)
        self.assertNotIn("stage_ =", body)
        self.assertNotIn("y_ -=", body)

    def test_global_map_render_does_not_move_clouds(self) -> None:
        body = function_body("src/scene/globalmap.cc", "void GlobalMap::render()")
        self.assertNotIn("cloudX_[i]++", body)
        self.assertNotIn("c = nullptr", body)

    def test_warfield_render_does_not_mutate_effect_cells(self) -> None:
        body = function_body("src/scene/warfield_render.cc", "void Warfield::render()")
        self.assertNotIn(".effectData =", body)
        self.assertNotIn("effectData = nullptr", body)

    def test_cached_node_render_does_not_rebuild_or_create_state(self) -> None:
        body = function_body("src/scene/nodewithcache.cc", "void NodeWithCache::render()")
        self.assertNotIn("makeCache", body)
        self.assertNotIn("cacheDirty_ =", body)

    def test_popup_layout_and_children_are_prepared_during_update(self) -> None:
        message_body = function_body("src/scene/messagebox.cc", "void MessageBox::makeCache()")
        title_body = function_body("src/scene/title.cc", "void Title::makeCache()")
        self.assertNotIn("new MenuYesNo", message_body)
        self.assertNotIn("new MenuYesNo", title_body)
        self.assertNotIn("text_.clear", message_body)

    def test_render_does_not_commit_scene_commands(self) -> None:
        body = function_body("src/scene/window.cc", "void Window::render()")
        self.assertNotIn("applyDeferredNodes", body)
        self.assertNotIn("applyDeferredCommands", body)

    def test_item_view_cache_does_not_clamp_selection(self) -> None:
        body = function_body("src/scene/itemview.cc", "void ItemView::makeCache()")
        self.assertNotIn("currTop_ =", body)


if __name__ == "__main__":
    unittest.main()
