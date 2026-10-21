#include "mapwithevent.hh"

#include "window_command.hh"
#include "content/constants.hh"
#include "content/event.hh"
#include "world/savedata.hh"
#include "util/random.hh"

#include <array>
#include <algorithm>
#include <cstdint>
#include <vector>

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

namespace {

bool validShopEntry(const ShopEventInfo &entry) {
    if (entry.subMapId < 0) { return false; }
    const auto mapIndex = static_cast<std::size_t>(entry.subMapId);
    const auto &save = ::hojy::world::state::gSaveData;
    if (mapIndex >= save.subMapEventInfo.size()) {
        return false;
    }
    const auto validEvent = [](std::int16_t id) {
        return id >= 0 && id < ::hojy::content::SubMapEventCount;
    };
    if (!validEvent(entry.shopEventIndex)) { return false; }
    for (const auto id: entry.randomEventIndex) {
        if (id > 0 && !validEvent(id)) { return false; }
    }
    return true;
}

}

bool MapWithEvent::openShop(MapWithEvent *map) {
    if (!map || map->subMapId_ < 0
        || static_cast<std::size_t>(map->subMapId_)
            >= ::hojy::world::state::gSaveData.subMapEventInfo.size()
        || static_cast<std::size_t>(map->subMapId_)
            >= ::hojy::world::state::gSaveData.subMapLayerInfo.size()) {
        return true;
    }
    int i;
    /* set random shop event on exit cells */
    auto *eventInfo = ::hojy::world::state::gSaveData
        .subMapEventInfo[map->subMapId_].operator->();
    auto *layerInfo = ::hojy::world::state::gSaveData
        .subMapLayerInfo[map->subMapId_].operator->();
    if (!eventInfo || !layerInfo) { return true; }
    auto candidateEvents = *eventInfo;
    for (i = 0; i < 5; ++i) {
        auto &evi = shopEventInfo[i];
        if (evi.subMapId == map->subMapId_) {
            if (!validShopEntry(evi)) { return true; }
            for (auto &n: evi.randomEventIndex) {
                if (n > 0) {
                    candidateEvents.events[n].event[2]
                        = ::hojy::content::RandomShopEventId;
                }
            }
            break;
        }
    }
    std::vector<std::int16_t> candidateLayer(
        layerInfo->data[3],
        layerInfo->data[3] + ::hojy::content::SubMapWidth
            * ::hojy::content::SubMapHeight);
    logic::SubMapStateSnapshot snapshot;
    if (!map->validateSubMapStateCandidate(
            map->subMapId_, candidateEvents, candidateLayer, snapshot)) {
        return true;
    }
    *eventInfo = candidateEvents;
    map->synchronizeCommittedSubMapState(map->subMapId_, snapshot);
    doTalk(map, 0xB9E, 0x6F, 0);
    if (i >= 5) {
        return false;
    }
    postSceneCommand(map, [i](SceneCommandContext &context) { (void)context.runShop(i); });
    return false;
}

bool MapWithEvent::randomShop(MapWithEvent *map) {
    if (!map || map->subMapId_ < 0) {
        return true;
    }
    for (const auto &entry: shopEventInfo) {
        if (!validShopEntry(entry)) { return true; }
    }
    const auto selected = shopEventInfo[util::gRandom(5)];
    struct Candidate final {
        std::int16_t id;
        ::hojy::world::state::SubMapEventData events;
        std::vector<std::int16_t> layer;
        logic::SubMapStateSnapshot snapshot;
    };
    std::vector<Candidate> candidates;
    candidates.reserve(5);
    auto findCandidate = [&candidates](std::int16_t id) -> Candidate * {
        for (auto &candidate: candidates) {
            if (candidate.id == id) { return &candidate; }
        }
        return nullptr;
    };
    auto ensureCandidate = [&candidates, &findCandidate](std::int16_t id) -> Candidate * {
        if (auto *existing = findCandidate(id)) { return existing; }
        if (id < 0 || static_cast<std::size_t>(id)
                >= ::hojy::world::state::gSaveData.subMapEventInfo.size()
            || static_cast<std::size_t>(id)
                >= ::hojy::world::state::gSaveData.subMapLayerInfo.size()) {
            return nullptr;
        }
        auto *events = ::hojy::world::state::gSaveData
            .subMapEventInfo[id].operator->();
        auto *layers = ::hojy::world::state::gSaveData
            .subMapLayerInfo[id].operator->();
        if (!events || !layers) { return nullptr; }
        candidates.push_back(Candidate{
            id, *events,
            std::vector<std::int16_t>(
                layers->data[3],
                layers->data[3] + ::hojy::content::SubMapWidth
                    * ::hojy::content::SubMapHeight),
        });
        return &candidates.back();
    };

    /* remove random shop event from exit cells */
    for (auto &evi: shopEventInfo) {
        if (evi.subMapId == map->subMapId_) {
            auto *candidate = ensureCandidate(evi.subMapId);
            if (!candidate) { return true; }
            auto &ev = candidate->events.events[evi.shopEventIndex];
            ev.blocked = 0;
            ev.event[0] = -1;
            ev.currTex = ev.begTex = ev.endTex = -1;
            for (auto &n: evi.randomEventIndex) {
                if (n > 0) { candidate->events.events[n].event[2] = -1; }
            }
            break;
        }
    }
    auto *selectedCandidate = ensureCandidate(selected.subMapId);
    if (!selectedCandidate) { return true; }
    auto &ev = selectedCandidate->events.events[selected.shopEventIndex];
    ev.blocked = 1;
    ev.event[0] = ::hojy::content::ShopEventId;
    ev.begTex = ev.currTex = ev.endTex = ::hojy::content::ShopEventTex;
    for (auto &candidate: candidates) {
        if (!map->validateSubMapStateCandidate(
                candidate.id, candidate.events, candidate.layer,
                candidate.snapshot)) {
            return true;
        }
    }
    for (const auto &candidate: candidates) {
        auto *events = ::hojy::world::state::gSaveData
            .subMapEventInfo[candidate.id].operator->();
        if (!events) { return true; }
        *events = candidate.events;
        map->synchronizeCommittedSubMapState(
            candidate.id, candidate.snapshot);
    }
    return true;
}

bool MapWithEvent::playMusic(MapWithEvent *map, std::int16_t musicId) {
    // Caller supplies the map through the VM host; use the command sink on
    // that host so audio is emitted only at the fixed-update barrier.
    postSceneCommand(map, [musicId](SceneCommandContext &context) { context.playMusic(musicId); });
    return true;
}

bool MapWithEvent::playSound(MapWithEvent *map, std::int16_t soundId) {
    postSceneCommand(map, [soundId](SceneCommandContext &context) { context.playAtkSound(soundId); });
    return true;
}

}
