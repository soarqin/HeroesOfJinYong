from pathlib import Path
import re
import unittest


ROOT = Path(__file__).resolve().parents[2]


def strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.DOTALL)
    return re.sub(r"//[^\n]*", "", text)


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


def render_bodies(text: str):
    pattern = re.compile(
        r"\b([A-Za-z_]\w*)::render\s*\([^)]*\)\s*(const)?\s*\{"
    )
    for match in pattern.finditer(text):
        owner = match.group(1)
        brace = match.end() - 1
        depth = 0
        for index in range(brace, len(text)):
            if text[index] == "{":
                depth += 1
            elif text[index] == "}":
                depth -= 1
                if depth == 0:
                    yield owner, match, text[brace + 1:index]
                    break
        else:
            raise AssertionError(f"unterminated render for {owner}")


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
        body = function_body("src/scene/globalmap_render.cc", "void GlobalMap::render() const")
        self.assertNotIn("cloudX_[i]++", body)
        self.assertNotIn("c = nullptr", body)

    def test_warfield_render_does_not_mutate_effect_cells(self) -> None:
        body = function_body("src/scene/warfield_render.cc", "void Warfield::render()")
        self.assertNotIn(".effectData =", body)
        self.assertNotIn("effectData = nullptr", body)

    def test_warfield_prepare_consumes_logic_effect_snapshot(self) -> None:
        source_text = strip_comments(
            (ROOT / "src/scene/warfield_render.cc").read_text(encoding="utf-8"))
        for token in (
            "world/savedata.hh", "skillInfo", "attackAreaType",
            "selRange", "enumerateAttackCells",
        ):
            self.assertNotIn(token, source_text)
        self.assertIn("effectOverlaySnapshot_.cellIndices", source_text)

        actions = strip_comments(
            (ROOT / "src/scene/warfield_actions.cc").read_text(encoding="utf-8"))
        self.assertIn("buildBattleEffectOverlaySnapshot", actions)

    def test_cached_node_render_does_not_rebuild_or_create_state(self) -> None:
        body = function_body("src/scene/nodewithcache.cc", "void NodeWithCache::render()")
        self.assertNotIn("makeCache", body)
        self.assertNotIn("cacheDirty_ =", body)

    def test_popup_layout_and_children_are_prepared_during_update(self) -> None:
        message_body = function_body("src/scene/messagebox.cc", "void MessageBox::makeCache()")
        title_body = function_body("src/scene/title_render.cc", "void Title::makeCache()")
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

    def test_every_scene_render_override_is_const(self) -> None:
        definitions = []
        for path in sorted((ROOT / "src" / "scene").rglob("*.cc")):
            text = strip_comments(path.read_text(encoding="utf-8"))
            for owner, match, _body in render_bodies(text):
                if owner == "TTF":
                    continue
                definitions.append((path, match))
                self.assertIsNotNone(
                    match.group(2),
                    f"{owner}::render is not const in {path.relative_to(ROOT)}",
                )
        self.assertTrue(definitions)

    def test_render_does_not_call_lazy_text_or_cache_mutation_apis(self) -> None:
        forbidden = (
            "->stringWidth(", "->charDimension(", "->setColor(", "->setAltColor(",
            "makeCache(", "rebuildCache(", "cacheBegin(", "cacheEnd(",
            "setTargetTexture(", "lock(", "unlock(",
        )
        for path in sorted((ROOT / "src" / "scene").rglob("*.cc")):
            text = strip_comments(path.read_text(encoding="utf-8"))
            for owner, _match, body in render_bodies(text):
                if owner == "TTF":
                    continue
                for token in forbidden:
                        self.assertNotIn(token, body, f"{token} in {owner}::render ({path.relative_to(ROOT)})")

    def test_cache_builders_use_prepared_ttf_apis_only(self) -> None:
        """Cache preparation may populate glyphs, but must not lazily mutate them."""
        for path in sorted((ROOT / "src" / "scene").rglob("*.cc")):
            text = strip_comments(path.read_text(encoding="utf-8"))
            for match in re.finditer(r"\b[A-Za-z_]\w*::makeCache\s*\([^)]*\)\s*\{", text):
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
                self.assertIsNotNone(end, f"unterminated makeCache in {path}")
                body = text[brace + 1:end]
                for token in ("->stringWidth(", "->charDimension(", "->render("):
                    self.assertNotIn(token, body, f"{token} in makeCache ({path.relative_to(ROOT)})")

    def test_ttf_render_and_measurement_are_read_only(self) -> None:
        render = function_body("src/scene/ttf.cc", "void TTF::render(")
        width = function_body("src/scene/ttf.cc", "int TTF::stringWidth(")
        dimension = function_body("src/scene/ttf.cc", "void TTF::charDimension(")
        for body in (render, width, dimension):
            self.assertNotIn("makeCache(", body)

    def test_render_does_not_touch_node_lifecycle_or_deferred_state(self) -> None:
        for path in sorted((ROOT / "src" / "scene").rglob("*.cc")):
            text = strip_comments(path.read_text(encoding="utf-8"))
            for owner, _match, body in render_bodies(text):
                if owner == "TTF":
                    continue
                for token in (
                    "requestDelete(", "applyDeferredDeletes(", "applyDeferredNodes(",
                    "applyDeferredCommands(", "defer(", "setDirty(", "delete ", "new ",
                    "dispatchDepth_ =", "processingStage_ =", "deleteRequested_ =",
                ):
                    self.assertNotIn(token, body, f"{token} in {owner}::render")

    def test_prepare_and_render_are_distinct_window_phases(self) -> None:
        application = strip_comments((ROOT / "src/app/application.cc").read_text(encoding="utf-8"))
        self.assertLess(application.index("window_.prepareRender()"), application.index("window_.render()"))
        prepare = function_body("src/scene/window.cc", "bool Window::prepareRender")
        render = function_body("src/scene/window.cc", "void Window::render() const")
        self.assertIn("dispatchPrepareRender", prepare)
        self.assertIn("doRender", render)
        self.assertNotIn("dispatchPrepareRender", render)

    def test_item_atlas_initialization_is_validated_and_transactional(self) -> None:
        body = function_body("src/scene/window.cc", "bool Window::initializeItemAtlas")
        self.assertIn("Texture::validateRLE", body)
        self.assertIn("TextureLock", body)
        self.assertIn("Texture::renderRLE", body)
        self.assertIn("std::unique_ptr<Texture>", body)
        self.assertIn("renderer_->setItemAtlas", body)
        self.assertLess(body.index("Texture::renderRLE"), body.index("renderer_->setItemAtlas"))
        self.assertNotIn("reinterpret_cast<const int16_t *>", body)

    def test_raw_texture_insertion_is_exception_safe_and_dimension_checked(self) -> None:
        raw = function_body("src/scene/texture.cc", "Texture *Texture::loadFromRAW")
        manager_raw = function_body("src/scene/texture.cc", "Texture *TextureMgr::loadFromRAW")
        self.assertIn("std::numeric_limits<std::int16_t>::max()", raw)
        self.assertIn("textures_.emplace", manager_raw)
        self.assertIn("delete tex", manager_raw)
        self.assertNotIn("textures_[index] = tex", manager_raw)

    def test_font_prepare_is_idempotent_and_failed_glyphs_are_not_committed(self) -> None:
        prepare = function_body("src/scene/ttf.cc", "bool TTF::prepareText")
        self.assertIn("fontCache_.find", prepare)
        self.assertLess(prepare.index("fontCache_.find"), prepare.index("makeCache"))

        make_cache = function_body("src/scene/ttf.cc", "const TTF::FontData *TTF::makeCache")
        self.assertIn("FontData candidate", make_cache)
        self.assertIn("TextureLock", make_cache)
        self.assertIn("fontCache_.emplace", make_cache)
        self.assertLess(make_cache.index("TextureLock"), make_cache.rindex("fontCache_.emplace"))
        self.assertNotIn("fontCache_[key]", make_cache)

    def test_font_loader_rejects_invalid_stb_inputs_and_checks_initialization(self) -> None:
        source = strip_comments((ROOT / "src/scene/ttf.cc").read_text(encoding="utf-8"))
        add = function_body("src/scene/ttf.cc", "bool TTF::add")
        self.assertIn("index < 0", source)
        self.assertIn("getStbFontOffset", add)
        self.assertIn("stbtt_InitFont", add)
        self.assertIn("fi.ttf_buffer.data()", add)
        self.assertIn("std::unique_ptr<stbtt_fontinfo>", add)
        self.assertIn("FT_Set_Pixel_Sizes", source)

    def test_stb_glyph_measurement_does_not_allocate_and_leak_a_bitmap(self) -> None:
        make_cache = function_body("src/scene/ttf.cc", "const TTF::FontData *TTF::makeCache")
        self.assertIn("stbtt_GetGlyphBitmapBox", make_cache)
        self.assertNotIn("stbtt_GetGlyphBitmap(", make_cache)

    def test_atlas_reservations_roll_back_after_upload_failure(self) -> None:
        rectpacker = strip_comments((ROOT / "src/scene/rectpacker.hh").read_text(encoding="utf-8"))
        self.assertIn("rollbackLast", rectpacker)
        for path, marker in (
            ("src/scene/ttf.cc", "const TTF::FontData *TTF::makeCache"),
            ("src/scene/texture.cc", "Texture *TextureMgr::loadFromRLE"),
        ):
            body = function_body(path, marker)
            self.assertIn("rollbackLast", body, path)

    def test_warfield_auxiliary_texture_creation_is_checked(self) -> None:
        body = function_body("src/scene/warfield_load.cc", "Warfield::Warfield(")
        self.assertIn("if (drawingTerrainTex2_)", body)
        self.assertNotIn("drawingTerrainTex2_->enableBlendMode(true);", body)

    def test_render_resource_entry_points_reject_null_textures(self) -> None:
        renderer = strip_comments((ROOT / "src/scene/renderer.cc").read_text(encoding="utf-8"))
        self.assertGreaterEqual(renderer.count("if (!tex || !tex->valid()) { return; }"), 4)
        self.assertIn("validSourceRect", renderer)
        self.assertIn("scaledDimension", renderer)
        blend = function_body("src/scene/texture.cc", "void Texture::setBlendColor")
        self.assertIn("if (!data_) { return; }", blend)

    def test_cache_prepare_rolls_back_geometry_and_never_deletes_a_bound_candidate(self) -> None:
        prepare = function_body("src/scene/nodewithcache.cc", "void NodeWithCache::prepareRender")
        for token in ("oldX", "oldY", "oldWidth", "oldHeight", "onPrepareFailed"):
            self.assertIn(token, prepare)

        rebuild = function_body("src/scene/nodewithcache.cc", "bool NodeWithCache::rebuildCache")
        self.assertIn("renderer_->targetTexture() == candidate", rebuild)
        self.assertIn("buildingCache_ = candidate", rebuild)
        self.assertIn("renderer_->targetTexture() == cache_", rebuild)
        destructor = function_body("src/scene/nodewithcache.cc", "NodeWithCache::~NodeWithCache()")
        self.assertIn("bound == cache_", destructor)

    def test_prepare_tree_cleanup_purges_stale_pending_delete_references(self) -> None:
        body = function_body("src/scene/node.cc", "void Node::removeAllChildren")
        self.assertIn("purgePendingReferences", body)
        self.assertIn("pendingDeletes_", body)
        self.assertIn("lastInputConsumer_", body)

    def test_fade_completion_is_queued_and_cleanup_waits_for_prepare(self) -> None:
        body = function_body("src/scene/node.cc", "void Node::doUpdate")
        self.assertIn("postCommand", body)
        self.assertIn("fadeCleanupRequested_", body)
        self.assertNotIn("fadeNode_->requestDelete", body)
        self.assertNotIn("if (fn) { fn(); }", body)

    def test_message_popup_defers_old_choice_menu_cleanup(self) -> None:
        popup = function_body("src/scene/messagebox.cc", "void MessageBox::popup")
        self.assertNotIn("menu_->requestDelete", popup)
        self.assertIn("presentationMenuCleanupRequested_", popup)
        prepare = function_body("src/scene/messagebox.cc", "void MessageBox::ensureLayout")
        self.assertIn("presentationMenuCleanupRequested_", prepare)

    def test_title_cache_keeps_previous_frame_on_missing_resource(self) -> None:
        body = function_body("src/scene/title_render.cc", "void Title::makeCache()")
        self.assertIn("throw std::runtime_error", body)
        self.assertNotIn("cacheEnd(); return;", body)

    def test_title_render_only_consumes_the_presentation_snapshot(self) -> None:
        render = strip_comments((ROOT / "src/scene/title_render.cc").read_text(encoding="utf-8"))
        for token in (
            "gSaveData", "gBag", "GETTEXT", "core::config", "world/",
            "calcColorForMpType", "CharacterData", "prepareNewGame",
        ):
            self.assertNotIn(token, render, token)
        self.assertIn("snapshot_", render)
        self.assertIn("std::visit", render)

    def test_message_box_creates_interactive_children_during_prepare(self) -> None:
        popup = function_body("src/scene/messagebox.cc", "void MessageBox::popup")
        update = function_body("src/scene/messagebox.cc", "void MessageBox::update")
        ensure = function_body("src/scene/messagebox.cc", "void MessageBox::ensureLayout")
        prepare_text = function_body("src/scene/messagebox.cc", "bool MessageBox::prepareTextResources")
        make_cache = function_body("src/scene/messagebox.cc", "void MessageBox::makeCache")
        self.assertNotIn("ensureYesNoMenu", popup)
        self.assertNotIn("ensureYesNoMenu", update)
        self.assertIn("ensureYesNoMenu", ensure)
        self.assertIn("buildLayoutSnapshot", ensure)
        self.assertNotIn("ensureYesNoMenu", prepare_text)
        self.assertNotIn("ensureYesNoMenu", make_cache)

    def test_logic_callers_do_not_expect_force_update_to_build_layout_synchronously(self) -> None:
        for path in (
            "src/scene/charlistmenu.cc",
            "src/scene/window_menu.cc",
            "src/scene/warfield_turns.cc",
        ):
            text = strip_comments((ROOT / path).read_text(encoding="utf-8"))
            self.assertNotIn("forceUpdate()", text, path)


if __name__ == "__main__":
    unittest.main()
