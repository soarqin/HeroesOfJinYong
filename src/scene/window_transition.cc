#include "window.hh"

#include "charlistmenu.hh"
#include "menu_action_adapter.hh"
#include "character_list_snapshot_builder.hh"
#include "dead.hh"
#include "endscreen.hh"
#include "submap.hh"
#include "title.hh"
#include "warfield.hh"
#include "window_command.hh"

#include "content/constants.hh"
#include "world/savedata.hh"
#include "world/strings.hh"

#include <set>
#include <utility>

namespace hojy::scene {

namespace {

bool validSubMapCoordinate(std::int16_t x, std::int16_t y) {
    return x >= 0 && x < ::hojy::content::SubMapWidth
        && y >= 0 && y < ::hojy::content::SubMapHeight;
}

bool validDirection(int direction) {
    return direction >= Map::DirUp && direction <= Map::DirDown;
}

}

void Window::title() {
    if (processingStage_) {
        deferredCommands_.push([](SceneCommandContext &context) { context.title(); });
        return;
    }
    invalidateBattleSession();
    if (!renderer_) {
        ready_ = false;
        quitRequested_ = true;
        return;
    }
    invalidateTransitions();
    playMusic(16);
    auto *title = new Title(renderer_, 0, 0, width_, height_);
    title->setFontSize(renderer_->fontSize());
    bindCommandSink(title);
    if (!title->init()) {
        delete title;
        ready_ = false;
        quitRequested_ = true;
        return;
    }
    replacePopup(title, true);
}

void Window::endscreen() {
    if (processingStage_) {
        deferredCommands_.push([](SceneCommandContext &context) { context.endscreen(); });
        return;
    }
    invalidateBattleSession();
    invalidateTransitions();
    if (!subMap_) {
        ready_ = false;
        quitRequested_ = true;
        return;
    }
    subMap_->cleanupEvents();
    map_ = nullptr;
    auto *endScreen = new EndScreen(renderer_, 0, 0, width_, height_);
    bindCommandSink(endScreen);
    if (!endScreen->init()) {
        delete endScreen;
        ready_ = false;
        quitRequested_ = true;
        return;
    }
    replacePopup(endScreen, true);
}

void Window::forceQuit() {
    quitRequested_ = true;
}

void Window::exitToGlobalMap(int direction) {
    if (processingStage_) {
        deferredCommands_.push([direction](SceneCommandContext &context) {
            context.exitToGlobalMap(direction);
        });
        return;
    }
    auto *source = map_;
    auto *global = globalMap_;
    if (!source || !global || !validDirection(direction)) { return; }
    invalidateBattleSession();
    const auto token = beginTransition();
    const auto windowLifetimeHandle = this->windowLifetimeHandle();
    source->fadeOut([windowLifetimeHandle, token, source, global, direction]() {
        const auto windowLifetimeState = windowLifetimeHandle.lock();
        if (!windowLifetimeState || !windowLifetimeState->owner) { return; }
        auto *window = windowLifetimeState->owner;
        if (!window->isCurrentTransition(token) || window->map_ != source
            || window->globalMap_ != global) {
            return;
        }
        window->map_ = global;
        auto *mapWithEvent = dynamic_cast<MapWithEvent *>(global);
        if (!mapWithEvent) {
            window->invalidateTransitions();
            return;
        }
        mapWithEvent->resetFrame();
        mapWithEvent->setDirection(Map::Direction(direction));
        global->fadeIn([windowLifetimeHandle, token,
                        expected = static_cast<Map *>(global)]() {
            const auto windowLifetimeState = windowLifetimeHandle.lock();
            if (!windowLifetimeState || !windowLifetimeState->owner) { return; }
            auto *window = windowLifetimeState->owner;
            if (!window->isCurrentTransition(token) || window->map_ != expected) {
                return;
            }
            expected->resetFrame();
        });
    });
}

void Window::enterSubMap(std::int16_t subMapId, int direction) {
    if (processingStage_) {
        deferredCommands_.push([subMapId, direction](SceneCommandContext &context) {
            context.enterSubMap(subMapId, direction);
        });
        return;
    }
    auto *subMap = dynamic_cast<SubMap *>(subMap_);
    auto *source = map_;
    if (!subMap || !source || !validDirection(direction) || subMapId < 0
        || static_cast<std::size_t>(subMapId) >= ::hojy::world::state::gSaveData.subMapInfo.size()
        || !::hojy::world::state::gSaveData.subMapInfo[subMapId]) {
        return;
    }
    const bool switching = source->subMapId() >= 0;
    const auto *smi = ::hojy::world::state::gSaveData.subMapInfo[subMapId];
    const bool hasSwitchEntry = validSubMapCoordinate(smi->subMapEnterX, smi->subMapEnterY);
    const std::int16_t x = switching && hasSwitchEntry ? smi->subMapEnterX : smi->enterX;
    const std::int16_t y = switching && hasSwitchEntry ? smi->subMapEnterY : smi->enterY;
    if (!validSubMapCoordinate(x, y) || !subMap->load(subMapId)) { return; }

    invalidateBattleSession();
    const auto music = smi->enterMusic;
    const auto token = beginTransition();
    const auto windowLifetimeHandle = this->windowLifetimeHandle();
    source->fadeOut([windowLifetimeHandle, token, source, subMap, subMapId,
                     direction, switching, x, y, music]() {
        const auto windowLifetimeState = windowLifetimeHandle.lock();
        if (!windowLifetimeState || !windowLifetimeState->owner) { return; }
        auto *window = windowLifetimeState->owner;
        if (!window->isCurrentTransition(token) || window->map_ != source
            || window->subMap_ != subMap
            || subMapId < 0
            || static_cast<std::size_t>(subMapId) >= ::hojy::world::state::gSaveData.subMapInfo.size()
            || !::hojy::world::state::gSaveData.subMapInfo[subMapId]) {
            return;
        }
        source->postCommand(
            [request = SubMapTransitionCompletion{
                token, subMapId, x, y, direction, music, switching
            }](SceneCommandContext &context) mutable {
                context.completeSubMapTransition(std::move(request));
            });
    });
}

void Window::completeSubMapTransition(SubMapTransitionCompletion request) {
    if (!isCurrentTransition(request.transitionToken)
        || !subMap_ || !globalMap_
        || (request.switching ? map_ != subMap_ : map_ != globalMap_)
        || request.subMapId < 0
        || static_cast<std::size_t>(request.subMapId)
            >= ::hojy::world::state::gSaveData.subMapInfo.size()
        || !::hojy::world::state::gSaveData.subMapInfo[request.subMapId]) {
        return;
    }
    auto *mapWithEvent = dynamic_cast<MapWithEvent *>(subMap_);
    if (!mapWithEvent || !mapWithEvent->validMapCoordinate(request.x, request.y)) {
        return;
    }
    if (!request.switching) {
        map_ = subMap_;
        subMap_->setDirection(Map::Direction(request.direction));
    }
    mapWithEvent->setPosition(request.x, request.y, false);

    auto *tips = new MessageBox(map_, 0, 0, width_, height_ * 4 / 5);
    if (!tips) { return; }
    tips->popup({GETSUBMAPNAME(request.subMapId)}, MessageBox::Normal);
    if (request.music >= 0) {
        postSceneCommand(map_, [music = request.music](SceneCommandContext &context) {
            context.playMusic(music);
        });
    }
    const auto expected = map_;
    const auto tipsLifetime = tips->lifetimeHandle();
    const auto windowLifetimeHandle = this->windowLifetimeHandle();
    expected->fadeIn([windowLifetimeHandle, token = request.transitionToken,
                      expected, tipsLifetime, x = request.x, y = request.y] {
        const auto windowLifetimeState = windowLifetimeHandle.lock();
        if (!windowLifetimeState || !windowLifetimeState->owner) { return; }
        auto *window = windowLifetimeState->owner;
        auto tipsState = tipsLifetime.lock();
        if (tipsState && tipsState->owner) {
            tipsState->owner->requestPresentationCleanup();
        }
        if (!window->isCurrentTransition(token) || window->map_ != expected
            || window->subMap_ != expected) {
            return;
        }
        auto *mapWithEvent = dynamic_cast<MapWithEvent *>(expected);
        if (!mapWithEvent || !mapWithEvent->validMapCoordinate(x, y)) { return; }
        mapWithEvent->setPosition(x, y);
        expected->resetFrame();
    });
}

bool Window::enterWar(std::int16_t warId, bool getExpOnLose, bool deadOnLose) {
    invalidateBattleSession();
    invalidateTransitions();
    auto *wf = warfield_;
    if (!wf || !wf->ready() || !wf->load(warId)) {
        if (wf) { wf->abortPresentationState(); }
        return false;
    }
    if (!activateBattleSession(wf)) {
        wf->abortPresentationState();
        return false;
    }
    wf->setGetExpOnLose(getExpOnLose);
    wf->setDeadOnLose(deadOnLose);
    const auto presentationSessionToken = battleSessionToken_;
    const auto windowLifetimeHandle = this->windowLifetimeHandle();
    const auto queueBattleMusic = [this, wf, presentationSessionToken]() {
        const auto music = wf->takePendingBattleMusic();
        if (music < 0) { return; }
        wf->postBattleCommand(
            wf->presentationOwnerHandle(), presentationSessionToken,
            wf->presentationGeneration_,
            [music](SceneCommandContext &context) {
                context.playMusic(music);
            });
    };
    std::set<std::int16_t> defaultChars;
    if (wf->getDefaultChars(defaultChars)) {
        auto *clm = new CharListMenu(renderer_, 0, 0, width_, height_);
        if (!clm) {
            wf->abortPresentationState();
            invalidateBattleSession();
            return false;
        }
        bindCommandSink(clm);
        clm->enableCheckBox(true);
        const auto battleToken = battleSessionToken_;
        auto controller = std::make_shared<ActionMenuController>();
        controller->bind(MenuConfirmEntryId, makeMenuAction(
            [this, windowLifetimeHandle, clm, wf, battleToken,
             queueBattleMusic](MenuSelection) {
            if (popup_ != clm || !isCurrentBattleSession(wf, battleToken)) { return; }
            const auto selectedChars = clm->getSelectedCharIds();
            closePopup();
            if (!wf->ready()) {
                wf->abortPresentationState();
                invalidateBattleSession();
                return;
            }
            const auto previousMap = map_;
            map_ = warfield_;
            const auto battleToken = beginTransition();
            if (!wf->putChars(selectedChars)) {
                map_ = previousMap;
                wf->abortPresentationState();
                invalidateBattleSession();
                invalidateTransitions();
                return;
            }
            queueBattleMusic();
            if (map_ == warfield_) {
                auto *expected = map_;
                expected->fadeIn([windowLifetimeHandle, battleToken, expected] {
                    const auto windowLifetimeState = windowLifetimeHandle.lock();
                    if (!windowLifetimeState || !windowLifetimeState->owner) { return; }
                    auto *window = windowLifetimeState->owner;
                    if (!window->isCurrentTransition(battleToken)
                        || window->map_ != expected) {
                        return;
                    }
                    expected->resetFrame();
                });
            }
        }));
        controller->bindDefault(makeMenuAction(
            [](MenuSelection) {}));
        controller->bindCancel(makeMenuAction(
            [](MenuSelection) {}));
        clm->init(buildCharacterListSnapshot(
                      {GETTEXT(70)}, teamCharacterSources(),
                      {levelProjection()}), std::move(controller));
        for (size_t i = 0; i < clm->charCount(); ++i) {
            const auto charId = clm->charId(i);
            if (defaultChars.find(charId) != defaultChars.end()) {
                clm->setEntryEnabledById(charId, false);
                clm->checkItem(i, true);
            }
        }
        clm->makeCenter(width_, height_ * 4 / 5, 0, 0);
        replacePopup(clm, true);
    } else {
        const auto previousMap = map_;
        map_ = warfield_;
        const auto battleToken = beginTransition();
        if (!wf->putChars({})) {
            map_ = previousMap;
            wf->abortPresentationState();
            invalidateBattleSession();
            invalidateTransitions();
            return false;
        }
        queueBattleMusic();
        if (map_ == warfield_) {
            auto *expected = map_;
            expected->fadeIn([windowLifetimeHandle, battleToken, expected] {
                const auto windowLifetimeState = windowLifetimeHandle.lock();
                if (!windowLifetimeState || !windowLifetimeState->owner) { return; }
                auto *window = windowLifetimeState->owner;
                if (!window->isCurrentTransition(battleToken)
                    || window->map_ != expected) {
                    return;
                }
                expected->resetFrame();
            });
        }
    }
    return true;
}

void Window::endWar(bool won, bool instantDie) {
    BattleEndRequest request{battleSessionToken_, won, instantDie};
    request.actionGeneration = warfield_ ? warfield_->presentationGeneration_ : 0;
    request.expectedStage = BattlePresentationStage::FinishMessages;
    endWar(std::move(request));
}

void Window::endWar(BattleEndRequest request) {
    if (processingStage_) {
        deferredCommands_.push([request](SceneCommandContext &context) {
            context.endWar(request);
        });
        return;
    }
    if (!battlePresentationAvailable(
            request.sessionToken, request.actionGeneration,
            BattlePresentationStage::FinishMessages)) {
        return;
    }
    if (warfield_) {
        warfield_->abortPresentationState();
    }
    invalidateBattleSession();
    if (request.instantDie) {
        playerDie();
        return;
    }
    auto *source = subMap_;
    if (!source || !source->ready()) {
        ready_ = false;
        quitRequested_ = true;
        return;
    }
    map_ = source;
    const auto token = beginTransition();
    const auto won = request.won;
    const auto windowLifetimeHandle = this->windowLifetimeHandle();
    source->fadeIn([windowLifetimeHandle, token, source, won]() {
        const auto windowLifetimeState = windowLifetimeHandle.lock();
        if (!windowLifetimeState || !windowLifetimeState->owner) { return; }
        auto *window = windowLifetimeState->owner;
        source->postCommand([request = BattleTransitionCompletion{token, won}](
                                 SceneCommandContext &context) mutable {
            context.completeBattleTransition(std::move(request));
        });
    });
}

void Window::completeBattleTransition(BattleTransitionCompletion request) {
    if (!isCurrentTransition(request.transitionToken)
        || map_ != subMap_ || !subMap_ || !subMap_->ready()) {
        return;
    }
    continueEvent(request.won);
    const auto subMapId = subMap_->subMapId();
    if (subMapId < 0 || static_cast<std::size_t>(subMapId)
            >= ::hojy::world::state::gSaveData.subMapInfo.size()) {
        return;
    }
    auto *subMapInfo = ::hojy::world::state::gSaveData.subMapInfo[subMapId];
    if (!subMapInfo) { return; }
    auto music = subMapInfo->enterMusic;
    if (music < 0) { music = subMapInfo->exitMusic; }
    if (music >= 0) {
        postSceneCommand(subMap_, [music](SceneCommandContext &context) {
            context.playMusic(music);
        });
    }
}

void Window::abortBattle(BattleAbortRequest request) {
    if (processingStage_) {
        deferredCommands_.push([request](SceneCommandContext &context) {
            context.abortBattle(request);
        });
        return;
    }
    if (!ownsBattleSession(warfield_, request.sessionToken)
        || map_ != warfield_) {
        return;
    }
    if (warfield_) {
        warfield_->abortPresentationState();
    }
    invalidateBattleSession();
    if (request.instantDie) {
        playerDie();
        return;
    }
    auto *source = subMap_;
    if (!source || !source->ready()) {
        ready_ = false;
        quitRequested_ = true;
        return;
    }
    map_ = source;
    const auto token = beginTransition();
    const auto windowLifetimeHandle = this->windowLifetimeHandle();
    source->fadeIn([windowLifetimeHandle, token, source]() {
        const auto windowLifetimeState = windowLifetimeHandle.lock();
        if (!windowLifetimeState || !windowLifetimeState->owner) { return; }
        auto *window = windowLifetimeState->owner;
        source->postCommand([request = BattleTransitionCompletion{token, false}](
                                 SceneCommandContext &context) mutable {
            context.completeBattleTransition(std::move(request));
        });
    });
}

void Window::playerDie() {
    if (processingStage_) {
        deferredCommands_.push([](SceneCommandContext &context) { context.playerDie(); });
        return;
    }
    invalidateBattleSession();
    invalidateTransitions();
    if (!subMap_) {
        ready_ = false;
        quitRequested_ = true;
        return;
    }
    subMap_->cleanupEvents();
    map_ = nullptr;
    auto *dead = new Dead(renderer_, 0, 0, width_, height_);
    bindCommandSink(dead);
    if (!dead->init()) {
        delete dead;
        ready_ = false;
        quitRequested_ = true;
        return;
    }
    replacePopup(dead, true);
}

}
