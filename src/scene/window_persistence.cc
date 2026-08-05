#include "window.hh"

#include "globalmap.hh"
#include "submap.hh"

#include "content/constants.hh"
#include "content/factors.hh"
#include "world/bag.hh"
#include "world/new_game_transaction.hh"
#include "world/savedata.hh"
#include "world/strings.hh"

#include <optional>
#include <limits>
#include <utility>

namespace hojy::scene {

namespace {

constexpr int GlobalMapWidth = 480;
constexpr int GlobalMapHeight = 480;

bool validGlobalCoordinate(int x, int y) {
    return x >= 0 && x < GlobalMapWidth && y >= 0 && y < GlobalMapHeight;
}

bool validSubMapCoordinate(int x, int y) {
    return x >= 0 && x < ::hojy::content::SubMapWidth
        && y >= 0 && y < ::hojy::content::SubMapHeight;
}

bool validDirection(std::int16_t direction) {
    return direction >= Map::DirUp && direction <= Map::DirDown;
}

bool validSubMapId(const ::hojy::world::state::SaveData &save, std::int16_t id) {
    if (id < 0) { return false; }
    const auto index = static_cast<std::size_t>(id);
    return index < save.subMapInfo.size()
        && index < save.subMapLayerInfo.size()
        && index < save.subMapEventInfo.size()
        && save.subMapInfo[id] != nullptr;
}

}

bool Window::startNewGame(
        ::hojy::world::state::NewGameCandidate &&candidate) {
    if (processingStage_) { return false; }
    auto *global = dynamic_cast<GlobalMap *>(globalMap_);
    auto *sub = dynamic_cast<SubMap *>(subMap_);
    const auto &candidateSave = candidate.saveData();
    const auto *baseInfo = candidateSave.baseInfo.operator->();
    const auto initSubMapId = candidate.initialSubMapId();
    if (!global || !sub || !baseInfo
        || !validGlobalCoordinate(baseInfo->mainX, baseInfo->mainY)
        || !validDirection(baseInfo->direction)
        || !validSubMapId(candidateSave, initSubMapId)
        || !validSubMapCoordinate(::hojy::content::gFactors.initSubMapX,
                                  ::hojy::content::gFactors.initSubMapY)
        || !global->ready() || !sub->ready()) {
        return false;
    }
    const auto candidateGlobalX = baseInfo->mainX;
    const auto candidateGlobalY = baseInfo->mainY;
    const auto candidateDirection = baseInfo->direction;

    const auto previousMap = map_;
    const auto previousGlobalX = global->currX();
    const auto previousGlobalY = global->currY();
    const auto previousGlobalDirection = global->direction();
    const auto previousSubId = sub->subMapId();
    const auto previousSubX = sub->currX();
    const auto previousSubY = sub->currY();
    const auto previousSubDirection = sub->direction();
    const auto commandCheckpoint = deferredCommands_.size();

    auto activation = ::hojy::world::state::activateNewGameCandidate(
        std::move(candidate));
    if (!activation) { return false; }

    const auto restorePreviousState = [&]() -> bool {
        try {
            if (!global->load(previousGlobalX, previousGlobalY)) { return false; }
            global->setDirection(previousGlobalDirection);
            if (previousSubId >= 0) {
                if (!sub->load(previousSubId, false)) { return false; }
            } else if (!sub->unload(false)) {
                return false;
            }
            if (previousSubId >= 0) {
                sub->setDirection(previousSubDirection);
                sub->setPosition(previousSubX, previousSubY, false);
            }
            map_ = previousMap;
            return true;
        } catch (...) {
            return false;
        }
    };

    const auto rollback = [&]() noexcept {
        invalidateTransitions();
        activation->rollback();
        if (!restorePreviousState()) {
            ready_ = false;
            quitRequested_ = true;
        }
        deferredCommands_.discardAfter(commandCheckpoint);
    };

    invalidateTransitions();
    invalidateBattleSession();
    try {
        if (!global->load(candidateGlobalX, candidateGlobalY)
            || !sub->load(initSubMapId, false)) {
            rollback();
            return false;
        }

        global->setDirection(Map::Direction(candidateDirection));
        global->setPosition(candidateGlobalX, candidateGlobalY, false);
        map_ = subMap_;
        const auto token = beginTransition();
        const auto initX = ::hojy::content::gFactors.initSubMapX;
        const auto initY = ::hojy::content::gFactors.initSubMapY;
        const auto mainTexture = ::hojy::content::gFactors.initMainCharTex / 2;
        sub->setDirection(Map::Direction(candidateDirection));
        sub->setPosition(initX, initY, false);
        sub->forceMainCharTexture(mainTexture);
        auto *expected = static_cast<Map *>(sub);
        const auto windowLifetimeHandle = this->windowLifetimeHandle();
        expected->fadeIn([windowLifetimeHandle, token, expected,
                          initX, initY, mainTexture] {
            const auto windowLifetimeState = windowLifetimeHandle.lock();
            if (!windowLifetimeState || !windowLifetimeState->owner) { return; }
            auto *window = windowLifetimeState->owner;
            if (!window->isCurrentTransition(token) || window->map_ != expected
                || window->subMap_ != expected) {
                return;
            }
            auto *subMap = dynamic_cast<SubMap *>(expected);
            if (!subMap || !subMap->validMapCoordinate(initX, initY)) {
                return;
            }
            subMap->setPosition(initX, initY);
            subMap->forceMainCharTexture(mainTexture);
            expected->resetFrame();
        });
        // Invalidate the old event VM only after all staged scene work has
        // succeeded.  A failed candidate therefore leaves its continuation,
        // VM and presentation session untouched.
        sub->cleanupEvents();
        activation->finalize();
        return true;
    } catch (...) {
        rollback();
        return false;
    }
}

bool Window::loadGame(int slot) {
    auto *global = dynamic_cast<GlobalMap *>(globalMap_);
    auto *sub = dynamic_cast<SubMap *>(subMap_);
    if (!global || !sub || !global->ready() || !sub->ready()) { return false; }

    // SaveData::load() is transactional by itself, but scene loading has a
    // second commit point. Keep both the old domain state and the active map
    // coordinates until every resource/coordinate check succeeds.
    ::hojy::world::state::SaveData previousSave;
    ::hojy::world::state::Bag previousBag;
    try {
        previousSave = ::hojy::world::state::gSaveData;
        previousBag = ::hojy::world::state::gBag;
    } catch (...) {
        return false;
    }
    const auto previousMap = map_;
    const auto previousGlobalX = global->currX();
    const auto previousGlobalY = global->currY();
    const auto previousGlobalDirection = global->direction();
    const auto previousSubId = sub->subMapId();
    const auto previousSubX = sub->currX();
    const auto previousSubY = sub->currY();
    const auto previousSubDirection = sub->direction();

    const auto restorePreviousState = [&]() -> bool {
        try {
            ::hojy::world::state::gSaveData = std::move(previousSave);
            ::hojy::world::state::gBag = std::move(previousBag);
            if (!global->load(previousGlobalX, previousGlobalY)) { return false; }
            global->setDirection(previousGlobalDirection);
            if (previousSubId >= 0) {
                if (!sub->load(previousSubId, false)) { return false; }
            } else if (!sub->unload(false)) {
                return false;
            }
            if (previousSubId >= 0) {
                sub->setDirection(previousSubDirection);
                sub->setPosition(previousSubX, previousSubY, false);
            }
            map_ = previousMap;
            return true;
        } catch (...) {
            return false;
        }
    };

    invalidateTransitions();
    if (!::hojy::world::state::gSaveData.load(slot)) { return false; }
    const auto *baseInfo = ::hojy::world::state::gSaveData.baseInfo.operator->();
    const auto validSave = baseInfo
        && validGlobalCoordinate(baseInfo->mainX, baseInfo->mainY)
        && validDirection(baseInfo->direction)
        && (baseInfo->subMap <= 0
            || (baseInfo->subMap - 1 >= 0
                && static_cast<std::size_t>(baseInfo->subMap - 1)
                    < ::hojy::world::state::gSaveData.subMapInfo.size()
                && static_cast<std::size_t>(baseInfo->subMap - 1)
                    < ::hojy::world::state::gSaveData.subMapLayerInfo.size()
                && static_cast<std::size_t>(baseInfo->subMap - 1)
                    < ::hojy::world::state::gSaveData.subMapEventInfo.size()
                && ::hojy::world::state::gSaveData.subMapInfo[baseInfo->subMap - 1]
                && validSubMapCoordinate(baseInfo->subX, baseInfo->subY)));
    if (!validSave || !global->load(baseInfo->mainX, baseInfo->mainY)) {
        if (!restorePreviousState()) {
            ready_ = false;
            quitRequested_ = true;
        }
        return false;
    }

    const auto subMapId = baseInfo->subMap > 0
        ? static_cast<std::int16_t>(baseInfo->subMap - 1) : static_cast<std::int16_t>(-1);
    if (subMapId >= 0 && !sub->load(subMapId)) {
        if (!restorePreviousState()) {
            ready_ = false;
            quitRequested_ = true;
        }
        return false;
    }

    invalidateBattleSession();
    ::hojy::world::state::gStrings.saveDataLoaded();
    if (subMapId >= 0) {
        sub->setDirection(Map::Direction(baseInfo->direction));
        sub->setPosition(baseInfo->subX, baseInfo->subY, false);
        map_ = subMap_;
        const auto token = beginTransition();
        auto *expected = static_cast<Map *>(sub);
        const auto x = baseInfo->subX;
        const auto y = baseInfo->subY;
        const auto windowLifetimeHandle = this->windowLifetimeHandle();
        expected->fadeIn([windowLifetimeHandle, token, expected, x, y]() {
            const auto windowLifetimeState = windowLifetimeHandle.lock();
            if (!windowLifetimeState || !windowLifetimeState->owner) { return; }
            auto *window = windowLifetimeState->owner;
            if (!window->isCurrentTransition(token) || window->map_ != expected
                || window->subMap_ != expected) {
                return;
            }
            auto *subMap = dynamic_cast<SubMap *>(expected);
            if (!subMap || !subMap->validMapCoordinate(x, y)) { return; }
            subMap->setPosition(x, y);
            expected->resetFrame();
        });
    } else {
        global->setDirection(Map::Direction(baseInfo->direction));
        map_ = globalMap_;
        const auto token = beginTransition();
        auto *expected = static_cast<Map *>(global);
        expected->resetFrame();
        const auto windowLifetimeHandle = this->windowLifetimeHandle();
        expected->fadeIn([windowLifetimeHandle, token, expected]() {
            const auto windowLifetimeState = windowLifetimeHandle.lock();
            if (!windowLifetimeState || !windowLifetimeState->owner) { return; }
            auto *window = windowLifetimeState->owner;
            if (!window->isCurrentTransition(token) || window->map_ != expected
                || window->globalMap_ != expected) {
                return;
            }
            expected->resetFrame();
        });
    }
    return true;
}

bool Window::saveGame(int slot) {
    auto *global = dynamic_cast<GlobalMap *>(globalMap_);
    auto *sub = dynamic_cast<SubMap *>(subMap_);
    auto *active = dynamic_cast<MapWithEvent *>(map_);
    if (!ready_ || !global || !sub || !active || !global->ready() || !sub->ready()
        || (map_ != global && map_ != sub)) {
        return false;
    }
    const auto *baseInfo = ::hojy::world::state::gSaveData.baseInfo.operator->();
    const auto subMapId = map_->subMapId();
    if (!baseInfo || !validGlobalCoordinate(global->currX(), global->currY())
        || !validDirection(active->direction())
        || (subMapId >= 0 && (!validSubMapId(::hojy::world::state::gSaveData, subMapId)
                              || !validSubMapCoordinate(sub->currX(), sub->currY())))
        || subMapId >= std::numeric_limits<std::int16_t>::max()) {
        return false;
    }

    ::hojy::world::state::SaveData previousSave;
    ::hojy::world::state::Bag previousBag;
    try {
        previousSave = ::hojy::world::state::gSaveData;
        previousBag = ::hojy::world::state::gBag;
        auto &binfo = ::hojy::world::state::gSaveData.baseInfo;
        binfo->onShip = global->onShip();
        binfo->mainX = global->currX();
        binfo->mainY = global->currY();
        binfo->subMap = subMapId + 1;
        if (subMapId >= 0) {
            binfo->subX = sub->currX();
            binfo->subY = sub->currY();
        }
        binfo->direction = std::int16_t(active->direction());
        if (::hojy::world::state::gSaveData.save(slot)) {
            return true;
        }
    } catch (...) {
        // Restore the candidate below; saving is a best-effort command.
    }
    try {
        ::hojy::world::state::gSaveData = std::move(previousSave);
        ::hojy::world::state::gBag = std::move(previousBag);
    } catch (...) {
        ready_ = false;
        quitRequested_ = true;
    }
    return false;
}

}
