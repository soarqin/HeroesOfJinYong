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

#include <vector>
#include <set>

namespace hojy::scene {

class WarField: public Map {
    struct CellInfo {
        const Texture *earth = nullptr, *building = nullptr, *charTex = nullptr;
        std::int16_t charId = -1;
        bool isWater = false;
    };
    struct CharInfo {
        std::uint32_t side; /* bit0: 0-self 1-enemy
                            * bit1: auto-battle
                            */
        std::int16_t id;
        std::int16_t x, y;
        Direction direction;
        std::int16_t speed;
        std::int16_t hp, mp;
        std::int16_t stamina;
        std::uint16_t exp;
    };
public:
    WarField(Renderer *renderer, int x, int y, int width, int height, float scale);
    ~WarField() override;

    bool load(std::int16_t warId);
    void setGetExpOnLose(bool b) { getExpOnLose_ = b; }
    bool getDefaultChars(std::set<std::int16_t> &chars) const;
    void putChars(const std::vector<std::int16_t> &chars);

    void render() override;
    void handleKeyInput(Key key) override;

protected:
    bool tryMove(int x, int y, bool checkEvent) override;
    void updateMainCharTexture() override;

private:
    std::int16_t warId_ = -1;
    bool getExpOnLose_ = false;
    std::vector<CellInfo> cellInfo_;
    Texture *drawingTerrainTex2_ = nullptr;
    std::set<std::int16_t> warMapLoaded_;

    std::vector<CharInfo> charQueue_;
    size_t activeChar_ = 0;
};

}
