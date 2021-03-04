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

#include "data/event.hh"
#include "mem/savedata.hh"
#include "mem/bag.hh"
#include "util/conv.hh"
#include "util/random.hh"

#include <functional>

namespace hojy::scene {

template <class R, class... Args>
constexpr auto argCounter(std::function<R(Args...)>) {
    return sizeof...(Args);
}

template <class R, class... Args, class P>
constexpr auto argCounter(R(P::*)(Args...)) {
    return sizeof...(Args);
}

template <class R, class... Args>
constexpr auto argCounter(R(Args...)) {
    return sizeof...(Args);
}

template <class R, class M, class... Args>
struct ReturnTypeMatches {};

template <class R, class M, class... Args>
struct ReturnTypeMatches<std::function<R(Args...)>, M> {
    static constexpr bool value = std::is_same<R, M>::value;
};

template <class R, class M, class... Args, class P>
struct ReturnTypeMatches<R(P::*)(Args...), M> {
    static constexpr bool value = std::is_same<R, M>::value;
};

template <class R, class M, class... Args>
struct ReturnTypeMatches<R(Args...), M> {
    static constexpr bool value = std::is_same<R, M>::value;
};

template<class F, class P, size_t ...I>
typename std::enable_if<ReturnTypeMatches<F, void>::value, void>::type
runFunc(F f, P *p, const std::vector<std::int16_t> &evlist, size_t &index, std::index_sequence<I...>) {
    (p->*f)(evlist[I + index]...);
    index += sizeof...(I);
}

template<class F, class P, size_t ...I>
typename std::enable_if<ReturnTypeMatches<F, bool>::value, void>::type
runFunc(F f, P *p, const std::vector<std::int16_t> &evlist, size_t &index, std::index_sequence<I...>) {
    if ((p->*f)(evlist[I + index]...)) {
        index += sizeof...(I);
        index += evlist[index] + 2;
    } else {
        index += sizeof...(I);
        index += evlist[index + 1] + 2;
    }
}

void MapWithEvent::doInteract() {
    int x, y;
    getFaceOffset(x, y);

    auto &layers = mem::gSaveData.subMapLayerInfo[subMapId_]->data;
    auto &events = mem::gSaveData.subMapEventInfo[subMapId_]->events;
    auto eventId = layers[3][y * mapWidth_ + x];

    if (eventId < 0) { return; }
    auto evt = events[eventId].event1;
    if (evt <= 0) { return; }
    currEventId_ = eventId;

#define OpRun(O, F)                                                    \
    case O: \
        runFunc(&MapWithEvent::F, this, evlist, index, std::make_index_sequence<argCounter(&MapWithEvent::F)>()); \
        break

    const auto &evlist = data::gEvent.event(evt);
    auto evsz = evlist.size();
    size_t index = 0;
    while (index < evsz) {
        auto op = evlist[index++];
        if (op == -1) { break; }
        switch (op) {
        OpRun(1, doTalk);
        OpRun(2, addItem);
        OpRun(3, modifyEvent);
        OpRun(51, tutorialTalk);
        default:
            break;
        }
    }
    currEventId_ = -1;
#undef OpRun
}

void MapWithEvent::doTalk(std::int16_t talkId, std::int16_t portrait, std::int16_t position) {
    (void)this; /* avoid Clang-Tidy warning: Method can be made static */
    const auto &str = data::gEvent.talk(talkId);
    auto wstr = util::big5Conv.toUnicode(str);
}

void MapWithEvent::addItem(std::int16_t itemId, std::int16_t itemCount) {
    (void)this; /* avoid Clang-Tidy warning: Method can be made static */
    mem::gBag.add(itemId, itemCount);
}

void MapWithEvent::modifyEvent(std::int16_t subMapId, std::int16_t eventId, std::int16_t blocked, std::int16_t index,
                               std::int16_t event1, std::int16_t event2, std::int16_t event3, std::int16_t currTex,
                               std::int16_t endTex, std::int16_t begTex, std::int16_t texDelay, std::int16_t x,
                               std::int16_t y) {
    if (subMapId < 0) { subMapId = subMapId_; }
    if (eventId < 0) { eventId = currEventId_; }
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
        layer[ev.y * mapWidth_ + ev.x] = -1;
        layer[y * mapWidth_ + x] = eventId;
        ev.x = x; ev.y = y;
    }
    if (currTex > -2) {
        ev.currTex = currTex;
        setCellTexture(x, y, currTex >> 1);
    }
}

void MapWithEvent::tutorialTalk() {
    doTalk(2547 + util::gRandom(18), 114, 0);
}

}
