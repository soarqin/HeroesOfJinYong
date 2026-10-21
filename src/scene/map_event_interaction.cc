#include "mapwithevent.hh"
#include "logic/submap_contract.hh"

#include "window_command.hh"
#include "content/constants.hh"
#include "content/event.hh"
#include "world/action.hh"
#include "world/bag.hh"
#include "world/savedata.hh"
#include "world/strings.hh"
#include "util/random.hh"

#include <fmt/xchar.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <list>
#include <string>
#include <utility>
#include <vector>

namespace hojy::scene {
namespace {

bool validSubMapStorage(std::int16_t id, bool needLayer, bool needEvents) {
    if (id < 0) { return false; }
    const auto index = static_cast<std::size_t>(id);
    const auto &save = ::hojy::world::state::gSaveData;
    if (index >= save.subMapInfo.size() || !save.subMapInfo[index]) { return false; }
    if (needLayer && index >= save.subMapLayerInfo.size()) {
        return false;
    }
    if (needEvents && index >= save.subMapEventInfo.size()) {
        return false;
    }
    return true;
}

bool validEventRef(std::int16_t subMapId, std::int16_t eventId) {
    return validSubMapStorage(subMapId, true, true)
        && eventId >= 0 && eventId < ::hojy::content::SubMapEventCount;
}

bool validCell(const MapWithEvent *map, std::int16_t x, std::int16_t y) {
    return map && x >= 0 && y >= 0
        && x < ::hojy::content::SubMapWidth
        && y < ::hojy::content::SubMapHeight;
}

bool validCharacter(std::int16_t id) {
    return id >= 0 && static_cast<std::size_t>(id)
        < ::hojy::world::state::gSaveData.charInfo.size()
        && ::hojy::world::state::gSaveData.charInfo[id] != nullptr;
}

bool checkedAdd(std::int16_t current, std::int16_t delta,
                std::int16_t &result) {
    const auto value = static_cast<int>(current) + static_cast<int>(delta);
    if (value < std::numeric_limits<std::int16_t>::min()
        || value > std::numeric_limits<std::int16_t>::max()) {
        return false;
    }
    result = static_cast<std::int16_t>(value);
    return true;
}

}

bool MapWithEvent::closePopup(MapWithEvent *map) {
    // The VM callback only records the side effect; execution is deferred to
    // the fixed-update command barrier.
    postSceneCommand(map, [](SceneCommandContext &context) { context.closePopup(); });
    return true;
}

bool MapWithEvent::doTalk(MapWithEvent *map, std::int16_t talkId, std::int16_t headId, std::int16_t position) {
    postSceneCommand(map, [talkId, headId, position](SceneCommandContext &context) {
        context.runTalk(::hojy::content::gEvent.talk(talkId), headId, position);
    });
    return false;
}

bool MapWithEvent::addItem(MapWithEvent *map, std::int16_t itemId, std::int16_t itemCount) {
    ::hojy::world::state::gBag.add(itemId, itemCount);
    postSceneCommand(map, [itemId, itemCount](SceneCommandContext &context) {
        context.showMessage({fmt::format(L"{} {}x{}", GETTEXT(71), GETITEMNAME(itemId), std::to_wstring(itemCount))}, ScenePopupType::PressToCloseTop);
    });
    return false;
}

bool MapWithEvent::modifyEvent(MapWithEvent *map, std::int16_t subMapId, std::int16_t eventId, std::int16_t blocked,
                               std::int16_t index, std::int16_t event1, std::int16_t event2, std::int16_t event3,
                               std::int16_t currTex, std::int16_t endTex, std::int16_t begTex, std::int16_t texDelay,
                               std::int16_t x, std::int16_t y) {
    if (subMapId < 0) { subMapId = map->subMapId_; }
    if (eventId < 0) { eventId = map->currEventId_; }
    if (!validEventRef(subMapId, eventId)) { return true; }
    auto &eventTable = ::hojy::world::state::gSaveData.subMapEventInfo[subMapId]->events;
    auto &layer = ::hojy::world::state::gSaveData.subMapLayerInfo[subMapId]->data[3];
    auto &ev = eventTable[eventId];
    auto candidate = ev;
    if (blocked > -2) { candidate.blocked = blocked; }
    if (index > -2) { candidate.index = index; }
    if (event1 > -2) { candidate.event[0] = event1; }
    if (event2 > -2) { candidate.event[1] = event2; }
    if (event3 > -2) { candidate.event[2] = event3; }
    if (endTex > -2) { candidate.endTex = endTex; }
    if (begTex > -2) { candidate.begTex = begTex; }
    if (texDelay > -2) { candidate.texDelay = texDelay; }
    if (x < 0) { x = candidate.x; }
    if (y < 0) { y = candidate.y; }
    if (!validCell(map, x, y)) { return true; }
    if (candidate.x != x || candidate.y != y) {
        if (!validCell(map, candidate.x, candidate.y)) { return true; }
        const auto oldIndex = static_cast<std::size_t>(candidate.y)
            * ::hojy::content::SubMapWidth + static_cast<std::size_t>(candidate.x);
        const auto newIndex = static_cast<std::size_t>(y)
            * ::hojy::content::SubMapWidth + static_cast<std::size_t>(x);
        if (layer[oldIndex] != eventId || (layer[newIndex] >= 0 && layer[newIndex] != eventId)) {
            return true;
        }
        candidate.x = x;
        candidate.y = y;
    }
    if (currTex > -2) { candidate.currTex = currTex; }

    std::vector<std::int16_t> candidateLayer(
        layer, layer + ::hojy::content::SubMapWidth * ::hojy::content::SubMapHeight);
    if (candidate.x != ev.x || candidate.y != ev.y) {
        const auto oldIndex = static_cast<std::size_t>(ev.y)
            * ::hojy::content::SubMapWidth + static_cast<std::size_t>(ev.x);
        const auto newIndex = static_cast<std::size_t>(candidate.y)
            * ::hojy::content::SubMapWidth + static_cast<std::size_t>(candidate.x);
        candidateLayer[oldIndex] = -1;
        candidateLayer[newIndex] = eventId;
    }
    logic::SubMapStateSnapshot snapshot;
    auto candidateEventData = *::hojy::world::state::gSaveData
        .subMapEventInfo[subMapId].operator->();
    candidateEventData.events[eventId] = candidate;
    if (!map->validateSubMapStateCandidate(
            subMapId, candidateEventData, candidateLayer, snapshot)) {
        return true;
    }
    ev = candidate;
    std::copy(candidateLayer.begin(), candidateLayer.end(), layer);
    map->synchronizeCommittedSubMapState(subMapId, snapshot);
    return true;
}

int MapWithEvent::useItem(MapWithEvent *map, std::int16_t itemId) {
    return itemId == map->currEventItem_ ? 1 : 0;
}

int MapWithEvent::askForWar(MapWithEvent *map) {
    postSceneCommand(map, [](SceneCommandContext &context) { context.showMessage({GETTEXT(72)}, ScenePopupType::YesNo); });
    return -1;
}

bool MapWithEvent::exitEventList(MapWithEvent *map) {
    postSceneCommand(map, [](SceneCommandContext &context) { context.closePopup(); });
    return true;
}

bool MapWithEvent::changeExitMusic(MapWithEvent *map, std::int16_t music) {
    if (!validSubMapStorage(map->subMapId_, false, false)) { return true; }
    ::hojy::world::state::gSaveData.subMapInfo[map->subMapId_]->exitMusic = music;
    return true;
}

int MapWithEvent::askForJoinTeam(MapWithEvent *map) {
    postSceneCommand(map, [](SceneCommandContext &context) { context.showMessage({GETTEXT(73)}, ScenePopupType::YesNo); });
    return -1;
}

bool MapWithEvent::joinTeam(MapWithEvent *map, std::int16_t charId) {
    if (!map || !validCharacter(charId) || !::hojy::world::state::gSaveData.baseInfo.operator->()) {
        return true;
    }
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    auto candidateBase = *::hojy::world::state::gSaveData.baseInfo.operator->();
    auto candidateCharacter = *charInfo;
    int emptySlot = -1;
    for (int index = 0; index < ::hojy::content::TeamMemberCount; ++index) {
        if (candidateBase.members[index] < 0) {
            emptySlot = index;
            break;
        }
    }
    if (emptySlot < 0) { return true; }
    std::vector<std::pair<std::int16_t, std::int16_t>> pendingItems;
    pendingItems.reserve(::hojy::content::CarryItemCount);
    for (int index = 0; index < ::hojy::content::CarryItemCount; ++index) {
        const auto itemId = candidateCharacter.item[index];
        if (itemId < 0) { continue; }
        if (static_cast<std::size_t>(itemId)
                >= ::hojy::world::state::gSaveData.itemInfo.size()
            || candidateCharacter.itemCount[index] < 0) {
            return true;
        }
        const auto itemCount = candidateCharacter.itemCount[index] == 0
            ? std::int16_t{1} : candidateCharacter.itemCount[index];
        pendingItems.emplace_back(itemId, itemCount);
        candidateCharacter.itemCount[index] = 0;
        candidateCharacter.item[index] = -1;
    }
    candidateBase.members[emptySlot] = charId;
    std::list<std::function<bool()>> candidatePending;
    for (const auto &[itemId, itemCount]: pendingItems) {
        candidatePending.emplace_back([map, itemId, itemCount]() -> bool {
            return addItem(map, itemId, itemCount);
        });
    }
    *::hojy::world::state::gSaveData.baseInfo.operator->() = candidateBase;
    *charInfo = candidateCharacter;
    map->pendingSubEvents_.splice(map->pendingSubEvents_.end(), candidatePending);
    return true;
}

int MapWithEvent::wantSleep(MapWithEvent *map) {
    postSceneCommand(map, [](SceneCommandContext &context) { context.showMessage({GETTEXT(74)}, ScenePopupType::YesNo); });
    return -1;
}

bool MapWithEvent::sleep(MapWithEvent *map) {
    for (auto id: ::hojy::world::state::gSaveData.baseInfo->members) {
        if (id < 0) { continue; }
        auto *charInfo = ::hojy::world::state::gSaveData.charInfo[id];
        if (!charInfo) { continue; }
        charInfo->stamina = ::hojy::content::StaminaMax;
        charInfo->hp = charInfo->maxHp;
        charInfo->mp = charInfo->maxMp;
        charInfo->hurt = 0;
        charInfo->poisoned = 0;
    }
    return true;
}

bool MapWithEvent::makeBright(MapWithEvent *map) {
    const auto session = map ? map->eventSessionToken() : 0;
    if (!map || session == 0) { return true; }
    const auto token = map->beginEventContinuation();
    auto continuation = map->createEventInputContinuation(token, 0, false);
    postSceneCommand(map, [session, token, continuation = std::move(continuation)](
                              SceneCommandContext &context) mutable {
        context.fadeEventIn({session, token, std::move(continuation)});
    });
    return false;
}

bool MapWithEvent::makeDim(MapWithEvent *map) {
    const auto session = map ? map->eventSessionToken() : 0;
    if (!map || session == 0) { return true; }
    const auto token = map->beginEventContinuation();
    auto continuation = map->createEventInputContinuation(token, 0, false);
    postSceneCommand(map, [session, token, continuation = std::move(continuation)](
                              SceneCommandContext &context) mutable {
        context.fadeEventOut({session, token, std::move(continuation)});
    });
    return false;
}

bool MapWithEvent::die(MapWithEvent *map) {
    postSceneCommand(map, [](SceneCommandContext &context) { context.playerDie(); });
    return false;
}

int MapWithEvent::checkTeamMember(MapWithEvent *map, std::int16_t charId) {
    for (auto id: ::hojy::world::state::gSaveData.baseInfo->members) {
        if (id == charId) {
            return 1;
        }
    }
    return 0;
}

bool MapWithEvent::changeLayer(MapWithEvent *map, std::int16_t subMapId, std::int16_t layer,
                               std::int16_t x, std::int16_t y, std::int16_t value) {
    if (!map) { return true; }
    if (subMapId < 0) {
        subMapId = map->subMapId_;
    }
    if (!validSubMapStorage(subMapId, true, false)
        || layer < 0 || layer >= ::hojy::content::SubMapLayerCount
        || !validCell(map, x, y) || value < -1) {
        return true;
    }
    if (layer == 3 && static_cast<std::size_t>(subMapId)
            >= ::hojy::world::state::gSaveData.subMapEventInfo.size()) {
        return true;
    }
    const auto cellIndex = static_cast<std::size_t>(y)
        * ::hojy::content::SubMapWidth + static_cast<std::size_t>(x);
    auto &layerData = ::hojy::world::state::gSaveData.subMapLayerInfo[subMapId]
        ->data[layer];
    std::vector<std::int16_t> candidateLayer(
        layerData,
        layerData + ::hojy::content::SubMapWidth * ::hojy::content::SubMapHeight);
    candidateLayer[cellIndex] = value;
    if (layer == 3) {
        if (static_cast<std::size_t>(subMapId)
                >= ::hojy::world::state::gSaveData.subMapEventInfo.size()) {
            return true;
        }
        auto *eventInfo = ::hojy::world::state::gSaveData
            .subMapEventInfo[subMapId].operator->();
        if (!eventInfo) { return true; }
        logic::SubMapStateSnapshot snapshot;
        if (!map->validateSubMapStateCandidate(
                subMapId, *eventInfo, candidateLayer, snapshot)) {
            return true;
        }
        std::copy(candidateLayer.begin(), candidateLayer.end(), layerData);
        map->synchronizeCommittedSubMapState(subMapId, snapshot);
        return true;
    } else if (subMapId == map->subMapId_ && value >= 0
               && map->texData(value / 2).empty()) {
        return true;
    }
    std::copy(candidateLayer.begin(), candidateLayer.end(), layerData);
    if (subMapId == map->subMapId_) {
        map->setCellSpriteId(x, y, layer, value < 0 ? -1 : value / 2);
    }
    return true;
}

int MapWithEvent::hasItem(MapWithEvent *map, std::int16_t itemId) {
    if (itemId < 0 || itemId >= ::hojy::content::BagItemCount) { return 0; }
    return ::hojy::world::state::gBag[itemId] > 0 ? 1 : 0;
}

bool MapWithEvent::setPlayerPosition(MapWithEvent *map, std::int16_t x, std::int16_t y) {
    if (!map->validMapCoordinate(x, y)) { return true; }
    map->setPosition(x, y, false);
    return true;
}

int MapWithEvent::checkTeamFull(MapWithEvent *map) {
    for (auto id: ::hojy::world::state::gSaveData.baseInfo->members) {
        if (id < 0) {
            return 0;
        }
    }
    return 1;
}

bool MapWithEvent::leaveTeam(MapWithEvent *map, std::int16_t charId) {
    ::hojy::world::state::leaveTeam(charId);
    return true;
}

bool MapWithEvent::emptyAllMP(MapWithEvent *map) {
    for (auto id: ::hojy::world::state::gSaveData.baseInfo->members) {
        if (id < 0) {
            continue;
        }
        auto *charInfo = ::hojy::world::state::gSaveData.charInfo[id];
        if (charInfo) { charInfo->mp = 0; }
    }
    return true;
}

bool MapWithEvent::setAttrPoison(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    if (!validCharacter(charId)) { return true; }
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (charInfo) {
        charInfo->poison = std::clamp<std::int16_t>(
            value, 0, ::hojy::content::PoisonMax);
    }
    return true;
}

bool MapWithEvent::moveCamera(MapWithEvent *map, std::int16_t x0, std::int16_t y0, std::int16_t x1, std::int16_t y1) {
    if (!validSubMapStorage(map->subMapId_, true, true)
        || !map->validMapCoordinate(x0, y0) || !map->validMapCoordinate(x1, y1)) { return true; }
    const auto distance = std::llabs(static_cast<long long>(x1) - x0)
        + std::llabs(static_cast<long long>(y1) - y0);
    if (distance > static_cast<long long>(::hojy::content::SubMapWidth)
            + ::hojy::content::SubMapHeight) { return true; }
    map->movingChar_ = false;
    map->moving_.clear();
    if (distance == 0) {
        map->cameraX_ = x1;
        map->cameraY_ = y1;
        map->markWorldChanged();
        return true;
    }
    if (y0 != y1) {
        std::int16_t dy = y0 < y1 ? -1 : 1;
        for (std::int16_t y = y1; y != y0; y+= dy) {
            map->moving_.emplace_back(std::make_pair(x1, y));
        }
    }
    if (x0 != x1) {
        std::int16_t dx = x0 < x1 ? -1 : 1;
        for (std::int16_t x = x1; x != x0; x+= dx) {
            map->moving_.emplace_back(std::make_pair(x, y0));
        }
    }
    return false;
}

bool MapWithEvent::modifyEventId(MapWithEvent *map, std::int16_t subMapId, std::int16_t eventId,
                                 std::int16_t ev0, std::int16_t ev1, std::int16_t ev2) {
    const auto id = subMapId < 0 ? map->subMapId_ : subMapId;
    if (!validEventRef(id, eventId)) { return true; }
    auto &ev = ::hojy::world::state::gSaveData.subMapEventInfo[id]->events[eventId];
    auto candidate = ev;
    if (!checkedAdd(candidate.event[0], ev0, candidate.event[0])
        || !checkedAdd(candidate.event[1], ev1, candidate.event[1])
        || !checkedAdd(candidate.event[2], ev2, candidate.event[2])) {
        return true;
    }
    ev = candidate;
    return true;
}

bool MapWithEvent::animation(MapWithEvent *map, std::int16_t eventId, std::int16_t begTex, std::int16_t endTex) {
    if (map->subMapId_ < 0) { return true; }
    map->animEventId_[0] = eventId;
    map->animCurrTex_[0] = begTex;
    map->animEndTex_[0] = endTex;
    return false;
}

int MapWithEvent::checkIntegrity(MapWithEvent *map, std::int16_t charId, std::int16_t low, std::int16_t high) {
    if (low > high || !validCharacter(charId)) { return 0; }
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return 0; }
    auto value = charInfo->integrity;
    return value >= low && value <= high ? 1 : 0;
}

int MapWithEvent::checkAttack(MapWithEvent *map, std::int16_t charId, std::int16_t low, std::int16_t high) {
    if (low > high || !validCharacter(charId)) { return 0; }
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return 0; }
    auto value = charInfo->attack;
    return value >= low && value <= high ? 1 : 0;
}

bool MapWithEvent::walkPath(MapWithEvent *map, std::int16_t x0, std::int16_t y0, std::int16_t x1, std::int16_t y1) {
    if (!validSubMapStorage(map->subMapId_, true, true)
        || !map->validMapCoordinate(x0, y0) || !map->validMapCoordinate(x1, y1)) { return true; }
    const auto distance = std::llabs(static_cast<long long>(x1) - x0)
        + std::llabs(static_cast<long long>(y1) - y0);
    if (distance > static_cast<long long>(::hojy::content::SubMapWidth)
            + ::hojy::content::SubMapHeight) { return true; }
    map->movingChar_ = true;
    map->moving_.clear();
    if (distance == 0) {
        const auto oldX = map->currX_;
        const auto oldY = map->currY_;
        map->currX_ = x1;
        map->currY_ = y1;
        map->cameraX_ = x1;
        map->cameraY_ = y1;
        map->movingChar_ = false;
        map->markWorldChanged();
        if (oldX != map->currX_ || oldY != map->currY_) {
            map->markMiniPanelChanged();
        }
        map->updateMainCharSpriteId();
        return true;
    }
    if (y0 != y1) {
        std::int16_t dy = y0 < y1 ? -1 : 1;
        for (std::int16_t y = y1; y != y0; y+= dy) {
            map->moving_.emplace_back(std::make_pair(x1, y));
        }
    }
    if (x0 != x1) {
        std::int16_t dx = x0 < x1 ? -1 : 1;
        for (std::int16_t x = x1; x != x0; x+= dx) {
            map->moving_.emplace_back(std::make_pair(x, y0));
        }
    }
    return false;
}

int MapWithEvent::checkMoney(MapWithEvent *map, std::int16_t amount) {
    if (amount < 0) { return 0; }
    return ::hojy::world::state::gBag[::hojy::content::ItemIDMoney] >= amount;
}

bool MapWithEvent::addItem2(MapWithEvent *map, std::int16_t itemId, std::int16_t itemCount) {
    ::hojy::world::state::gBag.add(itemId, itemCount);
    return true;
}

}
