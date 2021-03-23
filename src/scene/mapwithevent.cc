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
#include "mask.hh"
#include "data/event.hh"
#include "mem/action.hh"
#include "mem/savedata.hh"
#include "mem/strings.hh"
#include "util/random.hh"
#include <fmt/format.h>

namespace hojy::scene {

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

#ifndef NDEBUG
void printArgs(const std::vector<std::int16_t>& e, int i, int size) {
    fprintf(stdout, "{");
    for (int idx = 0; idx < size; ++idx, ++i) {
        fprintf(stdout, " %d", e[i]);
    }
    fprintf(stdout, " }\n");
    fflush(stdout);
}
#endif

template<class F, class P, size_t ...I>
typename std::enable_if<ReturnTypeMatches<F, bool>::value, bool>::type
runFunc(F f, P *p, const std::vector<std::int16_t> &evlist, size_t &index, size_t &advTrue, size_t &advFalse,
        std::index_sequence<I...>) {
#ifndef NDEBUG
    printArgs(evlist, index, sizeof...(I));
#endif
    bool result = f(p, evlist[I + index]...);
    index += sizeof...(I);
    advTrue = advFalse = 0;
    return result;
}

template<class F, class P, size_t ...I>
typename std::enable_if<ReturnTypeMatches<F, int>::value, bool>::type
runFunc(F f, P *p, const std::vector<std::int16_t> &evlist, size_t &index, size_t &advTrue, size_t &advFalse,
        std::index_sequence<I...>) {
#ifndef NDEBUG
    printArgs(evlist, index, sizeof...(I) + 2);
#endif
    advTrue = evlist[index + sizeof...(I)];
    advFalse = evlist[index + sizeof...(I) + 1];
    int result = f(p, evlist[I + index]...);
    index += sizeof...(I) + 2;
    if (result < 0) {
        return false;
    }
    index += result ? advTrue : advFalse;
    advTrue = advFalse = 0;
    return true;
}

void MapWithEvent::cleanupEvents() {
    currEventPaused_ = false;
    currEventId_ = -1;
    currEventIndex_ = 0; currEventSize_ = 0;
    currEventAdvTrue_ = 0; currEventAdvFalse_ = 0;
    currEventItem_ = -1;
    currEventList_ = nullptr;
}

void MapWithEvent::continueEvents(bool result) {
    if (pendingSubEvents_.empty() && !currEventList_) { return; }
    currEventPaused_ = false;
    currEventIndex_ += result ? currEventAdvTrue_ : currEventAdvFalse_;
    currEventAdvTrue_ = currEventAdvFalse_ = 0;

#define OpRun(O, F) \
    case O: \
        if (!runFunc(F, this, evlist, currEventIndex_, currEventAdvTrue_, currEventAdvFalse_, \
                     std::make_index_sequence<argCounter(F)-1>())) { \
            currEventPaused_ = true; \
        } \
        break

    const auto &evlist = *currEventList_;
    while (!currEventPaused_) {
        while (!pendingSubEvents_.empty()) {
            currEventPaused_ = false;
            auto func = std::move(pendingSubEvents_.front());
            pendingSubEvents_.pop_front();
            if (!func()) {
                currEventPaused_ = true;
                return;
            }
        }
        if (currEventIndex_ >= currEventSize_) {
            break;
        }
        auto op = evlist[currEventIndex_++];
#ifndef NDEBUG
        fprintf(stdout, "%2d: ", op);
#endif
        switch (op) {
        OpRun(-1, exitEventList);
        OpRun(0, closePopup);
        OpRun(1, doTalk);
        OpRun(2, addItem);
        OpRun(3, modifyEvent);
        OpRun(4, useItem);
        OpRun(5, askForWar);
        case 6:
#ifndef NDEBUG
            fprintf(stdout, "{ %d %d %d %d }\n", evlist[currEventIndex_], evlist[currEventIndex_ + 1], evlist[currEventIndex_ + 2], evlist[currEventIndex_ + 3]);
            fflush(stdout);
#endif
            currEventAdvTrue_ = evlist[currEventIndex_ + 1];
            currEventAdvFalse_ = evlist[currEventIndex_ + 2];
            gWindow->enterWar(evlist[currEventIndex_], evlist[currEventIndex_ + 3] > 0);
            currEventIndex_ += 4;
            currEventPaused_ = true;
            break;
        OpRun(7, exitEventList);
        OpRun(8, changeExitMusic);
        OpRun(9, askForJoinTeam);
        OpRun(10, joinTeam);
        OpRun(11, wantSleep);
        OpRun(12, sleep);
        OpRun(13, makeBright);
        OpRun(14, makeDim);
        OpRun(15, die);
        OpRun(16, checkTeamMember);
        OpRun(17, changeLayer);
        OpRun(18, hasItem);
        OpRun(19, setPlayerPosition);
        OpRun(20, checkTeamFull);
        OpRun(21, leaveTeam);
        OpRun(22, emptyAllMP);
        OpRun(23, setAttrPoison);
        OpRun(24, die);
        OpRun(25, moveCamera);
        OpRun(26, modifyEventId);
        OpRun(27, animation);
        OpRun(28, checkIntegrity);
        OpRun(29, checkAttack);
        OpRun(30, walkPath);
        OpRun(31, checkMoney);
        OpRun(32, addItem2);
        OpRun(33, learnSkill);
        OpRun(34, addPotential);
        OpRun(35, setSkill);
        OpRun(36, checkSex);
        OpRun(37, addIntegrity);
        OpRun(38, modifySubMapLayerTex);
        OpRun(39, openSubMap);
        OpRun(40, forceDirection);
        OpRun(41, addItemToChar);
        OpRun(42, checkFemaleInTeam);
        OpRun(43, hasItem);
        OpRun(44, animation2);
        OpRun(45, addSpeed);
        OpRun(46, addMaxMP);
        OpRun(47, addAttack);
        OpRun(48, addMaxHP);
        OpRun(49, setMPType);
        OpRun(50, checkHas5Item);
        OpRun(51, tutorialTalk);
        OpRun(52, showIntegrity);
        OpRun(53, showReputation);
        OpRun(54, openWorld);
        OpRun(55, checkEventID);
        OpRun(56, addReputation);
        OpRun(57, removeBarrier);
        OpRun(58, tournament);
        OpRun(59, disbandTeam);
        OpRun(60, checkSubMapTex);
        OpRun(61, checkAllStoryBooks);
        OpRun(62, goBackHome);
        OpRun(63, setSex);
        OpRun(64, openShop);
        OpRun(65, randomShop);
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

void MapWithEvent::runEvent(std::int16_t evt) {
    currEventList_ = &data::gEvent.event(evt);
    currEventSize_ = currEventList_->size();
    currEventIndex_ = 0;

    continueEvents(false);
}

void MapWithEvent::onUseItem(std::int16_t itemId) {
    currEventItem_ = itemId;
    int x, y;
    if (!getFaceOffset(x, y)) {
        return;
    }
    checkEvent(1, x, y);
}

void MapWithEvent::setDirection(Map::Direction dir) {
    if (direction_ == dir) { return; }
    direction_ = dir;
    resetTime();
    currMainCharFrame_ = 0;
    updateMainCharTexture();
}

void MapWithEvent::setPosition(int x, int y, bool checkEvent) {
    currX_ = x;
    currY_ = y;
    cameraX_ = x;
    cameraY_ = y;
    currMainCharFrame_ = 0;
    resting_ = false;
    drawDirty_ = true;
    bool r = tryMove(x, y, checkEvent);
    resetTime();
    if (r) {
        updateMainCharTexture();
    }
}

void MapWithEvent::move(Map::Direction direction) {
    int x, y;
    direction_ = direction;
    if (!getFaceOffset(x, y) || !tryMove(x, y, true)) {
        return;
    }
    resetTime();
    updateMainCharTexture();
}

void MapWithEvent::render() {
    Map::render();
    if (checkTime()) {
        updateMainCharTexture();
    }
}

void MapWithEvent::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeyUp:
        move(Map::DirUp);
        break;
    case KeyRight:
        move(Map::DirRight);
        break;
    case KeyLeft:
        move(Map::DirLeft);
        break;
    case KeyDown:
        move(Map::DirDown);
        break;
    case KeyCancel:
        gWindow->showMainMenu(subMapId_ >= 0);
        break;
    default:
        Map::handleKeyInput(key);
        break;
    }
}

void MapWithEvent::doInteract() {
    currEventItem_ = -1;
    int x, y;
    if (!getFaceOffset(x, y)) {
        return;
    }
    checkEvent(0, x, y);
}

void MapWithEvent::onMove() {
    currEventItem_ = -1;
    checkEvent(2, currX_, currY_);
}

void MapWithEvent::checkEvent(int type, int x, int y) {
    if (subMapId_ < 0) {
        currEventItem_ = -1;
        return;
    }
    auto &layers = mem::gSaveData.subMapLayerInfo[subMapId_]->data;
    auto eventId = layers[3][y * mapWidth_ + x];
    if (eventId < 0) { return; }

    auto &events = mem::gSaveData.subMapEventInfo[subMapId_]->events;
    auto evt = events[eventId].event[type];
    if (evt <= 0) { return; }

    resetTime();
    currMainCharFrame_ = 0;
    updateMainCharTexture();

    currEventId_ = eventId;
    runEvent(evt);
}

bool MapWithEvent::getFaceOffset(int &x, int &y) {
    x = currX_;
    y = currY_;
    switch (direction_) {
    case DirUp:
        if (y > 0) { --y; return true; }
        break;
    case DirRight:
        if (x < mapWidth_ - 1) { ++x; return true; }
        break;
    case DirLeft:
        if (x > 0) { --x; return true; }
        break;
    case DirDown:
        if (y < mapHeight_ - 1) { ++y; return true; }
        break;
    }
    return false;
}

void MapWithEvent::renderChar(int deltaY) {
    if (!showChar_ || !mainCharTex_) { return; }
    int dx = currX_ - cameraX_;
    int dy = currY_ - cameraY_;
    int cellDiffY = cellHeight_ / 2;
    int offsetX = (dx - dy) * cellWidth_ / 2;
    int offsetY = (dx + dy) * cellDiffY;
    renderer_->renderTexture(mainCharTex_, float(x_ + (width_ >> 1)) + float(offsetX) * scale_, float(y_ + (height_ >> 1)) + float(offsetY + cellDiffY - deltaY) * scale_, scale_);
}

void MapWithEvent::resetTime() {
    resting_ = false;
    nextMainTexTime_ = gWindow->currTime() + std::chrono::seconds(currMainCharFrame_ > 0 ? 2 : 5);
}

void MapWithEvent::frameUpdate() {
    if (!moving_.empty()) {
        std::tie(cameraX_, cameraY_) = moving_.back();
        if (movingChar_) {
            tryMove(cameraX_, cameraY_, false);
            if (currX_ != cameraX_ || currY_ != cameraY_) {
                direction_ = calcDirection(currX_, currY_, cameraX_, cameraY_);
                currX_ = cameraX_;
                currY_ = cameraY_;
            }
        }
        moving_.pop_back();
        drawDirty_ = true;
        if (moving_.empty()) {
            continueEvents(false);
        }
    }
    if (animCurrTex_[0] == 0) { return; }
    if (animCurrTex_[0] == animEndTex_[0]) {
        for (int i = 0; i < 3; ++i) {
            animEventId_[i] = 0;
            animCurrTex_[i] = 0;
            animEndTex_[i] = 0;
        }
        continueEvents(false);
        return;
    }
    for (int i = 0; i < 3; ++i) {
        if (animCurrTex_[i] == 0 || animCurrTex_[i] == animEndTex_[i]) { continue; }
        int step = animCurrTex_[i] < animEndTex_[i] ? 1 : -1;
        animCurrTex_[i] += step;
    }
    if (animEventId_[0] < 0) {
        updateMainCharTexture();
    } else {
        for (int i = 0; i < 3; ++i) {
            if (animCurrTex_[i] == 0) { continue; }
            auto &evt = mem::gSaveData.subMapEventInfo[subMapId_]->events[animEventId_[i]];
            evt.currTex = evt.begTex = evt.endTex = animCurrTex_[i];
            setCellTexture(evt.x, evt.y, 3, animCurrTex_[i] >> 1);
        }
    }
}

bool MapWithEvent::checkTime() {
    if (animEventId_[0] < 0) { return false; }
    auto now = gWindow->currTime();
    if (resting_) {
        if (now < nextMainTexTime_) {
            return false;
        }
        currMainCharFrame_ = (currMainCharFrame_ + 1) % 6;
        nextMainTexTime_ = now + std::chrono::milliseconds(500);
        return true;
    }
    if (now < nextMainTexTime_) {
        return false;
    }
    if (currMainCharFrame_ > 0) {
        currMainCharFrame_ = 0;
        nextMainTexTime_ = now + std::chrono::seconds(5);
    } else {
        currMainCharFrame_ = 0;
        resting_ = true;
        nextMainTexTime_ = now + std::chrono::milliseconds(500);
    }
    return true;
}

bool MapWithEvent::closePopup(MapWithEvent *) {
    gWindow->closePopup();
    return true;
}

bool MapWithEvent::doTalk(MapWithEvent *, std::int16_t talkId, std::int16_t headId, std::int16_t position) {
    gWindow->runTalk(data::gEvent.talk(talkId), headId, position);
    return false;
}

bool MapWithEvent::addItem(MapWithEvent *map, std::int16_t itemId, std::int16_t itemCount) {
    mem::gBag.add(itemId, itemCount);
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
    auto &ev = mem::gSaveData.subMapEventInfo[subMapId]->events[eventId];
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
        auto &layer = mem::gSaveData.subMapLayerInfo[subMapId]->data[3];
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
    map->currEventIndex_ = map->currEventSize_;
    gWindow->closePopup();
    return true;
}

bool MapWithEvent::changeExitMusic(MapWithEvent *map, std::int16_t music) {
    mem::gSaveData.subMapInfo[map->subMapId_]->exitMusic = music;
    return true;
}

int MapWithEvent::askForJoinTeam(MapWithEvent *map) {
    gWindow->popupMessageBox({GETTEXT(73)}, MessageBox::YesNo);
    return -1;
}

bool MapWithEvent::joinTeam(MapWithEvent *map, std::int16_t charId) {
    for (auto &id: mem::gSaveData.baseInfo->members) {
        if (id < 0) {
            id = charId;
            auto *charInfo = mem::gSaveData.charInfo[charId];
            if (!charInfo) { continue; }
            for (size_t j = 0; j < data::CarryItemCount; ++j) {
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
    for (auto id: mem::gSaveData.baseInfo->members) {
        if (id < 0) { continue; }
        auto *charInfo = mem::gSaveData.charInfo[id];
        if (!charInfo) { continue; }
        charInfo->stamina = data::StaminaMax;
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
    for (auto id: mem::gSaveData.baseInfo->members) {
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
    mem::gSaveData.subMapLayerInfo[subMapId]->data[layer][y * map->mapWidth_ + x] = value;
    if (subMapId == map->subMapId_) {
        map->setCellTexture(x, y, layer, value >> 1);
    }
    return true;
}

int MapWithEvent::hasItem(MapWithEvent *map, std::int16_t itemId) {
    return mem::gBag[itemId] > 0 ? 1 : 0;
}

bool MapWithEvent::setPlayerPosition(MapWithEvent *map, std::int16_t x, std::int16_t y) {
    map->setPosition(x, y, false);
    return true;
}

int MapWithEvent::checkTeamFull(MapWithEvent *map) {
    for (auto id: mem::gSaveData.baseInfo->members) {
        if (id < 0) {
            return 0;
        }
    }
    return 1;
}

bool MapWithEvent::leaveTeam(MapWithEvent *map, std::int16_t charId) {
    mem::leaveTeam(charId);
    return true;
}

bool MapWithEvent::emptyAllMP(MapWithEvent *map) {
    for (auto id: mem::gSaveData.baseInfo->members) {
        if (id < 0) {
            continue;
        }
        auto *charInfo = mem::gSaveData.charInfo[id];
        if (charInfo) { charInfo->mp = 0; }
    }
    return true;
}

bool MapWithEvent::setAttrPoison(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
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
    auto &ev = mem::gSaveData.subMapEventInfo[subMapId < 0 ? map->subMapId_ : subMapId]->events[eventId];
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
    auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return 0; }
    auto value = charInfo->integrity;
    return value >= low && value <= high ? 1 : 0;
}

int MapWithEvent::checkAttack(MapWithEvent *map, std::int16_t charId, std::int16_t low, std::int16_t high) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
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
    return mem::gBag[data::ItemIDMoney] >= amount;
}

bool MapWithEvent::addItem2(MapWithEvent *map, std::int16_t itemId, std::int16_t itemCount) {
    mem::gBag.add(itemId, itemCount);
    return true;
}

bool MapWithEvent::learnSkill(MapWithEvent *map, std::int16_t charId, std::int16_t skillId, std::int16_t quiet) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    auto *skillInfo = mem::gSaveData.skillInfo[skillId];
    if (!skillInfo) { return true; }

    int found = -1;
    auto learnId = skillInfo->id;
    for (int i = 0; i < data::LearnSkillCount; ++i) {
        auto thisId = charInfo->skillId[i];
        if (thisId == skillInfo->id) {
            if (charInfo->skillLevel[i] < data::SkillLevelMaxDiv * 100) {
                charInfo->skillLevel[i] += 100;
            }
            found = -1;
            break;
        }
        if (thisId < 0) {
            if (found < 0) {
                found = i;
            }
        }
    }
    if (found >= 0) {
        charInfo->skillId[found] = learnId;
        charInfo->skillLevel[found] = 0;
    }
    if (quiet) {
        return true;
    }
    gWindow->popupMessageBox({fmt::format(L"{} {} {}", GETCHARNAME(charId), GETTEXT(75), GETSKILLNAME(skillId))}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::addPotential(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->potential = std::clamp<std::int16_t>(charInfo->potential + value, 0, data::PotentialMax);
    gWindow->popupMessageBox({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(29), GETTEXT(33), std::to_wstring(value))}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::setSkill(MapWithEvent *map, std::int16_t charId, std::int16_t skillIndex,
                            std::int16_t skillId, std::int16_t level) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->skillId[skillIndex] = skillId;
    charInfo->skillLevel[skillIndex] = level;
    return true;
}

int MapWithEvent::checkSex(MapWithEvent *map, std::int16_t sex) {
    auto *charInfo = mem::gSaveData.charInfo[0];
    if (!charInfo) { return 0; }
    return charInfo->sex == sex ? 1 : 0;
}

bool MapWithEvent::addIntegrity(MapWithEvent *map, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[0];
    if (!charInfo) { return true; }
    charInfo->integrity = std::clamp<std::int16_t>(charInfo->integrity + value, 0, data::IntegrityMax);
    return true;
}

bool MapWithEvent::modifySubMapLayerTex(MapWithEvent *map, std::int16_t subMapId, std::int16_t layer,
                                        std::int16_t oldTex, std::int16_t newTex) {
    if (subMapId < 0) {
        subMapId = map->subMapId_;
    }
    auto &l = mem::gSaveData.subMapLayerInfo[subMapId]->data[layer];
    auto pos = 0;
    bool currentMap = subMapId == map->subMapId_;
    for (int y = 0; y < data::SubMapHeight; ++y) {
        for (int x = 0; x < data::SubMapWidth; ++x) {
            if (l[pos] == oldTex) {
                l[pos] = newTex;
                if (currentMap) {
                    map->setCellTexture(x, y, layer, newTex >> 1);
                }
            }
            ++pos;
        }
    }
    if (currentMap) { map->drawDirty_ = true; }
    return true;
}

bool MapWithEvent::openSubMap(MapWithEvent *, std::int16_t subMapId) {
    mem::gSaveData.subMapInfo[subMapId]->enterCondition = 0;
    return true;
}

bool MapWithEvent::forceDirection(MapWithEvent *map, std::int16_t direction) {
    map->setDirection(Direction(direction));
    map->updateMainCharTexture();
    return true;
}

bool MapWithEvent::addItemToChar(MapWithEvent *map, std::int16_t charId, std::int16_t itemId, std::int16_t itemCount) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    int firstEmpty = -1;
    for (int i = 0; i < data::CarryItemCount; ++i) {
        if (charInfo->item[i] < 0) {
            firstEmpty = i;
            continue;
        }
        if (charInfo->item[i] == itemId) {
            charInfo->itemCount[i] += itemCount;
            return true;
        }
    }
    if (firstEmpty < 0) {
        return true;
    }
    charInfo->item[firstEmpty] = itemId;
    charInfo->itemCount[firstEmpty] = itemCount;
    return true;
}

int MapWithEvent::checkFemaleInTeam(MapWithEvent *map) {
    for (auto id: mem::gSaveData.baseInfo->members) {
        if (id < 0) { continue; }
        auto *charInfo = mem::gSaveData.charInfo[id];
        if (!charInfo) { continue; }
        if (charInfo->sex == 1) {
            return 1;
        }
    }
    return 0;
}

bool MapWithEvent::animation2(MapWithEvent *map, std::int16_t eventId, std::int16_t begTex, std::int16_t endTex,
                              std::int16_t eventId2, std::int16_t begTex2, std::int16_t endTex2) {
    if (map->subMapId_ < 0) { return true; }
    map->animEventId_[0] = eventId;
    map->animCurrTex_[0] = begTex;
    map->animEndTex_[0] = endTex;
    map->animEventId_[1] = eventId2;
    map->animCurrTex_[1] = begTex2;
    map->animEndTex_[1] = endTex2;
    return false;
}

bool MapWithEvent::animation3(MapWithEvent *map, std::int16_t eventId, std::int16_t begTex, std::int16_t endTex,
                              std::int16_t eventId2, std::int16_t begTex2,
                              std::int16_t eventId3, std::int16_t begTex3) {
    if (map->subMapId_ < 0) { return true; }
    map->animEventId_[0] = eventId;
    map->animCurrTex_[0] = begTex;
    map->animEndTex_[0] = endTex;
    map->animEventId_[1] = eventId2;
    map->animCurrTex_[1] = begTex2;
    map->animEndTex_[1] = begTex2 + (endTex - begTex);
    map->animEventId_[1] = eventId3;
    map->animCurrTex_[1] = begTex3;
    map->animEndTex_[1] = begTex3 + (endTex - begTex);
    return false;
}

bool MapWithEvent::addSpeed(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->speed = std::clamp<std::int16_t>(charInfo->speed + value, 0, data::SpeedMax);
    gWindow->popupMessageBox({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(9), GETTEXT(33), std::to_wstring(value))}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::addMaxMP(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->maxMp = std::clamp<std::int16_t>(charInfo->maxMp + value, 0, data::MpMax);
    gWindow->popupMessageBox({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(7), GETTEXT(33), std::to_wstring(value))}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::addAttack(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->attack = std::clamp<std::int16_t>(charInfo->attack + value, 0, data::AttackMax);
    gWindow->popupMessageBox({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(8), GETTEXT(33), std::to_wstring(value))}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::addMaxHP(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->maxHp = std::clamp<std::int16_t>(charInfo->maxHp + value, 0, data::HpMax);
    gWindow->popupMessageBox({fmt::format(L"{} {}{} {}", GETCHARNAME(charId), GETTEXT(2), GETTEXT(33), std::to_wstring(value))}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::setMPType(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->mpType = value;
    return true;
}

int MapWithEvent::checkHas5Item(MapWithEvent *map, std::int16_t itemId0, std::int16_t itemId1, std::int16_t itemId2,
                                std::int16_t itemId3, std::int16_t itemId4) {
    return mem::gBag[itemId0] > 0
        && mem::gBag[itemId1] > 0
        && mem::gBag[itemId2] > 0
        && mem::gBag[itemId3] > 0
        && mem::gBag[itemId4] > 0 ? 1 : 0;
}

bool MapWithEvent::tutorialTalk(MapWithEvent *map) {
    return doTalk(map, 2547 + util::gRandom(18), 114, 0);
}

bool MapWithEvent::showIntegrity(MapWithEvent *map) {
    auto *charInfo = mem::gSaveData.charInfo[0];
    if (!charInfo) { return true; }
    gWindow->popupMessageBox({GETTEXT(76) + L' ' + std::to_wstring(charInfo->integrity)}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::showReputation(MapWithEvent *map) {
    auto *charInfo = mem::gSaveData.charInfo[0];
    if (!charInfo) { return true; }
    gWindow->popupMessageBox({GETTEXT(77) + L' ' + std::to_wstring(charInfo->reputation)}, MessageBox::PressToCloseTop);
    return false;
}

bool MapWithEvent::openWorld(MapWithEvent *) {
    auto sz = mem::gSaveData.subMapInfo.size();
    for (size_t i = 0; i < sz; ++i) {
        mem::gSaveData.subMapInfo[i]->enterCondition = 0;
    }
    mem::gSaveData.subMapInfo[2]->enterCondition = 2;
    mem::gSaveData.subMapInfo[38]->enterCondition = 2;
    mem::gSaveData.subMapInfo[75]->enterCondition = 1;
    mem::gSaveData.subMapInfo[80]->enterCondition = 1;
    return true;
}

int MapWithEvent::checkEventID(MapWithEvent *map, std::int16_t eventId, std::int16_t value) {
    return mem::gSaveData.subMapEventInfo[map->subMapId_]->events[eventId].event[0] == value ? 1 : 0;
}

bool MapWithEvent::addReputation(MapWithEvent *map, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[0];
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
            gWindow->enterWar(102 + i * 2 + n, false, true);
            return false;
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
    for (int i = data::TeamMemberCount - 1; i > 0; --i) {
        auto charId = mem::gSaveData.baseInfo->members[i];
        if (charId > 0) {
            mem::leaveTeam(charId);
        }
    }
    return true;
}

int MapWithEvent::checkSubMapTex(MapWithEvent *map, std::int16_t subMapId, std::int16_t eventId, std::int16_t tex) {
    const auto &evt = mem::gSaveData.subMapEventInfo[subMapId < 0 ? map->subMapId_ : subMapId]->events[eventId];
    return (evt.currTex == tex || evt.begTex == tex || evt.endTex == tex) ? 1 : 0;
}

int MapWithEvent::checkAllStoryBooks(MapWithEvent *map) {
    const auto &events = mem::gSaveData.subMapEventInfo[map->subMapId_]->events;
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
    auto *charInfo = mem::gSaveData.charInfo[charId];
    if (!charInfo) { return true; }
    charInfo->sex = value;
    return true;
}

struct ShopEventInfo {
    std::int16_t subMapId;
    std::int16_t shopEventIndex;
    std::vector<std::int16_t> randomEventIndex;
};

const ShopEventInfo shopEventInfo[5] = {
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
            auto &evts = mem::gSaveData.subMapEventInfo[map->subMapId_]->events;
            for (auto &n: evi.randomEventIndex) {
                evts[n].event[2] = data::RandomShopEventId;
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
            auto &evts = mem::gSaveData.subMapEventInfo[map->subMapId_]->events;
            auto &ev = evts[evi.shopEventIndex];
            ev.blocked = 0;
            ev.event[0] = -1;
            ev.currTex = ev.begTex = ev.endTex = -1;
            for (auto &n: evi.randomEventIndex) {
                evts[n].event[2] = -1;
            }
            break;
        }
    }
    const auto &evi = shopEventInfo[util::gRandom(5)];
    auto &ev = mem::gSaveData.subMapEventInfo[evi.subMapId]->events[evi.shopEventIndex];
    ev.blocked = 1;
    ev.event[0] = data::ShopEventId;
    ev.begTex = ev.currTex = ev.endTex = data::ShopEventTex;
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
