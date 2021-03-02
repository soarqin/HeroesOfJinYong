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

#include "map.hh"

#include "texture.hh"
#include "data/colorpalette.hh"
#include <chrono>

namespace hojy::scene {

Map::Map(Renderer *renderer, std::uint32_t width, std::uint32_t height): Node(renderer, width, height) {
    mapTextureMgr.setRenderer(renderer_);
    mapTextureMgr.setPalette(data::normalPalette.colors(), data::normalPalette.size());
    drawingTerrainTex_ = Texture::createAsTarget(renderer_, 2048, 2048);
    drawingTerrainTex_->enableBlendMode(true);
    moveDirty_ = true;
}

Map::~Map() {
    delete drawingTerrainTex_;
}

void Map::render() {
    checkTime();
}

void Map::setPosition(int x, int y) {
    currX_ = x;
    currY_ = y;
    currFrame_ = 0;
    resting_ = false;
    moveDirty_ = true;
    auto offset = y * mapWidth_ + x;
    tryMove(x, y);
    resetTime();
    updateMainCharTexture();
}

void Map::move(Map::Direction direction) {
    int x = currX_, y = currY_;
    direction_ = direction;
    switch (direction) {
    case DirUp:
        if (y > 0) { --y; }
        break;
    case DirRight:
        if (x < mapWidth_ - 1) { ++x; }
        break;
    case DirLeft:
        if (x > 0) { --x; }
        break;
    case DirDown:
        if (y < mapHeight_ - 1) { ++y; }
        break;
    }
    if (!tryMove(x, y)) {
        return;
    }
    resetTime();
    updateMainCharTexture();
}

void Map::resetTime() {
    resting_ = false;
    nextTime_ = std::chrono::steady_clock::now() + std::chrono::seconds(currFrame_ > 0 ? 2 : 5);
}

void Map::checkTime() {
    if (resting_) {
        if (std::chrono::steady_clock::now() < nextTime_) {
            return;
        }
        currFrame_ = (currFrame_ + 1) % 6;
        nextTime_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
        updateMainCharTexture();
        return;
    }
    if (std::chrono::steady_clock::now() < nextTime_) {
        return;
    }
    if (currFrame_ > 0) {
        currFrame_ = 0;
        nextTime_ = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    } else {
        currFrame_ = 0;
        resting_ = true;
        nextTime_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    }
    updateMainCharTexture();
}

void Map::renderChar() {
    renderer_->renderTexture(mainCharTex_, int(width_) / 2, int(height_) / 2);
}

}
