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
    Map(Renderer *renderer, int x, int y, int width, int height, std::pair<int, int> scale);
    Map(const Map&) = delete;
    ~Map() override;

    [[nodiscard]] std::int16_t subMapId() const { return subMapId_; }
    [[nodiscard]] const std::string &texData(std::int16_t id) const;
    [[nodiscard]] const Texture *getOrLoadTexture(std::int16_t id);

    void resetFrame();

    [[nodiscard]] const TextureMgr &textureMgr() const { return textureMgr_; }

    void render() override;

protected:
    Direction calcDirection(int fx, int fy, int tx, int ty);
    void showMiniPanel();

    virtual void resetTime() {}
    virtual void frameUpdate() {}

protected:
    TextureMgr textureMgr_;
    std::int16_t subMapId_ = -1;
    int cameraX_ = 0, cameraY_ = 0;

    std::uint64_t frames_ = 0;
    std::pair<int, int> scale_ = {1, 1};
    std::uint32_t auxWidth_ = 0, auxHeight_ = 0;
    std::int32_t currX_ = 0, currY_ = 0;
    std::int32_t miniMapX_ = 0, miniMapY_ = 0, miniMapW_ = 0, miniMapH_ = 0;
    std::int32_t miniMapAuxX_ = 0, miniMapAuxY_ = 0, miniMapAuxW_ = 0, miniMapAuxH_ = 0;
    bool drawDirty_ = false, miniPanelDirty_ = true;
    std::chrono::steady_clock::time_point nextFrameTime_;
    std::chrono::steady_clock::duration eachFrameTime_;
    std::int32_t mapWidth_ = 0, mapHeight_ = 0, cellWidth_ = 0, cellHeight_ = 0;
    std::int32_t offsetX_ = 0, offsetY_ = 0;
    std::vector<std::string> texData_;
    Texture *drawingTerrainTex_ = nullptr;
    Texture *miniMapTex_ = nullptr;
    Texture *miniPanelTex_ = nullptr;
    std::int32_t miniPanelX_ = 0, miniPanelY_ = 0;
};

}
