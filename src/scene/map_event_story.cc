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
bool MapWithEvent::tutorialTalk(MapWithEvent *map) {
    return doTalk(map, 2547 + util::gRandom(18), 114, 0);
}

bool MapWithEvent::showIntegrity(MapWithEvent *map) {
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[0];
    if (!charInfo) { return true; }
    gWindow->popupMessageBox({GETTEXT(76) + L' ' + std::to_wstring(charInfo->integrity)}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::showReputation(MapWithEvent *map) {
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[0];
    if (!charInfo) { return true; }
    gWindow->popupMessageBox({GETTEXT(77) + L' ' + std::to_wstring(charInfo->reputation)}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::openWorld(MapWithEvent *) {
    auto sz = ::hojy::world::state::gSaveData.subMapInfo.size();
    for (size_t i = 0; i < sz; ++i) {
        ::hojy::world::state::gSaveData.subMapInfo[i]->enterCondition = 0;
    }
    ::hojy::world::state::gSaveData.subMapInfo[2]->enterCondition = 2;
    ::hojy::world::state::gSaveData.subMapInfo[38]->enterCondition = 2;
    ::hojy::world::state::gSaveData.subMapInfo[75]->enterCondition = 1;
    ::hojy::world::state::gSaveData.subMapInfo[80]->enterCondition = 1;
    return true;
}

int MapWithEvent::checkEventID(MapWithEvent *map, std::int16_t eventId, std::int16_t value) {
    return ::hojy::world::state::gSaveData.subMapEventInfo[map->subMapId_]->events[eventId].event[0] == value ? 1 : 0;
}

bool MapWithEvent::addReputation(MapWithEvent *map, std::int16_t value) {
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[0];
    if (!charInfo) { return true; }
    auto oldRep = charInfo->reputation;
    charInfo->reputation += value;
    if (oldRep <= 200 && charInfo->reputation > 200) {
        modifyEvent(map, 70, 11, 0, 11, 932, -1, -1, 7968, 7968, 7968, 0, 18, 21);
    }
    return true;
}

bool MapWithEvent::removeBarrier(MapWithEvent *map) {
    animation(map, -1, 3832 * 2, 3844 * 2);
    map->pendingSubEvents_.emplace_back([map]() {
        return animation3(map, 2, 3845 * 2, 3873 * 2, 3, 3874 * 2, 4, 3903 * 2);
    });
    return false;
}

bool MapWithEvent::tournament(MapWithEvent *map) {
    static const std::int16_t heads[] = {
         8, 21, 23, 31, 32, 43,  7, 11, 14, 20, 33, 34, 10, 12, 19, 22,
        56, 68, 13, 55, 62, 67, 70, 71, 26, 57, 60, 64,  3, 69
    };

    for (int i = 0; i < 15; ++i) {
        int n = util::gRandom(2);
        map->pendingSubEvents_.emplace_back([map, i, n] {
            doTalk(map, 2854 + i * 2 + n, heads[i * 2 + n], util::gRandom(2) * 4 + util::gRandom(2));
            return false;
        });
        map->pendingSubEvents_.emplace_back([i, n] {
            gWindow->closePopup();
            return gWindow->enterWar(102 + i * 2 + n, false, true) ? false : true;
        });
        map->pendingSubEvents_.emplace_back([map] {
            makeDim(map);
            return false;
        });
        map->pendingSubEvents_.emplace_back([map] {
            makeBright(map);
            return false;
        });
        if (i % 3 == 2) {
            map->pendingSubEvents_.emplace_back([map] {
                doTalk(map, 2891, 70, 4);
                return false;
            });
            map->pendingSubEvents_.emplace_back([map] {
                gWindow->closePopup();
                sleep(map);
                makeDim(map);
                return false;
            });
            map->pendingSubEvents_.emplace_back([map] {
                makeBright(map);
                return false;
            });
        }
    }
    map->pendingSubEvents_.emplace_back([map] {
        doTalk(map, 2884, 0, 3);
        return false;
    });
    map->pendingSubEvents_.emplace_back([map] {
        gWindow->closePopup();
        doTalk(map, 2885, 0, 3);
        return false;
    });
    map->pendingSubEvents_.emplace_back([map] {
        gWindow->closePopup();
        doTalk(map, 2886, 0, 3);
        return false;
    });
    map->pendingSubEvents_.emplace_back([map] {
        gWindow->closePopup();
        doTalk(map, 2887, 0, 3);
        return false;
    });
    map->pendingSubEvents_.emplace_back([map] {
        gWindow->closePopup();
        doTalk(map, 2888, 0, 3);
        return false;
    });
    map->pendingSubEvents_.emplace_back([map] {
        gWindow->closePopup();
        doTalk(map, 2889, 0, 1);
        return false;
    });
    map->pendingSubEvents_.emplace_back([map] {
        gWindow->closePopup();
        return MapWithEvent::addItem(map, 0x8F, 1);
    });
    return true;
}

bool MapWithEvent::disbandTeam(MapWithEvent *map) {
    for (int i = ::hojy::content::TeamMemberCount - 1; i > 0; --i) {
        auto charId = ::hojy::world::state::gSaveData.baseInfo->members[i];
        if (charId > 0) {
            ::hojy::world::state::leaveTeam(charId);
        }
    }
    return true;
}

int MapWithEvent::checkSubMapTex(MapWithEvent *map, std::int16_t subMapId, std::int16_t eventId, std::int16_t tex) {
    const auto &evt = ::hojy::world::state::gSaveData.subMapEventInfo[subMapId < 0 ? map->subMapId_ : subMapId]->events[eventId];
    return (evt.currTex == tex || evt.begTex == tex || evt.endTex == tex) ? 1 : 0;
}

int MapWithEvent::checkAllStoryBooks(MapWithEvent *map) {
    const auto &events = ::hojy::world::state::gSaveData.subMapEventInfo[map->subMapId_]->events;
    for (int i = 11; i <= 24; i++)
    {
        if (events[i].currTex != 4664)
        {
            return 0;
        }
    }
    return 1;
}

bool MapWithEvent::goBackHome(MapWithEvent *map, std::int16_t eventId, std::int16_t begTex, std::int16_t endTex,
                              std::int16_t eventId2, std::int16_t begTex2, std::int16_t endTex2) {
    map->showChar(false);
    map->pendingSubEvents_.emplace_back([]() {
        gWindow->endscreen();
        return true;
    });
    return animation2(map, eventId, begTex, endTex, eventId2, begTex2, endTex2);
}

bool MapWithEvent::setSex(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = ::hojy::world::state::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->sex = value;
    return true;
}

}
