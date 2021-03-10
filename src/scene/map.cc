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

#include "window.hh"
#include "data/colorpalette.hh"
#include "core/config.hh"

#include <chrono>
#include <cmath>

namespace hojy::scene {

Map::Map(Renderer *renderer, int x, int y, int width, int height, float scale): Node(renderer, x, y, width, height),
    scale_(scale), auxWidth_(std::lround(float(width_) / scale)), auxHeight_(std::lround(float(height_) / scale)),
    drawDirty_(true), drawingTerrainTex_(Texture::createAsTarget(renderer_, width, height)) {
    textureMgr_.clear();
    textureMgr_.setRenderer(renderer_);
    textureMgr_.setPalette(data::gNormalPalette);
    drawingTerrainTex_->enableBlendMode(true);
}

Map::~Map() {
    delete drawingTerrainTex_;
}

void Map::setDirection(Map::Direction dir) {
    if (direction_ == dir) { return; }
    direction_ = dir;
    resetTime();
    currFrame_ = 0;
    updateMainCharTexture();
}

void Map::setPosition(int x, int y, bool checkEvent) {
    currX_ = x;
    currY_ = y;
    currFrame_ = 0;
    resting_ = false;
    drawDirty_ = true;
    bool r = tryMove(x, y, checkEvent);
    resetTime();
    if (r) {
        updateMainCharTexture();
    }
}

void Map::move(Map::Direction direction) {
    int x, y;
    direction_ = direction;
    if (!getFaceOffset(x, y) || !tryMove(x, y, true)) {
        return;
    }
    resetTime();
    updateMainCharTexture();
}

void Map::resetFrame() {
    frames_ = 0;
    nextFrameTime_ = gWindow->currTime();
    resetTime();
}

void Map::render() {
    if (checkTime()) {
        updateMainCharTexture();
    }
    ++frames_;
    if (gWindow->currTime() < nextFrameTime_) { return; }
    nextFrameTime_ += std::chrono::microseconds(int(100000.f / core::config.animationSpeed()));
    frameUpdate();
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
    case KeyCancel:
        gWindow->showMainMenu(subMapId_ >= 0);
        break;
    default:
        Node::handleKeyInput(key);
        break;
    }
}

void Map::resetTime() {
    resting_ = false;
    nextRestTexTime_ = gWindow->currTime() + std::chrono::seconds(currFrame_ > 0 ? 2 : 5);
}

bool Map::checkTime() {
    auto now = gWindow->currTime();
    if (resting_) {
        if (now < nextRestTexTime_) {
            return false;
        }
        currFrame_ = (currFrame_ + 1) % 6;
        nextRestTexTime_ = now + std::chrono::milliseconds(500);
        return true;
    }
    if (now < nextRestTexTime_) {
        return false;
    }
    if (currFrame_ > 0) {
        currFrame_ = 0;
        nextRestTexTime_ = now + std::chrono::seconds(5);
    } else {
        currFrame_ = 0;
        resting_ = true;
        nextRestTexTime_ = now + std::chrono::milliseconds(500);
    }
    return true;
}

void Map::renderChar(int deltaY) {
    renderer_->renderTexture(mainCharTex_, float(x_ + (width_ >> 1)), float(y_ + (height_ >> 1) - deltaY), scale_);
}

bool Map::getFaceOffset(int &x, int &y) {
    x = currX_;
    y = currY_;
    switch (direction_) {
    case DirUp:
        if (y > 0) { --y; return true; }
        break;
    case DirRight:
        if (x < mapWidth_ - 1) { ++x; return true; }
        break;
    case DirLeft:
        if (x > 0) { --x; return true; }
        break;
    case DirDown:
        if (y < mapHeight_ - 1) { ++y; return true; }
        break;
    }
    return false;
}

}
