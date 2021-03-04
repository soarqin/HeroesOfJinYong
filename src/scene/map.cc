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

#include "data/colorpalette.hh"

#include <chrono>

namespace hojy::scene {

Map::Map(Renderer *renderer, int x, int y, int width, int height, float scale, std::int16_t id): Node(renderer, x, y, width, height), subMapId_(id), scale_(scale) {
    auxWidth_ = std::uint32_t(width_ / scale + 0.5);
    auxHeight_ = std::uint32_t(height_ / scale + 0.5);
    textureMgr.clear();
    textureMgr.setRenderer(renderer_);
    textureMgr.setPalette(data::gNormalPalette);
    drawingTerrainTex_ = Texture::createAsTarget(renderer_, 2048, 2048);
    drawingTerrainTex_->enableBlendMode(true);
    drawDirty_ = true;
}

Map::~Map() {
    delete drawingTerrainTex_;
}

void Map::render() {
    checkTime();
}

void Map::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeyUp:
        move(Map::DirUp);
        break;
    case KeyRight:
        move(Map::DirRight);
        break;
    case KeyLeft:
        move(Map::DirLeft);
        break;
    case KeyDown:
        move(Map::DirDown);
        break;
    default:
        Node::handleKeyInput(key);
        break;
    }
}

void Map::setPosition(int x, int y) {
    currX_ = x;
    currY_ = y;
    currFrame_ = 0;
    resting_ = false;
    drawDirty_ = true;
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

void Map::renderChar(int deltaY) {
    renderer_->renderTexture(mainCharTex_, float(x_ + int(width_) / 2), float(y_ + int(height_) / 2 - deltaY), scale_);
}

void Map::getFaceOffset(int &x, int &y) {
    x = currX_;
    y = currY_;
    switch (direction_) {
    case DirUp:
        y -= 1;
        break;
    case DirDown:
        y += 1;
        break;
    case DirLeft:
        x -= 1;
        break;
    case DirRight:
        x += 1;
        break;
    }
}

}
