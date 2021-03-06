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

namespace hojy::scene {

class MapWithEvent: public Map {
public:
    using Map::Map;

    void render() override;
    void continueEvents(bool result = true);

protected:
    void doInteract();
    void onMove();

    bool checkTime() override;
    virtual void setCellTexture(int x, int y, std::int16_t tex) {}
    virtual void updateEventTextures();

private:
    static void doTalk(MapWithEvent *map, std::int16_t talkId, std::int16_t headId, std::int16_t position);
    static void addItem(MapWithEvent *map, std::int16_t itemId, std::int16_t itemCount);
    static void modifyEvent(MapWithEvent *map, std::int16_t subMapId, std::int16_t eventId, std::int16_t blocked, std::int16_t index,
                     std::int16_t event1, std::int16_t event2, std::int16_t event3, std::int16_t currTex,
                     std::int16_t endTex, std::int16_t begTex, std::int16_t texDelay, std::int16_t x,
                     std::int16_t y);
    static void animation(MapWithEvent *map, std::int16_t eventId, std::int16_t begTex, std::int16_t endTex);
    static void openSubMap(MapWithEvent *map, std::int16_t subMapId);
    static void forceDirection(MapWithEvent *map, std::int16_t direction);
    static void tutorialTalk(MapWithEvent *map);
    static void openWorld(MapWithEvent *map);
    static void playMusic(MapWithEvent *map, std::int16_t musicId);
    static void playSound(MapWithEvent *map, std::int16_t soundId);

protected:
    std::int16_t subMapId_ = -1;

    bool currEventPaused_ = false;
    std::int16_t currEventId_ = -1;
    size_t currEventIndex_ = 0, currEventSize_ = 0;
    size_t currEventAdvTrue_ = 0, currEventAdvFalse_ = 0;
    const std::vector<std::int16_t> *currEventList_ = nullptr;

    std::uint64_t frames_ = 0;
    std::int16_t animEventId_ = 0, animCurrTex_ = 0, animEndTex_ = 0;
};

}
