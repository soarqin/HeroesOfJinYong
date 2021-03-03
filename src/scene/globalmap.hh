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

class GlobalMap final: public Map {
    struct BuildingTex {
        std::int32_t order;
        std::int32_t x, y;
        const Texture *tex;
    };
    struct BuildingTexComp {
        bool operator()(const BuildingTex &a, const BuildingTex &b) const {
            return a.order < b.order;
        }
    };
    struct CellInfo {
        const Texture *earth, *surface;
        bool canWalk;
        /* 0-land 1-water 2-wood */
        std::uint8_t type;
    };
public:
    GlobalMap(Renderer *renderer, std::uint32_t width, std::uint32_t height, float scale);
    ~GlobalMap() override;

    void render() override;

protected:
    bool tryMove(int x, int y) override;
    void updateMainCharTexture() override;
    void resetTime() override;
    void checkTime() override;

private:
    bool onShip_ = false;
    const Texture *deepWaterTex_ = nullptr;
    std::vector<std::uint16_t> earth_, surface_, building_, buildx_, buildy_;
    std::vector<CellInfo> cellInfo_;
    std::vector<BuildingTex> buildingTex_;
    Texture *drawingBuildingTex_[2] = {nullptr, nullptr};
};

}
