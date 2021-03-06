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
#include "texture.hh"

#include <chrono>
#include <cstdint>

namespace hojy::scene {

class Texture;

class Map: public Node {
public:
    enum Direction {
        DirUp = 0,
        DirRight = 1,
        DirLeft = 2,
        DirDown = 3,
    };

public:
    Map(Renderer *renderer, int x, int y, int width, int height, float scale);
    Map(const Map&) = delete;
    ~Map() override;

    [[nodiscard]] std::int16_t subMapId() const { return subMapId_; }

    void resetFrame();

    [[nodiscard]] const TextureMgr &textureMgr() const { return textureMgr_; }

    void render() override;

protected:
    Direction calcDirection(int fx, int fy, int tx, int ty);

    virtual void resetTime() {}
    virtual void frameUpdate() {}

protected:
    TextureMgr textureMgr_;
    std::int16_t subMapId_ = -1;
    int cameraX_ = 0, cameraY_ = 0;

    std::uint64_t frames_ = 0;
    float scale_ = 1.f;
    std::uint32_t auxWidth_ = 0, auxHeight_ = 0;
    bool drawDirty_ = false;
    std::chrono::steady_clock::time_point nextFrameTime_;
    std::chrono::steady_clock::duration eachFrameTime_;
    std::int32_t mapWidth_ = 0, mapHeight_ = 0, cellWidth_ = 0, cellHeight_ = 0;
    std::int32_t texWidth_ = 0, texHeight_ = 0;
    std::int32_t offsetX_ = 0, offsetY_ = 0;
    Texture *drawingTerrainTex_ = nullptr;
};

}
