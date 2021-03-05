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

    void continueEvents();

protected:
    void doInteract();
    void onMove();

    virtual void setCellTexture(int x, int y, std::int16_t tex) {}

private:
    void doTalk(std::int16_t talkId, std::int16_t headId, std::int16_t position);
    void addItem(std::int16_t itemId, std::int16_t itemCount);
    void modifyEvent(std::int16_t subMapId, std::int16_t eventId, std::int16_t blocked, std::int16_t index,
                     std::int16_t event1, std::int16_t event2, std::int16_t event3, std::int16_t currTex,
                     std::int16_t endTex, std::int16_t begTex, std::int16_t texDelay, std::int16_t x,
                     std::int16_t y);
    void tutorialTalk();

protected:
    bool currEventPaused_ = false;
    std::int16_t currEventId_ = -1;
    size_t currEventIndex_ = 0, currEventSize_ = 0;
    const std::vector<std::int16_t> *currEventList_ = nullptr;

    std::int16_t subMapId_ = -1;
};

}
