import unittest
import re
from pathlib import Path


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


class LogicRenderBoundaryTests(unittest.TestCase):
    def test_map_character_logic_selects_sprite_ids(self):
        header = source("src/scene/mapwithevent.hh")
        self.assertIn("mainCharSpriteId_", header)
        self.assertNotIn("mainCharTex_", header)

        for path, signature in (
            ("src/scene/globalmap.cc", "void GlobalMap::updateMainCharSpriteId"),
            ("src/scene/submap.cc", "void SubMap::updateMainCharSpriteId"),
        ):
            body = function_body(path, signature)
            self.assertNotIn("getOrLoadTexture", body, path)
            self.assertIn("mainCharSpriteId_", body, path)

    def test_global_map_cloud_logic_uses_sprite_ids(self):
        header = source("src/scene/globalmap.hh")
        self.assertIn("cloudSpriteId_", header)
        self.assertIn("preparedCloud_", header)

        body = function_body("src/scene/globalmap.cc", "void GlobalMap::update()")
        self.assertNotIn("cloudTexMgr_", body)
        self.assertNotIn("cloud_[", body)
        self.assertIn("cloudSpriteId_", body)

    def test_warfield_logic_files_do_not_access_render_services(self):
        for path in (
            "src/scene/warfield_actions.cc",
            "src/scene/warfield_ai.cc",
            "src/scene/warfield_ai_skill.cc",
            "src/scene/warfield_input.cc",
            "src/scene/warfield_turns.cc",
        ):
            text = source(path)
            self.assertNotIn("renderer_", text, path)
            self.assertNotIn("Texture", text, path)
            self.assertNotIn("TTF", text, path)

    def test_warfield_logic_only_requests_status_panel_release(self):
        logic = strip_comments(source("src/scene/warfield_results.cc"))
        self.assertNotIn("delete statusPanel_", logic)
        self.assertIn("statusPanelReleaseRequested_", logic)
        header = strip_comments(source("src/scene/warfield.hh"))
        self.assertIn("statusPanelReleaseRequested_", header)
        prepare = function_body(
            "src/scene/warfield_render.cc", "void Warfield::prepareRender"
        )
        self.assertIn("statusPanelReleaseRequested_", prepare)

    def test_warfield_logic_defers_node_tree_cleanup_to_prepare(self):
        header = strip_comments(source("src/scene/warfield.hh"))
        self.assertIn("presentationCleanupRequested_", header)
        for path in (
            "src/scene/warfield_results.cc",
            "src/scene/warfield_load.cc",
        ):
            text = strip_comments(source(path))
            self.assertNotIn("removeAllChildren", text, path)
            self.assertNotIn("fadeNode_ =", text, path)
            self.assertNotIn("fadePostAction_ =", text, path)
            self.assertNotIn("runFadePostAction_ =", text, path)
        prepare = function_body(
            "src/scene/warfield_render.cc", "void Warfield::prepareRender"
        )
        self.assertIn("presentationCleanupRequested_", prepare)
        self.assertIn("removeAllChildren", prepare)

    def test_warfield_load_defers_texture_cache_reset_to_prepare(self):
        body = function_body("src/scene/warfield_load.cc", "bool Warfield::load")
        self.assertNotIn("textureMgr_.clear", body)
        header = strip_comments(source("src/scene/warfield.hh"))
        self.assertIn("presentationTextureResetRequested_", header)
        prepare = function_body(
            "src/scene/warfield_render.cc", "void Warfield::prepareRender"
        )
        self.assertIn("presentationTextureResetRequested_", prepare)
        self.assertIn("textureMgr_.clear", prepare)

    def test_item_view_only_reports_selection(self):
        show = function_body("src/scene/itemview.cc", "void ItemView::show")
        consume = function_body(
            "src/scene/itemview.cc", "void ItemView::consumeKeyIntent"
        )
        apply = function_body(
            "src/scene/itemview.cc", "void ItemView::applyInputLogic"
        )

        for token in ("gBag", "useItem(", "equipItem(", "charInfo_"):
            self.assertNotIn(token, show)
            self.assertNotIn(token, consume)
            self.assertNotIn(token, apply)
        self.assertIn("selectionController_", apply)

    def test_status_view_consumes_value_snapshot_only(self):
        text = strip_comments(source("src/scene/statusview.cc"))
        header = strip_comments(source("src/scene/statusview.hh"))
        for token in ("gSaveData", "addUpPropFromEquipToChar(",
                      "getExpForLevelUp(", "getExpForSkillLearn(",
                      "world/savedata.hh", "world/action.hh", "world/strings.hh"):
            self.assertNotIn(token, text, token)
            self.assertNotIn(token, header, token)
        self.assertIn("CharacterStatusSnapshot", header)

    def test_warfield_status_view_receives_logic_snapshot(self):
        render = strip_comments(source("src/scene/warfield_render.cc"))
        logic = strip_comments(source("src/scene/warfield_input_logic.cc"))
        presentation = strip_comments(source("src/scene/warfield_presentation.cc"))
        self.assertNotIn("status->show(&currentActor_->info", render)
        self.assertNotIn("status->show(&selected->info", presentation)
        self.assertIn("statusSnapshot_", render)
        self.assertIn("buildCharacterStatusSnapshot", logic)
        self.assertIn("request.statuses", logic)
        self.assertIn("request.statuses", presentation)
        self.assertNotIn("buildCharacterStatusSnapshot", presentation)

    def test_world_item_policy_does_not_commit_live_world_from_view_policy(self):
        text = strip_comments(source("src/scene/world_item_policy.cc"))
        for token in ("*character =", "gBag =", "state::equipItem(",
                      "state::useItem(", "gSaveData.charInfo", "gSaveData.itemInfo"):
            self.assertNotIn(token, text, token)
        self.assertIn("candidate", text)

    def test_world_item_selection_is_action_object_based(self):
        text = strip_comments(source("src/scene/world_item_policy.cc"))
        start = text.index("void WorldItemSelectionController::select")
        end = text.index("void WorldItemSelectionController::equipItem", start)
        body = text[start:end]
        self.assertNotIn("switch (", body)
        self.assertIn("actions_", text)

    def test_window_menu_uses_typed_world_candidates(self):
        text = strip_comments(source("src/scene/window_menu.cc"))
        for token in ("gSaveData", "gBag", "actMedic(", "actDepoison(",
                      "leaveTeam(", "getLeaveEventId(", "ShopData *",
                      "shopInfo->", "&shopInfo"):
            self.assertNotIn(token, text, token)
        for token in ("prepareMedic(", "prepareDepoison(",
                      "prepareLeaveTeam(", "prepareShopPurchase(",
                      "commitUtilityAction(", "commitShopPurchase("):
            self.assertNotIn(token, text, token)
        for token in ("setShowMapMiniPanel(", "setShowMinimap(",
                      "setMusicVolume(", "setSoundVolume(",
                      "gMixer.setVolume(", "saveOptions(",
                      "continueEvents(false)"):
            self.assertNotIn(token, text, token)
        commands = strip_comments(source("src/scene/menu_commands.hh"))
        for token in ("MedicActionCommand", "DepoisonActionCommand",
                      "LeaveTeamActionCommand", "PurchaseShopOfferCommand",
                      "OptionsCommitCommand", "ContinueEventCommand"):
            self.assertIn(token, commands, token)

    def test_logic_namespace_has_no_platform_or_render_dependencies(self):
        logic_root = ROOT / "src" / "scene" / "logic"
        paths = sorted(logic_root.rglob("*.cc")) + sorted(logic_root.rglob("*.hh"))
        self.assertTrue(paths)
        forbidden = (
            "SDL", "Renderer", "Texture", "TTF", "gWindow", "gSaveData", "gBag",
            "renderer_", "SDL_", "Window *", "Window&",
        )
        for path in paths:
            text = strip_comments(path.read_text(encoding="utf-8"))
            for token in forbidden:
                self.assertNotIn(token, text, f"{token} in {path.relative_to(ROOT)}")

    def test_input_and_battle_logic_files_do_not_reach_render_services(self):
        paths = sorted((ROOT / "src" / "scene").glob("*_input.cc"))
        paths.extend(
            ROOT / "src" / "scene" / name
            for name in (
                "warfield_actions.cc",
                "warfield_ai.cc",
                "warfield_ai_skill.cc",
                "warfield_turns.cc",
                "warfield_results.cc",
            )
        )
        forbidden = ("renderer_", "Renderer", "Texture", "TTF", "gWindow->")
        for path in paths:
            text = strip_comments(path.read_text(encoding="utf-8"))
            for token in forbidden:
                self.assertNotIn(token, text, f"{token} in {path.relative_to(ROOT)}")

    def test_map_logic_resolves_resources_only_during_prepare(self):
        for path, signature in (
            ("src/scene/map_logic_movement.cc", "void MapWithEvent::setDirection"),
            ("src/scene/map_logic_movement.cc", "void MapWithEvent::setPosition"),
            ("src/scene/map_logic_movement.cc", "void MapWithEvent::move"),
            ("src/scene/map_logic_movement.cc", "void MapWithEvent::update"),
            ("src/scene/globalmap.cc", "void GlobalMap::update"),
        ):
            body = function_body(path, signature)
            for token in ("getOrLoadTexture", "TextureMgr", "renderer_->", "ttf()"):
                self.assertNotIn(token, body, f"{token} in {path}:{signature}")

        prepare = function_body("src/scene/map_render.cc", "void MapWithEvent::prepareRender")
        self.assertIn("getOrLoadTexture", prepare)

    def test_map_logic_mutates_serialized_sprite_ids_not_render_textures(self):
        header = strip_comments(source("src/scene/mapwithevent.hh"))
        self.assertIn("setCellSpriteId", header)
        self.assertNotIn("setCellTexture", header)
        for path in (
            "src/scene/map_event_character.cc",
            "src/scene/map_event_extended.cc",
            "src/scene/map_event_interaction.cc",
            "src/scene/map_logic_movement.cc",
            "src/scene/submap.cc",
        ):
            text = strip_comments(source(path))
            self.assertNotIn("setCellTexture", text, path)

    def test_global_map_load_only_commits_value_state(self):
        body = function_body("src/scene/globalmap.cc", "bool GlobalMap::load")
        for token in (
            "Texture::", "TextureLock", "renderer_->", "calcRLEAvgColor",
            "enableBlendMode", "targetTexture",
        ):
            self.assertNotIn(token, body, token)
        self.assertIn("miniMapRevision_", body)
        prepare = function_body(
            "src/scene/globalmap_render.cc", "void GlobalMap::prepareMiniMapRender"
        )
        self.assertIn("calcRLEAvgColor", prepare)
        self.assertIn("TextureLock", prepare)

    def test_event_candidate_validation_uses_pure_rle_contract(self):
        event_ops = strip_comments(source("src/scene/map_event_ops.cc"))
        self.assertIn("logic::validateRleData", event_ops)
        self.assertNotIn("Texture::", event_ops)
        self.assertIn("GrpData::loadData", event_ops)
        self.assertNotIn("numeric_limits<std::int16_t>::max", event_ops)
        rle_contract = strip_comments(source("src/scene/logic/rle.hh"))
        self.assertIn("validateRleData", rle_contract)
        for token in ("Renderer", "Texture", "SDL"):
            self.assertNotIn(token, rle_contract)

    def test_map_and_battle_loaders_use_pure_rle_validation(self):
        for path in (
            "src/scene/globalmap.cc",
            "src/scene/submap.cc",
            "src/scene/warfield_load.cc",
        ):
            text = strip_comments(source(path))
            self.assertIn("logic::validateRleData", text, path)
            self.assertNotIn("Texture::validateRLE", text, path)

    def test_message_and_talk_layout_wait_for_render_preparation(self):
        message = source("src/scene/messagebox.cc")
        popup = function_body("src/scene/messagebox.cc", "void MessageBox::popup")
        update = function_body("src/scene/messagebox.cc", "void MessageBox::update")
        ensure = function_body("src/scene/messagebox.cc", "void MessageBox::ensureLayout")
        for token in ("renderer_->", "ttf()", "buildLayoutSnapshot", "ensureYesNoMenu"):
            self.assertNotIn(token, popup, token)
            self.assertNotIn(token, update, token)
        self.assertIn("buildLayoutSnapshot", ensure)
        self.assertIn("ensureYesNoMenu", ensure)
        self.assertIn("void ensureLayout() override", source("src/scene/messagebox.hh"))

        talk_popup = function_body("src/scene/talkbox.cc", "void TalkBox::popup")
        talk_prepare = function_body("src/scene/talkbox.cc", "void TalkBox::ensureLayout")
        for token in ("renderer_->", "ttf()", "headTextureProvider_"):
            self.assertNotIn(token, talk_popup, token)
        for token in ("renderer_->", "ttf()", "headTextureProvider_"):
            self.assertIn(token, talk_prepare, token)

        talk_resources = function_body(
            "src/scene/talkbox.cc", "bool TalkBox::prepareTextResources"
        )
        self.assertIn("(void)ttf->prepareText(sourceText_)", talk_resources)
        self.assertIn("return true", talk_resources)
        self.assertNotRegex(
            talk_resources,
            r"return\s+ttf->prepareText|prepareText\([^;]+&&",
        )
        self.assertIn("layoutReady_ = true", talk_prepare)
        self.assertIn("(void)ttf->measureCharAdvance(ch, advance)", talk_prepare)
        self.assertIn("collectTalkMetricRequest", talk_prepare)
        self.assertIn("measureKerningAdvance", talk_prepare)
        self.assertNotIn("metricsReady", talk_prepare)

        char_init = function_body("src/scene/charlistmenu.cc", "void CharListMenu::init")
        char_prepare = function_body("src/scene/charlistmenu.cc", "void CharListMenu::prepareRender")
        self.assertNotIn("renderer_->", char_init)
        self.assertIn("setAltColor", char_prepare)

    def test_window_menu_children_use_viewport_not_root_content_bounds(self):
        menu = strip_comments(source("src/scene/window_menu.cc"))
        self.assertNotIn("mainMenu->rootWidth()", menu)
        self.assertNotIn("mainMenu->rootHeight()", menu)
        self.assertNotIn("parent->rootWidth()", menu)
        self.assertNotIn("parent->rootHeight()", menu)
        self.assertIn("window->width() - x", menu)
        self.assertIn("window->height() - y", menu)
        self.assertIn("view->setViewportSize(window->width(), window->height())", menu)

        char_init = function_body(
            "src/scene/charlistmenu.cc", "void CharListMenu::init"
        )
        self.assertIn(
            "new MessageBox(renderer_, x_, y_, width_, height_)", char_init
        )
        self.assertNotIn("rootWidth() - x_", char_init)

    def test_nested_selection_and_yes_no_layout_keep_stable_frame_bounds(self):
        selection = function_body(
            "src/scene/window_presentation.cc",
            "void Window::showCharacterSelection",
        )
        self.assertIn("width_ - x - border * 2", selection)
        self.assertIn("height_ - y - border * 2", selection)
        self.assertIn("menu->makeCenter(width_, height_, 0, 0)", selection)
        self.assertNotIn("parent->rootWidth()", selection)
        self.assertNotIn("parent->rootHeight()", selection)

        yes_no = function_body(
            "src/scene/messagebox.cc", "void MessageBox::ensureYesNoMenu"
        )
        self.assertIn("frameX_ + frameWidth_ - menuX", yes_no)
        self.assertIn("frameY_ + frameHeight_ - y_", yes_no)
        self.assertNotIn("rootWidth()", yes_no)
        self.assertNotIn("rootHeight()", yes_no)

    def test_cached_widgets_center_after_content_layout(self):
        make_center = function_body(
            "src/scene/nodewithcache.cc", "void NodeWithCache::makeCenter"
        )
        prepare = function_body(
            "src/scene/nodewithcache.cc", "void NodeWithCache::prepareRender"
        )
        self.assertIn("layoutCenterRequested_ = true", make_center)
        self.assertNotIn("Node::makeCenter", make_center)
        self.assertIn("if (layoutCenterRequested_)", prepare)
        self.assertIn(
            "Node::makeCenter(layoutCenterWidth_, layoutCenterHeight_,",
            prepare,
        )

    def test_empty_menu_values_do_not_create_a_value_column(self):
        menu = source("src/scene/menu.cc")
        ensure = function_body("src/scene/menu.cc", "void Menu::ensureLayout")
        cache = function_body("src/scene/menu.cc", "void Menu::makeCache")
        self.assertIn("hasValueColumn", ensure)
        self.assertIn("if (hasValueColumn)", ensure)
        self.assertIn("hasValueColumn", cache)
        self.assertIn("if (hasValueColumn)", cache)
        self.assertNotIn("if (!values_.empty())", ensure)
        self.assertNotIn("if (!values_.empty())", cache)
        self.assertIn("MenuEntries{", source("src/scene/window_menu.cc"))

    def test_status_and_item_panels_keep_rendering_when_optional_glyphs_fail(self):
        status = function_body(
            "src/scene/statusview.cc", "bool StatusView::prepareTextResources"
        )
        item = function_body(
            "src/scene/itemview.cc", "bool ItemView::prepareTextResources"
        )
        self.assertIn("return true", status)
        self.assertIn("return true", item)
        self.assertNotIn("return ready", status)
        self.assertNotIn("return ready", item)

    def test_item_atlas_skips_unused_empty_slots_like_master(self):
        window = function_body(
            "src/scene/window.cc", "bool Window::initializeItemAtlas"
        )
        self.assertIn("if (data.empty()) { continue; }", window)
        self.assertLess(window.index("data.empty()"), window.index("validateRLE(data)"))

    def test_item_atlas_is_resolved_by_the_view_during_prepare(self):
        item_view = strip_comments(source("src/scene/itemview.cc")
                                   + source("src/scene/itemview.hh"))
        self.assertIn("void prepareRender() override", item_view)
        self.assertIn("renderer_->itemAtlas()", item_view)
        for path in ("src/scene/window_menu.cc",
                     "src/scene/warfield_presentation.cc"):
            text = strip_comments(source(path))
            self.assertNotIn("renderer_->itemAtlas", text, path)
            self.assertNotIn("setItemAtlas", text, path)

    def test_every_node_render_override_is_const(self):
        pattern = re.compile(
            r"\b([A-Za-z_]\w*)::render\s*\([^)]*\)\s*(const)?\s*\{"
        )
        found = []
        for path in sorted((ROOT / "src" / "scene").rglob("*.cc")):
            text = strip_comments(path.read_text(encoding="utf-8"))
            for match in pattern.finditer(text):
                owner = match.group(1)
                if owner in {"TTF"}:
                    continue
                found.append(owner)
                self.assertIsNotNone(match.group(2), f"{owner}::render in {path.relative_to(ROOT)}")
        self.assertTrue(found)

    def test_render_bodies_do_not_write_logic_or_cache_state(self):
        pattern = re.compile(
            r"\b([A-Za-z_]\w*)::render\s*\([^)]*\)\s*(?:const)?\s*\{"
        )
        forbidden = (
            r"\b(?:drawDirty_|miniPanelDirty_|cacheDirty_|dispatchDepth_|processingStage_|"
            r"deleteRequested_|preparedRevision_|requestedRevision_)\s*=",
            r"\b(?:children_|pendingDeletes_|deferredCommands_|commands_)\.(?:clear|push_back|"
            r"emplace_back|erase|insert|resize|pop_back)\s*\(",
            r"\b(?:makeCache|rebuildCache|cacheBegin|cacheEnd|setDirty|requestDelete|defer)\s*\(",
            r"\b(?:lock|unlock|setTargetTexture)\s*\(",
            r"\b(?:new|delete)\b",
            r"->(?:stringWidth|charDimension|setColor|setAltColor|init|add)\s*\(",
            r"\bSDL_[A-Za-z0-9_]+\b",
        )
        for path in sorted((ROOT / "src" / "scene").rglob("*.cc")):
            text = strip_comments(path.read_text(encoding="utf-8"))
            for match in pattern.finditer(text):
                if match.group(1) == "TTF":
                    continue
                body = text[match.end():]
                depth = 1
                end = 0
                for index, char in enumerate(body):
                    if char == "{":
                        depth += 1
                    elif char == "}":
                        depth -= 1
                        if depth == 0:
                            end = index
                            break
                self.assertGreater(end, 0, f"unterminated render in {path}")
                body = body[:end]
                for expression in forbidden:
                    self.assertIsNone(
                        re.search(expression, body),
                        f"{expression} in {match.group(1)}::render ({path.relative_to(ROOT)})",
                    )

    def test_cache_owners_expose_a_prepare_phase(self):
        for path, owner in (
            ("src/scene/nodewithcache.hh", "NodeWithCache"),
            ("src/scene/map.hh", "Map"),
            ("src/scene/globalmap.hh", "GlobalMap"),
            ("src/scene/submap.hh", "SubMap"),
            ("src/scene/warfield.hh", "Warfield"),
        ):
            text = strip_comments(source(path))
            self.assertRegex(text, r"\bprepareRender\s*\(\s*\)\s*override", owner)

    def test_logic_mutations_signal_revisions_instead_of_writing_render_dirty_flags(self):
        logic_paths = (
            "src/scene/map_logic_movement.cc",
            "src/scene/map_event_character.cc",
            "src/scene/map_event_extended.cc",
            "src/scene/map_event_interaction.cc",
            "src/scene/map_event_shop.cc",
            "src/scene/map_event_story.cc",
            "src/scene/submap.cc",
            "src/scene/warfield_actions.cc",
            "src/scene/warfield_input_logic.cc",
            "src/scene/warfield_load.cc",
            "src/scene/warfield_turns.cc",
            "src/scene/globalmap.cc",
        )
        for path in logic_paths:
            text = strip_comments(source(path))
            self.assertNotIn("drawDirty_", text, path)
            self.assertNotIn("miniPanelDirty_", text, path)
        map_header = strip_comments(source("src/scene/map.hh"))
        self.assertIn("worldRevision_", map_header)
        self.assertIn("markWorldChanged", map_header)
        self.assertIn("markMiniPanelChanged", map_header)

    def test_scene_logic_uses_explicit_presentation_invalidation_signal(self):
        cache_header = strip_comments(source("src/scene/nodewithcache.hh"))
        self.assertIn("requestPresentationRefresh", cache_header)
        self.assertIn("requestedPresentationRevision_", cache_header)
        self.assertIn("preparedPresentationRevision_", cache_header)
        self.assertNotIn("setDirty", cache_header)
        self.assertNotIn("forceUpdate", cache_header)

        request_body = function_body(
            "src/scene/nodewithcache.hh", "void requestPresentationRefresh"
        )
        self.assertIn("requestedPresentationRevision_", request_body)
        self.assertNotIn("cacheDirty_", request_body)
        self.assertIn("preparedPresentationRevision_ = 0", request_body)

        prepare_body = function_body(
            "src/scene/nodewithcache.cc", "void NodeWithCache::prepareRender"
        )
        self.assertIn("requestedPresentationRevision_", prepare_body)
        self.assertIn("preparedPresentationRevision_", prepare_body)

        scene_root = ROOT / "src" / "scene"
        for path in sorted(scene_root.rglob("*.cc")):
            text = strip_comments(path.read_text(encoding="utf-8"))
            self.assertNotIn(
                "setDirty(", text, f"legacy render dirty API in {path.relative_to(ROOT)}"
            )

    def test_map_panel_snapshot_commits_name_and_position_atomically(self):
        map_header = strip_comments(source("src/scene/map.hh"))
        self.assertIn("commitMiniPanelSnapshot", map_header)
        self.assertNotIn("setMiniPanelMapName", map_header)

        submap_load = function_body("src/scene/submap.cc", "bool SubMap::load")
        global_load = function_body("src/scene/globalmap.cc", "bool GlobalMap::load")
        self.assertIn("commitMiniPanelSnapshot", submap_load)
        self.assertIn("commitMiniPanelSnapshot", global_load)

    def test_submap_event_and_layer_mutations_validate_candidates_before_commit(self):
        extended = strip_comments(source("src/scene/map_event_extended.cc"))
        self.assertIn("validateSubMapState", extended)
        self.assertIn("candidateEvents", extended)
        self.assertIn("candidateLayer", extended)
        self.assertNotRegex(
            extended,
            r"gSaveData\.subMapEventInfo\[mapIndex\]->events\[eventIndex\].*=",
        )
        self.assertNotRegex(
            extended,
            r"gSaveData\.subMapLayerInfo\[mapIndex\]->data\[layer\]\[.*\]\s*=",
        )

        shop = strip_comments(source("src/scene/map_event_shop.cc"))
        self.assertIn("validateSubMapState", shop)
        self.assertIn("candidateEvents", shop)
        self.assertNotIn("evts[n].event[2] =", shop)
        self.assertNotIn("subMapEventInfo[evi.subMapId]->events", shop)

        interaction = strip_comments(source("src/scene/map_event_interaction.cc"))
        self.assertIn("amount < 0", interaction)


if __name__ == "__main__":
    unittest.main()
