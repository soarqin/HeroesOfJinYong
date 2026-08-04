#include "mapwithevent.hh"

#include "window.hh"
#include "mask.hh"
#include "content/constants.hh"
#include "content/event.hh"
#include "world/action.hh"
#include "world/bag.hh"
#include "world/savedata.hh"
#include "world/strings.hh"
#include "util/random.hh"

#include <fmt/xchar.h>
#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace hojy::scene {
bool MapWithEvent::closePopup(MapWithEvent *) {
    gWindow->closePopup();
    return true;
}

bool MapWithEvent::doTalk(MapWithEvent *, std::int16_t talkId, std::int16_t headId, std::int16_t position) {
    gWindow->runTalk(::hojy::content::gEvent.talk(talkId), headId, position);
    return false;
}

bool MapWithEvent::addItem(MapWithEvent *map, std::int16_t itemId, std::int16_t itemCount) {
    ::hojy::world::state::gBag.add(itemId, itemCount);
    gWindow->popupMessageBox({fmt::format(L"{} {}x{}", GETTEXT(71), GETITEMNAME(itemId), std::to_wstring(itemCount))}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::modifyEvent(MapWithEvent *map, std::int16_t subMapId, std::int16_t eventId, std::int16_t blocked,
                               std::int16_t index, std::int16_t event1, std::int16_t event2, std::int16_t event3,
                               std::int16_t currTex, std::int16_t endTex, std::int16_t begTex, std::int16_t texDelay,
                               std::int16_t x, std::int16_t y) {
    if (subMapId < 0) { subMapId = map->subMapId_; }
    if (subMapId < 0) { return true; }
    if (eventId < 0) { eventId = map->currEventId_; }
    if (eventId < 0) { return true; }
    auto &ev = ::hojy::world::state::gSaveData.subMapEventInfo[subMapId]->events[eventId];
    if (blocked > -2) { ev.blocked = blocked; }
    if (index > -2) { ev.index = index; }
    if (event1 > -2) { ev.event[0] = event1; }
    if (event2 > -2) { ev.event[1] = event2; }
    if (event3 > -2) { ev.event[2] = event3; }
    if (endTex > -2) { ev.endTex = endTex; }
    if (begTex > -2) { ev.begTex = begTex; }
    if (texDelay > -2) { ev.texDelay = texDelay; }
    if (x < 0) { x = ev.x; }
    if (y < 0) { y = ev.y; }
    if (x != ev.x || y != ev.y) {
        auto &layer = ::hojy::world::state::gSaveData.subMapLayerInfo[subMapId]->data[3];
        layer[ev.y * map->mapWidth_ + ev.x] = -1;
        layer[y * map->mapWidth_ + x] = eventId;
        if (subMapId == map->subMapId_) {
            map->setCellTexture(ev.x, ev.y, 3, -1);
        }
        ev.x = x; ev.y = y;
    }
    if (currTex > -2) {
        ev.currTex = currTex;
        if (subMapId == map->subMapId_) {
            map->setCellTexture(x, y, 3, currTex >> 1);
        }
    }
    return true;
}

int MapWithEvent::useItem(MapWithEvent *map, std::int16_t itemId) {
    return itemId == map->currEventItem_ ? 1 : 0;
}

int MapWithEvent::askForWar(MapWithEvent *map) {
    gWindow->popupMessageBox({GETTEXT(72)}, MessageBox::YesNo);
    return -1;
}

bool MapWithEvent::exitEventList(MapWithEvent *map) {
    gWindow->closePopup();
    return true;
}

bool MapWithEvent::changeExitMusic(MapWithEvent *map, std::int16_t music) {
    ::hojy::world::state::gSaveData.subMapInfo[map->subMapId_]->exitMusic = music;
    return true;
}

int MapWithEvent::askForJoinTeam(MapWithEvent *map) {
    gWindow->popupMessageBox({GETTEXT(73)}, MessageBox::YesNo);
    return -1;
}

bool MapWithEvent::joinTeam(MapWithEvent *map, std::int16_t charId) {
    for (auto &id: ::hojy::world::state::gSaveData.baseInfo->members) {
        if (id < 0) {
            id = charId;
            auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
            if (!charInfo) { continue; }
            for (size_t j = 0; j < ::hojy::content::CarryItemCount; ++j) {
                if (charInfo->item[j] >= 0) {
                    if (charInfo->itemCount[j] == 0) { charInfo->itemCount[j] = 1; }
                    auto itemId = charInfo->item[j];
                    std::int16_t itemCount = charInfo->itemCount[j] == 0 ? 1 : charInfo->itemCount[j];
                    map->pendingSubEvents_.emplace_back([map, itemId, itemCount]()->bool {
                        return addItem(map, itemId, itemCount);
                    });
                    charInfo->item[j] = -1;
                    charInfo->itemCount[j] = 0;
                }
            }
            break;
        }
    }
    return true;
}

int MapWithEvent::wantSleep(MapWithEvent *map) {
    gWindow->popupMessageBox({GETTEXT(74)}, MessageBox::YesNo);
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
    map->fadeIn([map]() {
        map->continueEvents(false);
    });
    return false;
}

bool MapWithEvent::makeDim(MapWithEvent *map) {
    map->fadeOut([map]() {
        map->continueEvents(false);
    });
    return false;
}

bool MapWithEvent::die(MapWithEvent *map) {
    gWindow->playerDie();
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
    if (subMapId < 0) {
        subMapId = map->subMapId_;
    }
    ::hojy::world::state::gSaveData.subMapLayerInfo[subMapId]->data[layer][y * map->mapWidth_ + x] = value;
    if (subMapId == map->subMapId_) {
        map->setCellTexture(x, y, layer, value >> 1);
    }
    return true;
}

int MapWithEvent::hasItem(MapWithEvent *map, std::int16_t itemId) {
    return ::hojy::world::state::gBag[itemId] > 0 ? 1 : 0;
}

bool MapWithEvent::setPlayerPosition(MapWithEvent *map, std::int16_t x, std::int16_t y) {
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
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (charInfo) { charInfo->poison = value; }
    return true;
}

bool MapWithEvent::moveCamera(MapWithEvent *map, std::int16_t x0, std::int16_t y0, std::int16_t x1, std::int16_t y1) {
    if (map->subMapId_ < 0) { return true; }
    map->movingChar_ = false;
    map->moving_.clear();
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
    auto &ev = ::hojy::world::state::gSaveData.subMapEventInfo[subMapId < 0 ? map->subMapId_ : subMapId]->events[eventId];
    ev.event[0] += ev0;
    ev.event[1] += ev1;
    ev.event[2] += ev2;
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
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return 0; }
    auto value = charInfo->integrity;
    return value >= low && value <= high ? 1 : 0;
}

int MapWithEvent::checkAttack(MapWithEvent *map, std::int16_t charId, std::int16_t low, std::int16_t high) {
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return 0; }
    auto value = charInfo->attack;
    return value >= low ? 1 : 0;
}

bool MapWithEvent::walkPath(MapWithEvent *map, std::int16_t x0, std::int16_t y0, std::int16_t x1, std::int16_t y1) {
    if (map->subMapId_ < 0) { return true; }
    map->movingChar_ = true;
    map->moving_.clear();
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
    return ::hojy::world::state::gBag[::hojy::content::ItemIDMoney] >= amount;
}

bool MapWithEvent::addItem2(MapWithEvent *map, std::int16_t itemId, std::int16_t itemCount) {
    ::hojy::world::state::gBag.add(itemId, itemCount);
    return true;
}

}
