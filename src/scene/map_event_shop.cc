#include "mapwithevent.hh"

#include "window.hh"
#include "content/constants.hh"
#include "content/event.hh"
#include "world/savedata.hh"
#include "util/random.hh"

#include <cstdint>

namespace hojy::scene {
struct ShopEventInfo {
    std::int16_t subMapId;
    std::int16_t shopEventIndex;
    std::int16_t randomEventIndex[3];
};

static const ShopEventInfo shopEventInfo[5] = {
    {1, 16, {17, 18}},
    {3, 14, {15, 16}},
    {40, 20, {21, 22}},
    {60, 16, {17, 18}},
    {61, 9, {10, 11, 12}},
};

bool MapWithEvent::openShop(MapWithEvent *map) {
    if (map->subMapId_ < 0) {
        return true;
    }
    int i;
    /* set random shop event on exit cells */
    for (i = 0; i < 5; ++i) {
        auto &evi = shopEventInfo[i];
        if (evi.subMapId == map->subMapId_) {
            auto &evts = ::hojy::world::state::gSaveData.subMapEventInfo[map->subMapId_]->events;
            for (auto &n: evi.randomEventIndex) {
                if (n > 0) { evts[n].event[2] = ::hojy::content::RandomShopEventId; }
            }
            break;
        }
    }
    doTalk(map, 0xB9E, 0x6F, 0);
    if (i >= 5) {
        return false;
    }
    return !gWindow->runShop(i);
}

bool MapWithEvent::randomShop(MapWithEvent *map) {
    if (map->subMapId_ < 0) {
        return true;
    }
    /* remove random shop event from exit cells */
    for (auto &evi: shopEventInfo) {
        if (evi.subMapId == map->subMapId_) {
            auto &evts = ::hojy::world::state::gSaveData.subMapEventInfo[map->subMapId_]->events;
            auto &ev = evts[evi.shopEventIndex];
            ev.blocked = 0;
            ev.event[0] = -1;
            ev.currTex = ev.begTex = ev.endTex = -1;
            for (auto &n: evi.randomEventIndex) {
                if (n > 0) { evts[n].event[2] = -1; }
            }
            break;
        }
    }
    const auto &evi = shopEventInfo[util::gRandom(5)];
    auto &ev = ::hojy::world::state::gSaveData.subMapEventInfo[evi.subMapId]->events[evi.shopEventIndex];
    ev.blocked = 1;
    ev.event[0] = ::hojy::content::ShopEventId;
    ev.begTex = ev.currTex = ev.endTex = ::hojy::content::ShopEventTex;
    return true;
}

bool MapWithEvent::playMusic(MapWithEvent *, std::int16_t musicId) {
    gWindow->playMusic(musicId);
    return true;
}

bool MapWithEvent::playSound(MapWithEvent *, std::int16_t soundId) {
    gWindow->playAtkSound(soundId);
    return true;
}

}
