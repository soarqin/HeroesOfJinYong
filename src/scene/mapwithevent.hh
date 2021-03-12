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

#pragma once

#include "map.hh"

#include <functional>
#include <list>

namespace hojy::scene {

class MapWithEvent: public Map {
public:
    using Map::Map;

    void continueEvents(bool result = true);
    void runEvent(std::int16_t evt);
    void onUseItem(std::int16_t itemId);

    [[nodiscard]] std::int16_t currX() const { return currX_; }
    [[nodiscard]] std::int16_t currY() const { return currY_; }
    [[nodiscard]] Map::Direction direction() const { return direction_; }
    void setDirection(Direction dir);
    void setPosition(int x, int y, bool checkEvent = true);
    void move(Direction direction);

    void render() override;
    void handleKeyInput(Key key) override;

protected:
    void doInteract();
    void onMove();
    void checkEvent(int type, int x, int y);

    bool getFaceOffset(int &x, int &y);
    void renderChar(int deltaY = 0);

    virtual bool tryMove(int x, int y, bool checkEvent) { return false; }
    virtual void updateMainCharTexture() {}

    void resetTime() override;
    void frameUpdate() override;
    virtual bool checkTime();
    virtual void setCellTexture(int x, int y, int layer, std::int16_t tex) {}

private:
    static bool doTalk(MapWithEvent *map, std::int16_t talkId, std::int16_t headId, std::int16_t position);
    static bool addItem(MapWithEvent *map, std::int16_t itemId, std::int16_t itemCount);
    static bool modifyEvent(MapWithEvent *map, std::int16_t subMapId, std::int16_t eventId, std::int16_t blocked,
                            std::int16_t index, std::int16_t event1, std::int16_t event2, std::int16_t event3,
                            std::int16_t currTex, std::int16_t endTex, std::int16_t begTex, std::int16_t texDelay,
                            std::int16_t x, std::int16_t y);
    static int useItem(MapWithEvent *map, std::int16_t itemId);
    static int askForWar(MapWithEvent *map);
    static bool changeExitMusic(MapWithEvent *map, std::int16_t music);
    static int askForJoinTeam(MapWithEvent *map);
    static bool joinTeam(MapWithEvent *map, std::int16_t charId);
    static int wantSleep(MapWithEvent *map);
    static bool sleep(MapWithEvent *map);
    static bool makeBright(MapWithEvent *map);
    static bool makeDim(MapWithEvent *map);
    static bool die(MapWithEvent *map);
    static int checkTeamMember(MapWithEvent *map, std::int16_t charId);
    static bool changeLayer(MapWithEvent *map, std::int16_t subMapId, std::int16_t layer,
                            std::int16_t x, std::int16_t y, std::int16_t value);
    static int hasItem(MapWithEvent *map, std::int16_t itemId);
    static bool setCameraPosition(MapWithEvent *map, std::int16_t x, std::int16_t y);
    static int checkTeamFull(MapWithEvent *map);
    static bool leaveTeam(MapWithEvent *map, std::int16_t charId);
    static bool emptyAllMP(MapWithEvent *map);
    static bool setAttrPoison(MapWithEvent *map, std::int16_t charId, std::int16_t value);
    static bool moveCamera(MapWithEvent *map, std::int16_t x0, std::int16_t y0, std::int16_t x1, std::int16_t y1);
    static bool modifyEventId(MapWithEvent *map, std::int16_t subMapId, std::int16_t eventId,
                              std::int16_t ev0, std::int16_t ev1, std::int16_t ev2);
    static bool animation(MapWithEvent *map, std::int16_t eventId, std::int16_t begTex, std::int16_t endTex);
    static int checkIntegrity(MapWithEvent *map, std::int16_t charId, std::int16_t low, std::int16_t high);
    static int checkAttack(MapWithEvent *map, std::int16_t charId, std::int16_t low, std::int16_t high);
    static bool walkPath(MapWithEvent *map, std::int16_t x0, std::int16_t y0, std::int16_t x1, std::int16_t y1);
    static int checkMoney(MapWithEvent *map, std::int16_t amount);
    static bool addItem2(MapWithEvent *map, std::int16_t itemId, std::int16_t itemCount);
    static bool learnSkill(MapWithEvent *map, std::int16_t charId, std::int16_t skillId, std::int16_t quiet);
    static bool addPotential(MapWithEvent *map, std::int16_t charId, std::int16_t value);
    static bool setSkill(MapWithEvent *map, std::int16_t charId, std::int16_t skillIndex,
                         std::int16_t skillId, std::int16_t level);
    static int checkSex(MapWithEvent *map, std::int16_t sex);
    static bool addIntegrity(MapWithEvent *map, std::int16_t value);
    static bool modifySubMapLayerTex(MapWithEvent *map, std::int16_t subMapId, std::int16_t layer, std::int16_t oldTex, std::int16_t newTex);
    static bool openSubMap(MapWithEvent *map, std::int16_t subMapId);
    static bool forceDirection(MapWithEvent *map, std::int16_t direction);
    static bool addItemToChar(MapWithEvent *map, std::int16_t charId, std::int16_t itemId, std::int16_t itemCount);
    static int checkFemaleInTeam(MapWithEvent *map);
    static bool animation2(MapWithEvent *map, std::int16_t eventId, std::int16_t begTex, std::int16_t endTex,
                           std::int16_t eventId2, std::int16_t begTex2, std::int16_t endTex2);
    static bool animation3(MapWithEvent *map, std::int16_t eventId, std::int16_t begTex, std::int16_t endTex,
                           std::int16_t eventId2, std::int16_t begTex2,
                           std::int16_t eventId3, std::int16_t begTex3);
    static bool addSpeed(MapWithEvent *map, std::int16_t charId, std::int16_t value);
    static bool addMaxMP(MapWithEvent *map, std::int16_t charId, std::int16_t value);
    static bool addAttack(MapWithEvent *map, std::int16_t charId, std::int16_t value);
    static bool addMaxHP(MapWithEvent *map, std::int16_t charId, std::int16_t value);
    static bool setMPType(MapWithEvent *map, std::int16_t charId, std::int16_t value);
    static int checkHas5Item(MapWithEvent *map, std::int16_t itemId0, std::int16_t itemId1, std::int16_t itemId2,
                             std::int16_t itemId3, std::int16_t itemId4);
    static bool tutorialTalk(MapWithEvent *map);
    static bool showIntegrity(MapWithEvent *map);
    static bool showReputation(MapWithEvent *map);
    static bool openWorld(MapWithEvent *map);
    static int checkEventID(MapWithEvent *map, std::int16_t eventId, std::int16_t value);
    static bool addReputation(MapWithEvent *map, std::int16_t value);
    static bool removeBarrier(MapWithEvent *map);
    static bool tournament(MapWithEvent *map);
    static bool disbandTeam(MapWithEvent *map);
    static int checkSubMapTex(MapWithEvent *map, std::int16_t subMapId, std::int16_t eventId, std::int16_t tex);
    static int checkAllStoryBooks(MapWithEvent *map);
    static bool goBackHome(MapWithEvent *map, std::int16_t eventId, std::int16_t begTex, std::int16_t endTex,
                           std::int16_t eventId2, std::int16_t begTex2, std::int16_t endTex2);
    static bool setSex(MapWithEvent *map, std::int16_t charId, std::int16_t value);
    static bool openShop(MapWithEvent *map);
    static bool playMusic(MapWithEvent *map, std::int16_t musicId);
    static bool playSound(MapWithEvent *map, std::int16_t soundId);

protected:
    bool currEventPaused_ = false;
    std::int16_t currEventId_ = -1;
    size_t currEventIndex_ = 0, currEventSize_ = 0;
    size_t currEventAdvTrue_ = 0, currEventAdvFalse_ = 0;
    std::int16_t currEventItem_ = -1;
    const std::vector<std::int16_t> *currEventList_ = nullptr;

    const Texture *mainCharTex_ = nullptr;
    std::int32_t currX_ = 0, currY_ = 0;
    Direction direction_ = DirUp;

    bool resting_ = false;
    std::int32_t currMainCharFrame_ = 0;
    std::chrono::steady_clock::time_point nextMainTexTime_;

    std::int16_t animEventId_ = 0, animCurrTex_ = 0, animEndTex_ = 0;

    std::list<std::function<bool()>> pendingSubEvents_;
};

}
