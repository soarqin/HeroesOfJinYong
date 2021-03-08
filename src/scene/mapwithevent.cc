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
#include "core/config.hh"
#include "util/random.hh"
#include "util/conv.hh"

namespace hojy::scene {

void MapWithEvent::render() {
    Map::render();
    ++frames_;
    if (gWindow->currTime() < nextEventCheck_) { return; }
    nextEventCheck_ += std::chrono::microseconds(int(100000.f / core::config.animationSpeed()));
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

void MapWithEvent::continueEvents(bool result) {
    if (!pendingSubEvents_.empty()) {
        currEventPaused_ = false;
        auto func = std::move(pendingSubEvents_.front());
        pendingSubEvents_.pop_front();
        func();
        return;
    }
    if (!currEventList_) { return; }
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
    while (!currEventPaused_ && currEventIndex_ < currEventSize_) {
        auto op = evlist[currEventIndex_++];
#ifndef NDEBUG
        fprintf(stdout, "%2d: ", op);
#endif
        if (op == 0) {
#ifndef NDEBUG
            fprintf(stdout, "\n");
            fflush(stdout);
#endif
            gWindow->closePopup();
            continue;
        }
        if (op == -1 || op == 7) {
            currEventIndex_ = currEventSize_;
#ifndef NDEBUG
            fprintf(stdout, "\n");
            fflush(stdout);
#endif
            gWindow->closePopup();
            break;
        }
        switch (op) {
        OpRun(1, doTalk);
        OpRun(2, addItem);
        OpRun(3, modifyEvent);
        OpRun(4, useItem);
        OpRun(5, tryStartFight);
        case 6:
            currEventAdvTrue_ = evlist[currEventIndex_ + 1];
            currEventAdvFalse_ = evlist[currEventIndex_ + 2];
            /* TODO: Fight with parameter evlist[currEventIndex_] and evlist[currEventIndex_ + 3] */
            currEventIndex_ += 4;
            /* currEventPaused_ = true; */
            break;
        OpRun(8, changeExitMusic);
        OpRun(9, wantJoinTeam);
        OpRun(10, joinTeam);
        OpRun(11, wantSleep);
        OpRun(12, sleep);
        OpRun(13, makeBright);
        OpRun(14, makeDim);
        OpRun(15, die);
        OpRun(16, checkTeamMember);
        OpRun(17, changeLayer);
        OpRun(18, hasItem);
        OpRun(19, setCameraPosition);
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
    currEventItem_ = -1;
    int x, y;
    if (!getFaceOffset(x, y)) {
        return;
    }
    checkEvent(0, x, y);
}

void MapWithEvent::onUseItem(std::int16_t itemId) {
    currEventItem_ = itemId;
    checkEvent(1, currX_, currY_);
}

void MapWithEvent::onMove() {
    currEventItem_ = -1;
    checkEvent(2, currX_, currY_);
}

void MapWithEvent::checkEvent(int type, int x, int y) {
    auto &layers = mem::gSaveData.subMapLayerInfo[subMapId_]->data;
    auto eventId = layers[3][y * mapWidth_ + x];
    if (eventId < 0) { return; }

    auto &events = mem::gSaveData.subMapEventInfo[subMapId_]->events;
    auto evt = events[eventId].event[type];
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

bool MapWithEvent::checkTime() {
    if (animEventId_ < 0) { return false; }
    return Map::checkTime();
}

void MapWithEvent::updateEventTextures() {
    if (animCurrTex_ == 0) { return; }
    if (animCurrTex_ == animEndTex_) {
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
        setCellTexture(evt.x, evt.y, 3, animCurrTex_ >> 1);
    }
}

bool MapWithEvent::doTalk(MapWithEvent *map, std::int16_t talkId, std::int16_t headId, std::int16_t position) {
    gWindow->runTalk(data::gEvent.talk(talkId), headId, position);
    return false;
}

bool MapWithEvent::addItem(MapWithEvent *map, std::int16_t itemId, std::int16_t itemCount) {
    mem::gBag.add(itemId, itemCount);
    gWindow->popupMessageBox({L"獲得 " + util::big5Conv.toUnicode(mem::gSaveData.itemInfo[itemId]->name) + L'x' + std::to_wstring(itemCount)}, MessageBox::ClickToClose);
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
        ev.x = x; ev.y = y;
    }
    if (currTex > -2) {
        ev.currTex = currTex;
        map->setCellTexture(x, y, 3, currTex >> 1);
    }
    return true;
}

int MapWithEvent::useItem(MapWithEvent *map, std::int16_t itemId) {
    return itemId == map->currEventItem_ ? 1 : 0;
}

int MapWithEvent::tryStartFight(MapWithEvent *map) {
    /* TODO: implement this */
    return 0;
}

bool MapWithEvent::changeExitMusic(MapWithEvent *map, std::int16_t music) {
    mem::gSaveData.subMapInfo[map->subMapId_]->exitMusic = music;
    return true;
}

int MapWithEvent::wantJoinTeam(MapWithEvent *map) {
    /* TODO: implement this */
    return 0;
}

bool MapWithEvent::joinTeam(MapWithEvent *map, std::int16_t charId) {
    for (size_t i = 0; i < mem::TeamMemberCount; ++i) {
        if (mem::gSaveData.baseInfo->members[i] < 0) {
            mem::gSaveData.baseInfo->members[i] = charId;
            auto *charInfo = mem::gSaveData.charInfo[charId];
            for (size_t j = 0; j < mem::CarryItemCount; ++j) {
                if (charInfo->item[j] >= 0) {
                    if (charInfo->itemCount[j] == 0) { charInfo->itemCount[j] = 1; }
                    auto itemId = charInfo->item[j];
                    std::int16_t itemCount = charInfo->itemCount[j] == 0 ? 1 : charInfo->itemCount[j];
                    map->pendingSubEvents_.emplace_back([map, itemId, itemCount]()->bool {
                        addItem(map, itemId, itemCount);
                        return false;
                    });
                    charInfo->item[j] = -1;
                    charInfo->itemCount[j] = 0;
                }
            }
            break;
        }
    }
    return map->pendingSubEvents_.empty();
}

int MapWithEvent::wantSleep(MapWithEvent *map) {
    gWindow->popupMessageBox({L"請選擇是或否？"}, MessageBox::YesNo);
    return -1;
}

bool MapWithEvent::sleep(MapWithEvent *map) {
    for (int i = 0; i < mem::TeamMemberCount; ++i) {
        auto id = mem::gSaveData.baseInfo->members[i];
        if (id <= 0) { continue; }
        auto *charInfo = mem::gSaveData.charInfo[id];
        charInfo->stamina = mem::StaminaMax;
        charInfo->hp = charInfo->maxHp;
        charInfo->mp = charInfo->maxMp;
        charInfo->hurt = 0;
        charInfo->poisoned = 0;
    }
    return true;
}

bool MapWithEvent::makeBright(MapWithEvent *map) {
    /* TODO: implement this */
    return true;
}

bool MapWithEvent::makeDim(MapWithEvent *map) {
    /* TODO: implement this */
    return true;
}

bool MapWithEvent::die(MapWithEvent *map) {
    /* TODO: implement this */
    return true;
}

int MapWithEvent::checkTeamMember(MapWithEvent *map, std::int16_t charId) {
    for (size_t i = 0; i < mem::TeamMemberCount; ++i) {
        if (mem::gSaveData.baseInfo->members[i] == charId) {
            return 1;
        }
    }
    return 0;
}

bool MapWithEvent::changeLayer(MapWithEvent *map, std::int16_t subMapId, std::int16_t layer,
                               std::int16_t x, std::int16_t y, std::int16_t value) {
    mem::gSaveData.subMapLayerInfo[map->subMapId_]->data[layer][y * map->mapWidth_ + x] = value;
    map->setCellTexture(x, y, layer, value >> 1);
    return true;
}

int MapWithEvent::hasItem(MapWithEvent *map, std::int16_t itemId) {
    return mem::gBag[itemId] > 0 ? 1 : 0;
}

bool MapWithEvent::setCameraPosition(MapWithEvent *map, std::int16_t x, std::int16_t y) {
    /* TODO: implement this */
    return true;
}

int MapWithEvent::checkTeamFull(MapWithEvent *map) {
    for (size_t i = 0; i < mem::TeamMemberCount; ++i) {
        if (mem::gSaveData.baseInfo->members[i] < 0) {
            return 0;
        }
    }
    return 1;
}

bool MapWithEvent::leaveTeam(MapWithEvent *map, std::int16_t charId) {
    for (size_t i = 0; i < mem::TeamMemberCount; ++i) {
        if (mem::gSaveData.baseInfo->members[i] == charId) {
            for (size_t j = i; j < mem::TeamMemberCount - 1; ++j) {
                mem::gSaveData.baseInfo->members[j] = mem::gSaveData.baseInfo->members[j + 1];
            }
            mem::gSaveData.baseInfo->members[mem::TeamMemberCount - 1] = -1;
            break;
        }
    }
    return true;
}

bool MapWithEvent::emptyAllMP(MapWithEvent *map) {
    for (size_t i = 0; i < mem::TeamMemberCount; ++i) {
        auto id = mem::gSaveData.baseInfo->members[i];
        if (id < 0) {
            continue;
        }
        mem::gSaveData.charInfo[id]->mp = 0;
    }
    return true;
}

bool MapWithEvent::setAttrPoison(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    mem::gSaveData.charInfo[charId]->poison = value;
    return true;
}

bool MapWithEvent::moveCamera(MapWithEvent *map, std::int16_t x0, std::int16_t y0, std::int16_t x1, std::int16_t y1) {
    /* TODO: implement this */
    return true;
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
    map->animEventId_ = eventId;
    map->animCurrTex_ = begTex;
    map->animEndTex_ = endTex;
    return false;
}

int MapWithEvent::checkIntegrity(MapWithEvent *map, std::int16_t charId, std::int16_t low, std::int16_t high) {
    auto value = mem::gSaveData.charInfo[charId]->integrity;
    return value >= low && value <= high ? 1 : 0;
}

int MapWithEvent::checkAttack(MapWithEvent *map, std::int16_t charId, std::int16_t low, std::int16_t high) {
    auto value = mem::gSaveData.charInfo[charId]->attack;
    return value >= low ? 1 : 0;
}

bool MapWithEvent::walkPath(MapWithEvent *map, std::int16_t x0, std::int16_t y0, std::int16_t x1, std::int16_t y1) {
    /* TODO: implement this */
    return true;
}

int MapWithEvent::checkMoney(MapWithEvent *map, std::int16_t amount) {
    return mem::gBag[mem::ItemIDMoney] >= amount;
}

bool MapWithEvent::addItem2(MapWithEvent *map, std::int16_t itemId, std::int16_t itemCount) {
    mem::gBag.add(itemId, itemCount);
    return true;
}

bool MapWithEvent::learnSkill(MapWithEvent *map, std::int16_t charId, std::int16_t skillId, std::int16_t quiet) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    auto *skillInfo = mem::gSaveData.skillInfo[skillId];

    int found = -1;
    auto learnId = skillInfo->id;
    for (int i = 0; i < mem::LearnSkillCount; ++i) {
        auto thisId = charInfo->skillId[i];
        if (thisId == skillInfo->id) {
            if (charInfo->skillLevel[i] < mem::SkillLevelMaxDiv * 100) {
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
    gWindow->popupMessageBox({util::big5Conv.toUnicode(charInfo->name) + L" 習得武學 " + util::big5Conv.toUnicode(skillInfo->name)}, MessageBox::ClickToClose);
    return false;
}

bool MapWithEvent::addPotential(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    charInfo->potential = std::clamp<std::int16_t>(charInfo->potential + value, 0, mem::PotentialMax);
    gWindow->popupMessageBox({util::big5Conv.toUnicode(charInfo->name) + L" 資質增加 " + std::to_wstring(value)}, MessageBox::ClickToClose);
    return false;
}

bool MapWithEvent::setSkill(MapWithEvent *map, std::int16_t charId, std::int16_t skillIndex,
                            std::int16_t skillId, std::int16_t level) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    charInfo->skillId[skillIndex] = skillId;
    charInfo->skillLevel[skillIndex] = level;
    return true;
}

int MapWithEvent::checkSex(MapWithEvent *map, std::int16_t sex) {
    return mem::gSaveData.charInfo[0]->sex == sex ? 1 : 0;
}

bool MapWithEvent::addIntegrity(MapWithEvent *map, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[0];
    charInfo->integrity = std::clamp<std::int16_t>(charInfo->integrity + value, 0, mem::IntegrityMax);
    return true;
}

bool MapWithEvent::modifySubMapLayerTex(MapWithEvent *map, std::int16_t subMapId, std::int16_t layer,
                                        std::int16_t oldTex, std::int16_t newTex) {
    auto &l = mem::gSaveData.subMapLayerInfo[subMapId < 0 ? map->subMapId_ : subMapId]->data[layer];
    auto pos = 0;
    bool currentMap = subMapId == map->subMapId_;
    for (int y = 0; y < mem::SubMapHeight; ++y) {
        for (int x = 0; x < mem::SubMapWidth; ++x) {
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
    int firstEmpty = -1;
    for (int i = 0; i < mem::CarryItemCount; ++i) {
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
    for (int i = 0; i < mem::TeamMemberCount; ++i) {
        auto id = mem::gSaveData.baseInfo->members[i];
        if (id < 0) { continue; }
        if (mem::gSaveData.charInfo[id]->sex == 1) {
            return 1;
        }
    }
    return 0;
}

bool MapWithEvent::animation2(MapWithEvent *map, std::int16_t eventId, std::int16_t begTex, std::int16_t endTex,
                              std::int16_t eventId2, std::int16_t begTex2, std::int16_t endTex2) {
    /* TODO: implement this */
    return true;
}

bool MapWithEvent::animation3(MapWithEvent *map, std::int16_t eventId, std::int16_t begTex, std::int16_t endTex,
                              std::int16_t eventId2, std::int16_t begTex2,
                              std::int16_t eventId3, std::int16_t begTex3) {
    /* TODO: implement this */
    return true;
}

bool MapWithEvent::addSpeed(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    charInfo->speed = std::clamp<std::int16_t>(charInfo->speed + value, 0, mem::SpeedMax);
    gWindow->popupMessageBox({util::big5Conv.toUnicode(charInfo->name) + L" 輕功增加 " + std::to_wstring(value)}, MessageBox::ClickToClose);
    return false;
}

bool MapWithEvent::addMaxMP(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    charInfo->maxMp = std::clamp<std::int16_t>(charInfo->maxMp + value, 0, mem::MPMax);
    gWindow->popupMessageBox({util::big5Conv.toUnicode(charInfo->name) + L" 內力增加 " + std::to_wstring(value)}, MessageBox::ClickToClose);
    return false;
}

bool MapWithEvent::addAttack(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    charInfo->attack = std::clamp<std::int16_t>(charInfo->attack + value, 0, mem::AttackMax);
    gWindow->popupMessageBox({util::big5Conv.toUnicode(charInfo->name) + L" 武力增加 " + std::to_wstring(value)}, MessageBox::ClickToClose);
    return false;
}

bool MapWithEvent::addMaxHP(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    charInfo->maxHp = std::clamp<std::int16_t>(charInfo->maxHp + value, 0, mem::HPMax);
    gWindow->popupMessageBox({util::big5Conv.toUnicode(charInfo->name) + L" 生命增加 " + std::to_wstring(value)}, MessageBox::ClickToClose);
    return false;
}

bool MapWithEvent::setMPType(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    mem::gSaveData.charInfo[charId]->mpType = value;
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
    gWindow->popupMessageBox({L"你的道德指數為 " + std::to_wstring(mem::gSaveData.charInfo[0]->integrity)}, MessageBox::ClickToClose);
    return false;
}

bool MapWithEvent::showReputation(MapWithEvent *map) {
    gWindow->popupMessageBox({L"你的聲望指數為 " + std::to_wstring(mem::gSaveData.charInfo[0]->reputation)}, MessageBox::ClickToClose);
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
    /* TODO: implement this */
    return true;
}

bool MapWithEvent::disbandTeam(MapWithEvent *map) {
    for (int i = 1; i < mem::TeamMemberCount; ++i) {
        mem::gSaveData.baseInfo->members[i] = -1;
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
    /* TODO: implement this */
    return true;
}

bool MapWithEvent::setSex(MapWithEvent *map, std::int16_t charId, std::int16_t value) {
    auto *charInfo = mem::gSaveData.charInfo[charId];
    charInfo->sex = value;
    return true;
}

bool MapWithEvent::openShop(MapWithEvent *map) {
    doTalk(map, 0xB9E, 0x6F, 0);
    /* TODO: popup shop ui */
    map->pendingSubEvents_.emplace_back([map]() {
        gWindow->closePopup();
        return doTalk(map, 0xBA0, 0x6F, 0);
    });
    return false;
}

bool MapWithEvent::playMusic(MapWithEvent *, std::int16_t musicId) {
    gWindow->playMusic(musicId);
    return true;
}

bool MapWithEvent::playSound(MapWithEvent *map, std::int16_t soundId) {
    if (soundId < 24) {
        gWindow->playAtkSound(soundId);
    } else {
        gWindow->playEffectSound(soundId - 24);
    }
    return true;
}

}
