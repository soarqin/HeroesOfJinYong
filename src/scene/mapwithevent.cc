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

#include "mapwithevent.hh"

#include "window.hh"
#include "data/event.hh"
#include "mem/savedata.hh"
#include "audio/mixer.hh"
#include "audio/channelmidi.hh"
#include "audio/channelwav.hh"
#include "util/random.hh"

#include <functional>

namespace hojy::scene {

void MapWithEvent::render() {
    Map::render();
    if ((++frames_) % 10) { return; }
    updateEventTextures();
}

template <class R, class... Args>
constexpr auto argCounter(std::function<R(Args...)>) {
    return sizeof...(Args);
}

template <class R, class... Args>
constexpr auto argCounter(R(*)(Args...)) {
    return sizeof...(Args);
}

template <class R, class M, class... Args>
struct ReturnTypeMatches {};

template <class R, class M, class... Args>
struct ReturnTypeMatches<std::function<R(Args...)>, M> {
    static constexpr bool value = std::is_same<R, M>::value;
};

template <class R, class M, class... Args>
struct ReturnTypeMatches<R(*)(Args...), M> {
    static constexpr bool value = std::is_same<R, M>::value;
};

template<class F, class P, size_t ...I>
typename std::enable_if<ReturnTypeMatches<F, void>::value, void>::type
runFunc(F f, P *p, const std::vector<std::int16_t> &evlist, size_t &index, size_t &advTrue, size_t &advFalse, std::index_sequence<I...>) {
    f(p, evlist[I + index]...);
    index += sizeof...(I);
    advTrue = advFalse = 0;
}

template<class F, class P, size_t ...I>
typename std::enable_if<ReturnTypeMatches<F, bool>::value, void>::type
runFunc(F f, P *p, const std::vector<std::int16_t> &evlist, size_t &index, size_t &advTrue, size_t &advFalse, std::index_sequence<I...>) {
    advTrue = evlist[index + sizeof...(I)];
    advFalse = evlist[index + sizeof...(I) + 1];
    f(p, evlist[I + index]...);
    index += sizeof...(I) + 2;
}

void MapWithEvent::continueEvents(bool result) {
    if (!currEventList_) { return; }
    currEventPaused_ = false;
    currEventIndex_ += result ? currEventAdvTrue_ : currEventAdvFalse_;
    currEventAdvTrue_ = currEventAdvFalse_ = 0;

#define OpRun(O, F) \
    case O: \
        runFunc(F, this, evlist, currEventIndex_, currEventAdvTrue_, currEventAdvFalse_, std::make_index_sequence<argCounter(F)-1>()); \
        break

    const auto &evlist = *currEventList_;
    while (!currEventPaused_ && currEventIndex_ < currEventSize_) {
        auto op = evlist[currEventIndex_++];
        if (op == -1 || op == 7) { break; }
        switch (op) {
        OpRun(1, doTalk);
        OpRun(2, addItem);
        OpRun(3, modifyEvent);
        OpRun(27, animation);
        OpRun(39, openSubMap);
        OpRun(40, forceDirection);
        OpRun(51, tutorialTalk);
        OpRun(54, openWorld);
        OpRun(66, playMusic);
        OpRun(67, playSound);
        default:
            break;
        }
    }
    if (currEventIndex_ >= currEventSize_) {
        currEventId_ = -1;
        currEventIndex_ = currEventSize_ = 0;
        currEventList_ = nullptr;
    }
#undef OpRun
}

void MapWithEvent::doInteract() {
    int x, y;
    if (!getFaceOffset(x, y)) {
        return;
    }

    auto &layers = mem::gSaveData.subMapLayerInfo[subMapId_]->data;
    auto eventId = layers[3][y * mapWidth_ + x];
    if (eventId < 0) { return; }

    auto &events = mem::gSaveData.subMapEventInfo[subMapId_]->events;
    auto evt = events[eventId].event1;
    if (evt <= 0) { return; }

    currEventId_ = eventId;
    currEventList_ = &data::gEvent.event(evt);
    currEventSize_ = currEventList_->size();
    currEventIndex_ = 0;

    resetTime();
    currFrame_ = 0;
    updateMainCharTexture();

    continueEvents();
}

void MapWithEvent::onMove() {
    auto &layers = mem::gSaveData.subMapLayerInfo[subMapId_]->data;
    auto eventId = layers[3][currY_ * mapWidth_ + currX_];
    if (eventId < 0) { return; }

    auto &events = mem::gSaveData.subMapEventInfo[subMapId_]->events;
    auto evt = events[eventId].event3;
    if (evt <= 0) { return; }

    currEventId_ = eventId;
    currEventList_ = &data::gEvent.event(evt);
    currEventSize_ = currEventList_->size();
    currEventIndex_ = 0;

    resetTime();
    currFrame_ = 0;
    updateMainCharTexture();

    continueEvents();
}

void MapWithEvent::updateEventTextures() {
    if (animCurrTex_ == 0) { return; }
    if (animCurrTex_ == animEndTex_) {
        if (animEventId_ < 0) {
            updateMainCharTexture();
        }
        animEventId_ = 0;
        animCurrTex_ = 0;
        animEndTex_ = 0;
        continueEvents();
        return;
    }
    int step = animCurrTex_ < animEndTex_ ? 1 : -1;
    animCurrTex_ += step;
    if (animEventId_ < 0) {
        updateMainCharTexture();
    } else {
        auto &evt = mem::gSaveData.subMapEventInfo[subMapId_]->events[animEventId_];
        evt.currTex = evt.begTex = evt.endTex = animCurrTex_;
        setCellTexture(evt.x, evt.y, animCurrTex_ >> 1);
    }
}

void MapWithEvent::doTalk(MapWithEvent *map, std::int16_t talkId, std::int16_t headId, std::int16_t position) {
    gWindow->runTalk(data::gEvent.talk(talkId), headId, position);
    map->currEventPaused_ = true;
}

void MapWithEvent::addItem(MapWithEvent *map, std::int16_t itemId, std::int16_t itemCount) {
    mem::gBag.add(itemId, itemCount);
}

void MapWithEvent::modifyEvent(MapWithEvent *map, std::int16_t subMapId, std::int16_t eventId, std::int16_t blocked, std::int16_t index,
                               std::int16_t event1, std::int16_t event2, std::int16_t event3, std::int16_t currTex,
                               std::int16_t endTex, std::int16_t begTex, std::int16_t texDelay, std::int16_t x,
                               std::int16_t y) {
    if (subMapId < 0) { subMapId = map->subMapId_; }
    if (subMapId < 0) { return; }
    if (eventId < 0) { eventId = map->currEventId_; }
    if (eventId < 0) { return; }
    auto &ev = mem::gSaveData.subMapEventInfo[subMapId]->events[eventId];
    if (blocked > -2) { ev.blocked = blocked; }
    if (index > -2) { ev.index = index; }
    if (event1 > -2) { ev.event1 = event1; }
    if (event2 > -2) { ev.event2 = event2; }
    if (event3 > -2) { ev.event3 = event3; }
    if (endTex > -2) { ev.endTex = endTex; }
    if (begTex > -2) { ev.begTex = begTex; }
    if (texDelay > -2) { ev.texDelay = texDelay; }
    if (x < 0) { x = ev.x; }
    if (y < 0) { y = ev.y; }
    if (x != ev.x || y != ev.y) {
        auto &layer = mem::gSaveData.subMapLayerInfo[subMapId]->data[3];
        layer[ev.y * map->mapWidth_ + ev.x] = -1;
        layer[y * map->mapWidth_ + x] = eventId;
        ev.x = x; ev.y = y;
    }
    if (currTex > -2) {
        ev.currTex = currTex;
        map->setCellTexture(x, y, currTex >> 1);
    }
}

void MapWithEvent::animation(MapWithEvent *map, std::int16_t eventId, std::int16_t begTex, std::int16_t endTex) {
    if (map->subMapId_ < 0) { return; }
    map->animEventId_ = eventId;
    map->animCurrTex_ = begTex;
    map->animEndTex_ = endTex;
    map->currEventPaused_ = true;
}

void MapWithEvent::openSubMap(MapWithEvent *, std::int16_t subMapId) {
    mem::gSaveData.subMapInfo[subMapId]->enterCondition = 0;
}

void MapWithEvent::forceDirection(MapWithEvent *map, std::int16_t direction) {
    map->setDirection(Direction(direction));
    map->updateMainCharTexture();
}

void MapWithEvent::tutorialTalk(MapWithEvent *map) {
    doTalk(map, 2547 + util::gRandom(18), 114, 0);
}

void MapWithEvent::openWorld(MapWithEvent *) {
    auto sz = mem::gSaveData.subMapInfo.size();
    for (size_t i = 0; i < sz; ++i) {
        mem::gSaveData.subMapInfo[i]->enterCondition = 0;
    }
    mem::gSaveData.subMapInfo[2]->enterCondition = 2;
    mem::gSaveData.subMapInfo[38]->enterCondition = 2;
    mem::gSaveData.subMapInfo[75]->enterCondition = 1;
    mem::gSaveData.subMapInfo[80]->enterCondition = 1;
}

bool MapWithEvent::checkTime() {
    if (animEventId_ < 0) { return false; }
    return Map::checkTime();
}

void MapWithEvent::playMusic(MapWithEvent *, std::int16_t musicId) {
    gWindow->playMusic(musicId);
}

void MapWithEvent::playSound(MapWithEvent *map, std::int16_t soundId) {
    if (soundId < 24) {
        gWindow->playAtkSound(soundId);
    } else {
        gWindow->playEffectSound(soundId - 24);
    }
}

}
