/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "window.hh"

#include "colorpalette.hh"
#include "globalmap.hh"
#include "submap.hh"
#include "warfield.hh"
#include "effect.hh"
#include "talkbox.hh"
#include "title.hh"
#include "dead.hh"
#include "endscreen.hh"
#include "menu.hh"
#include "charlistmenu.hh"
#include "itemview.hh"
#include "statusview.hh"

#include "audio/mixer.hh"
#include "content/factors.hh"
#include "content/grpdata.hh"
#include "content/event.hh"
#include "world/strings.hh"
#include "world/savedata.hh"
#include "core/config.hh"
#include "util/conv.hh"

#include <SDL.h>
#include <fmt/xchar.h>
#include <limits>
#include <algorithm>
#include <thread>
#include <stdexcept>

namespace hojy::scene {

Window *gWindow = nullptr;

#if !defined(HOJY_VERSION)
#define HOJY_VERSION "development"
#endif

static const char *GameWindowTitle = "Heroes of Jin Yong " HOJY_VERSION;

namespace {

std::uint64_t wallTimeMicros() {
    const auto frequency = SDL_GetPerformanceFrequency();
    const auto counter = SDL_GetPerformanceCounter();
    if (frequency == 0) {
        return static_cast<std::uint64_t>(SDL_GetTicks64()) * 1000ULL;
    }
    if (counter > std::numeric_limits<std::uint64_t>::max() / 1000000ULL) {
        return std::numeric_limits<std::uint64_t>::max();
    }
    return counter * 1000000ULL / static_cast<std::uint64_t>(frequency);
}

}

Window::Window(int w, int h) : width_(w), height_(h), currTime_(wallTimeMicros()) {
    if (gWindow) {
        throw std::runtime_error("Duplicate window creation");
    }
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        SDL_Init(SDL_INIT_VIDEO);
    }
    if (!SDL_WasInit(SDL_INIT_GAMECONTROLLER)) {
        SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    }
    SDL_GameControllerEventState(SDL_ENABLE);
    auto *win = SDL_CreateWindow(GameWindowTitle,
                                 SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED,
                                 w,
                                 h,
                                 SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_HIDDEN);
#ifdef _WIN32
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");
#endif
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
    win_ = win;
    gWindow = this;

    renderer_ = new Renderer(win_, w, h);
    renderer_->enableLinear(false);

    if (!gNormalPalette.load("MMAP") || !gEndPalette.load("ENDCOL")) {
        ready_ = false;
        return;
    }
    {
        std::array<std::uint32_t, 256> n{};
        n.fill(0xFFFFFFFFu);
        n[0] = 0;
        gMaskPalette.create(n);
    }

    headTextureMgr_.setPalette(gNormalPalette);
    headTextureMgr_.setRenderer(renderer_);
    ::hojy::content::GrpData::DataSet dset;
    renderer_->enableLinear(true);
    if (::hojy::content::GrpData::loadData("HDGRP", dset)) {
        headTextureMgr_.loadFromRLE(dset);
    }
    renderer_->enableLinear(false);
    if (!gEffect.load("EFT")) {
        ready_ = false;
        return;
    }

    globalMap_ = new GlobalMap(renderer_, 0, 0, w, h, core::config.scale());
    subMap_ = new SubMap(renderer_, 0, 0, w, h, core::config.scale());
    warfield_ = new Warfield(renderer_, 0, 0, w, h, core::config.scale());

    {
        const auto *arr = reinterpret_cast<const int16_t *>(globalMap_->texData(::hojy::content::ItemTexIdStart).data());
        itemTexW_ = arr[0];
        itemTexH_ = arr[1];
    }
    itemWCount_ = 1024 / itemTexW_;
    itemHCount_ = (::hojy::content::BagItemCount + itemWCount_ - 1) / itemWCount_;
    int height = itemTexH_ * itemHCount_;
    itemTexture_ = Texture::create(renderer_, itemTexW_ * itemWCount_, height);
    itemTexture_->enableBlendMode(true);
    int pitch;
    const auto *colors = gNormalPalette.colors();
    auto *pixels = itemTexture_->lock(pitch);
    for (int i = 0; i < ::hojy::content::BagItemCount; ++i) {
        Texture::renderRLE(globalMap_->texData(::hojy::content::ItemTexIdStart + i),
                           colors,
                           pixels,
                           pitch,
                           height,
                           itemTexW_ * (i % itemWCount_),
                           itemTexH_ * (i / itemWCount_));
    }
    itemTexture_->unlock();
    SDL_ShowWindow(win);
    if (audio::gMixer.init(3)) {
        audio::gMixer.pause(false);
    }
    title();
}

Window::~Window() {
    if (gWindow == this) {
        gWindow = nullptr;
    }
    closePopup();
    headTextureMgr_.clear();
    gEffect.clear();
    delete itemTexture_;
    delete talkBox_;
    delete globalMap_;
    delete subMap_;
    delete warfield_;
    delete renderer_;
    SDL_DestroyWindow(static_cast<SDL_Window *>(win_));
}

const Texture *Window::smpTexture(std::int16_t id) const {
    if (!subMap_) { return nullptr; }
    return subMap_->getOrLoadTexture(id);
}

void Window::renderItemTexture(std::int16_t id, int x, int y, int w, int h) {
    renderer_->renderTexture(itemTexture_, x, y, w, h,
                             itemTexW_ * (id % itemWCount_), itemTexH_ * (id / itemWCount_),
                             itemTexW_, itemTexH_, true);
}

void Window::updateFixed() {
    audio::gMixer.service();
    const bool wasProcessing = processingStage_;
    processingStage_ = true;
    if (map_) {
        map_->doUpdate();
    }
    if (popup_) {
        popup_->doUpdate();
    }
    processingStage_ = wasProcessing;
    if (!wasProcessing) {
        applyDeferredNodes();
        applyDeferredCommands();
    }
}

void Window::compatibilityUpdate() {
    const bool wasProcessing = processingStage_;
    processingStage_ = true;
    if (map_) {
        map_->advanceCompatibilityFrame();
    }
    processingStage_ = wasProcessing;
    if (!wasProcessing) {
        applyDeferredNodes();
        applyDeferredCommands();
    }
}

void Window::update() {
    updateFixed();
}

void Window::render() {
    const bool wasProcessing = processingStage_;
    processingStage_ = true;
    if (map_) {
        map_->doRender();
    }
    if (popup_) {
        popup_->doRender();
    }
    processingStage_ = wasProcessing;
}

bool Window::flush() {
    const auto now = wallTimeMicros();
    if (!renderer_->canRender()) {
        const auto next = renderer_->nextRenderTime();
        if (next > now) {
            SDL_Delay(static_cast<Uint32>(std::min<std::uint64_t>(
                (next - now) / 1000ULL,
                std::numeric_limits<Uint32>::max())));
        }
        return false;
    }
    renderer_->present();
    if (core::config.showFPS()) {
        static float lastFPS = 0.f;
        float fps = renderer_->fps();
        if (lastFPS != fps) {
            SDL_SetWindowTitle(static_cast<SDL_Window *>(win_),
                               fmt::format("{}     FPS: {}", GameWindowTitle, fps).c_str());
        }
    }
    SDL_Delay(1);
    return true;
}

void Window::defer(std::function<void()> command) {
    if (!command) { return; }
    if (processingStage_) {
        deferredCommands_.emplace_back(std::move(command));
        return;
    }
    command();
}

void Window::applyDeferredCommands() {
    if (processingStage_ || applyingDeferred_) { return; }
    applyingDeferred_ = true;
    while (!deferredCommands_.empty()) {
        auto commands = std::move(deferredCommands_);
        deferredCommands_.clear();
        for (auto &command : commands) {
            if (command) { command(); }
        }
    }
    applyingDeferred_ = false;
}

void Window::applyDeferredNodes() {
    if (map_) {
        map_->applyDeferredDeletes();
        // Persistent maps are owned by Window and are never deleted as a
        // consequence of a child callback. Consume an accidental root request
        // and keep the owner pointer valid until an explicit scene transition.
        (void)map_->consumeDeleteRequest();
    }
    if (!popup_) { return; }
    popup_->applyDeferredDeletes();
    if (!popup_->deleteRequested()) { return; }
    auto *victim = popup_;
    popup_ = nullptr;
    const bool owned = freeOnClose_;
    freeOnClose_ = false;
    (void)victim->consumeDeleteRequest();
    if (owned) {
        delete victim;
    } else {
        victim->close();
    }
}

void Window::title() {
    if (processingStage_) {
        defer([this] { title(); });
        return;
    }
    playMusic(16);
    auto *title = new Title(renderer_, 0, 0, width_, height_);
    title->init();
    freeOnClose_ = true;
    popup_ = title;
}

void Window::endscreen() {
    if (processingStage_) {
        defer([this] { endscreen(); });
        return;
    }
    subMap_->cleanupEvents();
    map_ = nullptr;
    auto *endScreen = new EndScreen(renderer_, 0, 0, width_, height_);
    endScreen->init();
    freeOnClose_ = true;
    popup_ = endScreen;
}

void Window::newGame() {
    if (processingStage_) {
        defer([this] { newGame(); });
        return;
    }
    ::hojy::world::state::gStrings.saveDataLoaded();
    map_ = subMap_;
    dynamic_cast<GlobalMap *>(globalMap_)->load();
    globalMap_->setPosition(::hojy::world::state::gSaveData.baseInfo->mainX, ::hojy::world::state::gSaveData.baseInfo->mainY);
    dynamic_cast<SubMap *>(subMap_)->load(::hojy::content::gFactors.initSubMapId);
    subMap_->setPosition(::hojy::content::gFactors.initSubMapX, ::hojy::content::gFactors.initSubMapY, false);
    dynamic_cast<SubMap *>(subMap_)->forceMainCharTexture(::hojy::content::gFactors.initMainCharTex / 2);
    map_->fadeIn([this] {
        dynamic_cast<SubMap *>(subMap_)->setPosition(::hojy::content::gFactors.initSubMapX, ::hojy::content::gFactors.initSubMapY);
        dynamic_cast<SubMap *>(subMap_)->forceMainCharTexture(::hojy::content::gFactors.initMainCharTex / 2);
        map_->resetFrame();
    });
}

bool Window::loadGame(int slot) {
    if (!::hojy::world::state::gSaveData.load(slot)) { return false; }
    ::hojy::world::state::gStrings.saveDataLoaded();
    dynamic_cast<GlobalMap *>(globalMap_)->load();
    globalMap_->setPosition(::hojy::world::state::gSaveData.baseInfo->mainX, ::hojy::world::state::gSaveData.baseInfo->mainY);
    auto &binfo = ::hojy::world::state::gSaveData.baseInfo;
    if (binfo->subMap > 0) {
        map_ = subMap_;
        dynamic_cast<SubMap *>(subMap_)->load(binfo->subMap - 1);
        subMap_->setPosition(binfo->subX, binfo->subY, false);
        subMap_->setDirection(Map::Direction(binfo->direction));
        map_->fadeIn([this]() {
            dynamic_cast<SubMap *>(subMap_)->setPosition(::hojy::world::state::gSaveData.baseInfo->subX, ::hojy::world::state::gSaveData.baseInfo->subY);
            map_->resetFrame();
        });
    } else {
        globalMap_->setDirection(Map::Direction(binfo->direction));
        map_ = globalMap_;
        map_->resetFrame();
        map_->fadeIn([this]() {
            map_->resetFrame();
        });
    }
    return true;
}

bool Window::saveGame(int slot) {
    auto &binfo = ::hojy::world::state::gSaveData.baseInfo;
    binfo->onShip = dynamic_cast<GlobalMap *>(globalMap_)->onShip();
    binfo->mainX = globalMap_->currX();
    binfo->mainY = globalMap_->currY();
    binfo->subMap = map_->subMapId() + 1;
    if (binfo->subMap > 0) {
        binfo->subX = dynamic_cast<SubMap *>(subMap_)->currX();
        binfo->subY = dynamic_cast<SubMap *>(subMap_)->currY();
    }
    binfo->direction = std::int16_t(dynamic_cast<MapWithEvent *>(map_)->direction());
    return ::hojy::world::state::gSaveData.save(slot);
}

void Window::forceQuit() {
    quitRequested_ = true;
}

void Window::exitToGlobalMap(int direction) {
    if (processingStage_) {
        defer([this, direction] { exitToGlobalMap(direction); });
        return;
    }
    map_->fadeOut([this, direction]() {
        map_ = globalMap_;
        map_->resetFrame();
        dynamic_cast<MapWithEvent *>(map_)->setDirection(Map::Direction(direction));
        map_->fadeIn([this]() {
            map_->resetFrame();
        });
    });
}

void Window::enterSubMap(std::int16_t subMapId, int direction) {
    if (processingStage_) {
        defer([this, subMapId, direction] { enterSubMap(subMapId, direction); });
        return;
    }
    bool switching = map_->subMapId() >= 0;
    map_->fadeOut([this, subMapId, direction, switching]() {
        if (!switching) {
            map_ = subMap_;
        }
        const auto *smi = ::hojy::world::state::gSaveData.subMapInfo[subMapId];
        dynamic_cast<SubMap *>(map_)->load(subMapId);
        if (!switching) {
            subMap_->setDirection(Map::Direction(direction));
        }
        std::int16_t x, y;
        if (switching && smi->subMapEnterX) {
            x = smi->subMapEnterX;
            y = smi->subMapEnterY;
        } else {
            x = smi->enterX;
            y = smi->enterY;
        }
        dynamic_cast<MapWithEvent *>(map_)->setPosition(x, y, false);
        auto *tips = new MessageBox(map_, 0, 0, width_, height_ * 4 / 5);
        tips->popup({GETSUBMAPNAME(subMapId)}, MessageBox::Normal);
        map_->fadeIn([this, tips, x, y] {
            tips->requestDelete();
            dynamic_cast<MapWithEvent *>(map_)->setPosition(x, y);
            map_->resetFrame();
        });
    });
}

bool Window::enterWar(std::int16_t warId, bool getExpOnLose, bool deadOnLose) {
    auto *wf = dynamic_cast<Warfield *>(warfield_);
    if (!wf) { return false; }
    if (!wf->load(warId)) {
        return false;
    }
    wf->setGetExpOnLose(getExpOnLose);
    wf->setDeadOnLose(deadOnLose);
    std::set<std::int16_t> defaultChars;
    if (wf->getDefaultChars(defaultChars)) {
        auto *clm = new CharListMenu(renderer_, 0, 0, gWindow->width(), gWindow->height());
        clm->enableCheckBox(true, [defaultChars](std::int16_t charId) -> bool {
            return defaultChars.find(charId) == defaultChars.end();
        });
        clm->initWithTeamMembers({GETTEXT(70)}, {CharListMenu::LEVEL}, [this, clm](std::int16_t) {
            const auto selectedChars = clm->getSelectedCharIds();
            closePopup();
            auto *wf = dynamic_cast<Warfield *>(warfield_);
            if (!wf) { return; }
            map_ = warfield_;
            if (!wf->putChars(selectedChars)) {
                map_ = subMap_;
                return;
            }
            if (map_ == warfield_) {
                map_->fadeIn();
            }
        }, []() -> bool { return false; });
        for (size_t i = 0; i < clm->charCount(); ++i) {
            if (defaultChars.find(clm->charId(i)) != defaultChars.end()) {
                clm->checkItem(i, true);
            }
        }
        clm->makeCenter(gWindow->width(), gWindow->height() * 4 / 5, 0, 0);
        popup_ = clm;
        freeOnClose_ = true;
    } else {
        map_ = warfield_;
        if (!wf->putChars({})) {
            map_ = subMap_;
            return false;
        }
        if (map_ == warfield_) {
            map_->fadeIn();
        }
    }
    return true;
}

void Window::endWar(bool won, bool instantDie) {
    if (processingStage_) {
        defer([this, won, instantDie] { endWar(won, instantDie); });
        return;
    }
    if (instantDie) {
        playerDie();
        return;
    }
    map_ = subMap_;
    subMap_->fadeIn([this, won]() {
        subMap_->continueEvents(won);
        auto *subMapInfo = ::hojy::world::state::gSaveData.subMapInfo[subMap_->subMapId()];
        if (subMapInfo) {
            auto music = subMapInfo->enterMusic;
            if (music < 0) {
                music = subMapInfo->exitMusic;
            }
            if (music >= 0) {
                gWindow->playMusic(music);
            }
        }
    });
}

void Window::playerDie() {
    if (processingStage_) {
        defer([this] { playerDie(); });
        return;
    }
    subMap_->cleanupEvents();
    map_ = nullptr;
    auto *dead = new Dead(renderer_, 0, 0, width_, height_);
    dead->init();
    freeOnClose_ = true;
    popup_ = dead;
}

void Window::useQuestItem(std::int16_t itemId) {
    if (processingStage_) {
        defer([this, itemId] { useQuestItem(itemId); });
        return;
    }
    auto *mapev = dynamic_cast<MapWithEvent *>(map_);
    if (mapev) mapev->onUseItem(itemId);
}

void Window::forceEvent(std::int16_t eventId) {
    if (processingStage_) {
        defer([this, eventId] { forceEvent(eventId); });
        return;
    }
    auto *mapev = dynamic_cast<MapWithEvent *>(map_);
    if (mapev) mapev->runEvent(eventId);
    else if (subMap_) subMap_->runEvent(eventId);
}

void Window::closePopup() {
    if (processingStage_) {
        defer([this] { closePopup(); });
        return;
    }
    if (!popup_) { return; }
    if (freeOnClose_) {
        delete popup_;
    } else {
        popup_->close();
    }
    popup_ = nullptr;
}

void Window::endPopup(bool close, bool result) {
    if (processingStage_) {
        defer([this, close, result] { endPopup(close, result); });
        return;
    }
    if (close) {
        closePopup();
    }
    auto *mapev = dynamic_cast<MapWithEvent *>(map_);
    if (mapev) mapev->continueEvents(result);
}

}
