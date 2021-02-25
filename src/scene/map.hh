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

#include "node.hh"

#include <chrono>
#include <cstdint>

namespace hojy::scene {

class Texture;

class Map final: public Node {
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
        std::int32_t x, y;
        bool canWalk;
        /* 0-land 1-water 2-wood */
        std::uint8_t type;
    };

public:
    enum Direction {
        DirUp = 0,
        DirRight = 1,
        DirLeft = 2,
        DirDown = 3,
    };

public:
    explicit Map(Renderer *renderer, std::uint32_t width, std::uint32_t height);
    Map(const Map&) = delete;
    ~Map();

    void setPosition(int x, int y);
    void move(Direction direction);

    void render() override;

private:
    void updateMainCharTexture();
    void resetTime();
    void checkTime();

private:
    std::int32_t currX_ = 0, currY_ = 0, currFrame_ = 0;
    Direction direction_ = DirUp;
    bool moveDirty_ = false, resting_ = false, onShip_ = false;
    const Texture *mainCharTex_ = nullptr, *deepWaterTex_ = nullptr;
    std::chrono::steady_clock::time_point nextTime_;
    std::vector<CellInfo> cellInfo_;
    std::int32_t mapWidth_ = 0, mapHeight_ = 0, cellWidth_ = 0, cellHeight_ = 0;
    std::int32_t texWidth_ = 0, texHeight_ = 0;
    std::int32_t offsetX_ = 0, offsetY_ = 0;
    std::vector<std::uint16_t> earth_, surface_, building_, buildx_, buildy_;
    std::vector<BuildingTex> buildingTex_;
    Texture *drawingTerrainTex_ = nullptr;
    Texture *drawingBuildingTex_[2] = {nullptr, nullptr};
};

}
