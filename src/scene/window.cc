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

#include "extendednode.hh"
#include "colorpalette.hh"
#include "globalmap.hh"
#include "submap.hh"
#include "warfield.hh"
#include "effect.hh"
#include "audio/mixer.hh"
#include "content/constants.hh"
#include "content/grpdata.hh"
#include "core/config.hh"

#include <SDL.h>
#include <fmt/xchar.h>
#include <limits>
#include <algorithm>
#include <cstring>
#include <memory>
#include <new>

namespace hojy::scene {

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

Window::Window(int w, int h):
    width_(w), height_(h),
    lifetimeState_(std::make_shared<WindowLifetimeState>()),
    currTime_(wallTimeMicros()) {
    lifetimeState_->owner = this;
    if (w <= 0 || h <= 0) {
        ready_ = false;
        quitRequested_ = true;
        return;
    }
    if (!SDL_WasInit(SDL_INIT_VIDEO)) {
        if (SDL_Init(SDL_INIT_VIDEO) != 0) {
            ready_ = false;
            quitRequested_ = true;
            return;
        }
        ownsVideoSubsystem_ = true;
    }
    // Controller hot-plug support is optional.  On Windows, joystick
    // notification setup can fail even though the video subsystem and window
    // are fully usable; do not turn that optional failure into a startup
    // failure.
    bool controllerReady = SDL_WasInit(SDL_INIT_GAMECONTROLLER) != 0;
    if (!controllerReady) {
        controllerReady = SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0;
        ownsControllerSubsystem_ = controllerReady;
    }
    if (controllerReady) {
        SDL_GameControllerEventState(SDL_ENABLE);
    }
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
    if (!win) {
        ready_ = false;
        quitRequested_ = true;
        return;
    }
    win_ = win;

    renderer_ = new (std::nothrow) Renderer(win_, w, h);
    if (!renderer_ || !renderer_->ready()) {
        ready_ = false;
        quitRequested_ = true;
        return;
    }
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
    if (!globalMap_ || !subMap_
        || !globalMap_->ready() || !subMap_->ready()) {
        ready_ = false;
        return;
    }
    if (!warfield_ || !warfield_->ready()) {
        ready_ = false;
        return;
    }
    warfield_->setHeadTextureProvider(
        [this](std::int16_t id) { return headTexture(id); });
    const auto commandSink = [this](std::unique_ptr<SceneCommand> command) {
        deferredCommands_.push(std::move(command));
    };
    globalMap_->setCommandSink(commandSink);
    subMap_->setCommandSink(commandSink);
    warfield_->setCommandSink(commandSink);

    if (!initializeItemAtlas()) {
        // The atlas is a derived presentation cache.  It is safe to leave it
        // unavailable; item views already fall back when no atlas is bound.
    }
    SDL_ShowWindow(win);
    if (audio::gMixer.init(3)) {
        audio::gMixer.pause(false);
    }
    title();
}

bool Window::initializeItemAtlas() {
    if (!globalMap_ || !renderer_) { return false; }

    const auto &firstData = globalMap_->texData(::hojy::content::ItemTexIdStart);
    if (!Texture::validateRLE(firstData)) { return false; }

    std::int16_t firstHeader[4]{};
    std::memcpy(firstHeader, firstData.data(), sizeof(firstHeader));
    const int itemTexW = firstHeader[0];
    const int itemTexH = firstHeader[1];
    if (itemTexW <= 0 || itemTexH <= 0 || itemTexW > 1024) { return false; }

    const int itemWCount = 1024 / itemTexW;
    if (itemWCount <= 0) { return false; }
    const int itemHCount = (::hojy::content::BagItemCount + itemWCount - 1) / itemWCount;
    const auto atlasWidth = static_cast<std::int64_t>(itemTexW) * itemWCount;
    const auto atlasHeight = static_cast<std::int64_t>(itemTexH) * itemHCount;
    if (atlasWidth <= 0 || atlasWidth > std::numeric_limits<std::int16_t>::max()
        || atlasHeight <= 0 || atlasHeight > std::numeric_limits<std::int16_t>::max()) {
        return false;
    }

    std::unique_ptr<Texture> candidate(Texture::create(
        renderer_, static_cast<std::int16_t>(atlasWidth), static_cast<std::int16_t>(atlasHeight)));
    if (!candidate || !candidate->enableBlendMode(true)) { return false; }

    int pitch = 0;
    TextureLock lock(candidate.get(), pitch);
    if (!lock.valid() || pitch < atlasWidth) { return false; }
    const auto pixelCount = static_cast<std::size_t>(pitch) * static_cast<std::size_t>(atlasHeight);
    std::fill_n(lock.pixels(), pixelCount, 0U);

    const auto *colors = gNormalPalette.colors();
    for (int i = 0; i < ::hojy::content::BagItemCount; ++i) {
        const auto &data = globalMap_->texData(::hojy::content::ItemTexIdStart + i);
        if (data.empty()) { continue; }
        if (!Texture::validateRLE(data)) { return false; }
        std::int16_t header[4]{};
        std::memcpy(header, data.data(), sizeof(header));
        if (header[0] != itemTexW || header[1] != itemTexH) {
            return false;
        }
        if (!Texture::renderRLE(data,
                                colors,
                                lock.pixels(),
                                pitch,
                                static_cast<int>(atlasHeight),
                                itemTexW * (i % itemWCount),
                                itemTexH * (i / itemWCount))) {
            return false;
        }
    }
    lock.unlock();

    auto *previous = itemTexture_;
    itemTexture_ = candidate.release();
    itemTexW_ = itemTexW;
    itemTexH_ = itemTexH;
    itemWCount_ = itemWCount;
    itemHCount_ = itemHCount;
    renderer_->setItemAtlas(itemTexture_, itemTexW_, itemTexH_);
    delete previous;
    return true;
}

Window::~Window() {
    if (lifetimeState_) {
        lifetimeState_->owner = nullptr;
        lifetimeState_.reset();
    }
    invalidateBattleSession();
    eventOverlay_ = nullptr;
    eventOverlayOwner_ = nullptr;
    eventOverlaySession_ = 0;
    eventFadeOwner_ = nullptr;
    eventFadeSession_ = 0;
    if (deferredPopup_) {
        if (deferredPopupOwned_) {
            delete deferredPopup_;
        } else {
            deferredPopup_->close();
        }
        deferredPopup_ = nullptr;
    }
    if (popup_) { closePopup(); }
    headTextureMgr_.clear();
    gEffect.clear();
    delete itemTexture_;
    delete talkBox_;
    delete mainMenu_;
    mainMenu_ = nullptr;
    delete globalMap_;
    delete subMap_;
    delete warfield_;
    delete renderer_;
    SDL_DestroyWindow(static_cast<SDL_Window *>(win_));
    if (ownsControllerSubsystem_) { SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER); }
    if (ownsVideoSubsystem_) { SDL_QuitSubSystem(SDL_INIT_VIDEO); }
}

void Window::setSimulationTime(std::uint64_t timestamp) {
    currTime_ = timestamp;
    if (globalMap_) { globalMap_->setPhaseTime(timestamp); }
    if (subMap_) { subMap_->setPhaseTime(timestamp); }
    if (warfield_) { warfield_->setPhaseTime(timestamp); }
    if (popup_) { popup_->setPhaseTime(timestamp); }
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
    // Direct callers still get the same routing behavior as Application:
    // popup input is consumed immediately, while map input is staged below.
    updateInput();
    if (quitRequested_) { return; }

    // Input collection is intentionally side-effect free: platform events
    // are translated to value intents and retained until the fixed-logic
    // sub-phase below. No scene consumer runs while this loop is active.
    while (!pendingInputEvents_.empty() && !quitRequested_) {
        auto event = std::move(pendingInputEvents_.front());
        pendingInputEvents_.pop_front();
        if (event.action == core::InputAction::Quit) {
            quitRequested_ = true;
            pendingInputEvents_.clear();
            break;
        }
        if (auto intent = makeIntent(event)) {
            if (popup_ || map_) {
                inputPort_.enqueue(std::move(intent));
            }
        }
    }

    if (quitRequested_) { return; }

    // Deliver and consume intents only from fixed logic. Keeping the
    // command/node barrier after this sub-phase preserves input ordering while
    // preventing an input event from executing movement or UI work inline.
    if (!inputPort_.empty()) {
        const bool wasProcessing = processingStage_;
        processingStage_ = true;
        try {
            while (!inputPort_.empty() && !quitRequested_) {
                // Resolve focus for every intent. The previous intent may
                // have closed or replaced the popup during its barrier.
                auto *target = popup_ ? popup_ : map_;
                if (!target) { break; }

                inputPort_.deliverNext(*target);
                target->dispatchInputLogic();

                processingStage_ = wasProcessing;
                if (!wasProcessing) {
                    applyDeferredNodes();
                    applyDeferredCommands();
                }
                processingStage_ = true;
            }
        } catch (...) {
            processingStage_ = wasProcessing;
            throw;
        }
        processingStage_ = wasProcessing;
    }

    audio::gMixer.service();
    const bool wasProcessing = processingStage_;
    processingStage_ = true;
    try {
    if (map_) {
        map_->doUpdate();
    }
    if (popup_) {
        popup_->doUpdate();
    }
    } catch (...) {
        processingStage_ = wasProcessing;
        throw;
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
    try {
        if (map_) {
            map_->advanceCompatibilityFrame();
        }
    } catch (...) {
        processingStage_ = wasProcessing;
        throw;
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

bool Window::prepareRender() {
    if (!ready_ || !renderer_ || !renderer_->ready()) {
        return false;
    }
    try {
        preparePresentationCleanup();
        if (map_) {
            map_->dispatchPrepareRender();
        }
        if (popup_) {
            popup_->dispatchPrepareRender();
        }
        // Presentation nodes that fixed logic marked for cleanup are now
        // removed while the preparation phase owns the node tree.
        applyPresentationCleanup();
    } catch (...) {
        return false;
    }
    return true;
}

void Window::render() const {
    if (map_) {
        map_->doRender();
    }
    if (popup_) {
        popup_->doRender();
    }
}

bool Window::flush() {
    const auto now = wallTimeMicros();
    if (!renderer_->canRender(now)) {
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

void Window::applyDeferredCommands() {
    if (processingStage_ || applyingDeferred_) { return; }
    applyingDeferred_ = true;
    try {
        deferredCommands_.executeGeneration(*this);
    } catch (...) {
        applyingDeferred_ = false;
        throw;
    }
    applyingDeferred_ = false;
}

void Window::applyDeferredNodes() {
    if (eventOverlay_ && eventOverlay_->deleteRequested()) {
        eventOverlay_ = nullptr;
        eventOverlayOwner_ = nullptr;
        eventOverlaySession_ = 0;
    }
    if (map_) {
        map_->applyDeferredDeletes();
        // Persistent maps are owned by Window and are never deleted as a
        // consequence of a child callback. Consume an accidental root request
        // and keep the owner pointer valid until an explicit scene transition.
        (void)map_->consumeDeleteRequest();
    }
    if (deferredPopup_) {
        auto *victim = deferredPopup_;
        const bool owned = deferredPopupOwned_;
        deferredPopup_ = nullptr;
        deferredPopupOwned_ = false;
        victim->applyDeferredDeletes();
        (void)victim->consumeDeleteRequest();
        if (owned) {
            delete victim;
        } else {
            victim->close();
        }
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

void Window::applyPresentationCleanup() noexcept {
    // The same ownership routine is safe after a prepare traversal; the
    // phase-specific name keeps command-barrier APIs out of render code.
    applyDeferredNodes();
}

void Window::bindCommandSink(Node *node) {
    if (!node) { return; }
    node->setCommandSink([this](std::unique_ptr<SceneCommand> command) {
        deferredCommands_.push(std::move(command));
    });
}

std::uint64_t Window::beginTransition() noexcept {
    ++transitionGeneration_;
    if (transitionGeneration_ == 0) {
        transitionGeneration_ = 1;
    }
    return transitionGeneration_;
}

bool Window::isCurrentTransition(std::uint64_t token) const noexcept {
    return token != 0 && token == transitionGeneration_;
}

void Window::invalidateTransitions() noexcept {
    (void)beginTransition();
}

void Window::invalidateBattleSession() noexcept {
    auto *owner = battleSessionOwner_;
    battleSessionOwner_ = nullptr;
    battleSessionToken_ = 0;
    if (owner) {
        owner->abortPresentationState();
    }
}

bool Window::activateBattleSession(Warfield *owner) noexcept {
    invalidateBattleSession();
    if (!owner || !owner->beginPresentationSession()) {
        return false;
    }
    battleSessionOwner_ = owner;
    battleSessionToken_ = owner->presentationSessionToken();
    return battleSessionToken_ != 0;
}

bool Window::isCurrentBattleSession(
        const Warfield *owner, std::uint64_t token) const noexcept {
    return ownsBattleSession(owner, token)
        && owner->isCurrentPresentationSession(token);
}

bool Window::ownsBattleSession(
        const Warfield *owner, std::uint64_t token) const noexcept {
    return owner && owner == battleSessionOwner_
        && token != 0 && token == battleSessionToken_;
}

}
