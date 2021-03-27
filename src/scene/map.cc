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
#include "colorpalette.hh"
#include "core/config.hh"

#include <chrono>

namespace hojy::scene {

Map::Map(Renderer *renderer, int x, int y, int width, int height, std::pair<int, int> scale): Node(renderer, x, y, width, height),
    scale_(scale), auxWidth_(width_ * scale.second / scale.first), auxHeight_(height_ * scale.second / scale.first),
    drawDirty_(true), drawingTerrainTex_(Texture::createAsTarget(renderer_, width, height)),
    eachFrameTime_(std::chrono::microseconds(int(1000000.f / 15.f / core::config.animationSpeed()))) {
    textureMgr_.clear();
    textureMgr_.setRenderer(renderer_);
    textureMgr_.setPalette(gNormalPalette);
    drawingTerrainTex_->enableBlendMode(true);
}

Map::~Map() {
    delete drawingTerrainTex_;
}

void Map::resetFrame() {
    frames_ = 0;
    nextFrameTime_ = gWindow->currTime();
    resetTime();
}

void Map::render() {
    ++frames_;
    auto now = gWindow->currTime();
    if (now >= nextFrameTime_) {
        nextFrameTime_ += eachFrameTime_;
        if (nextFrameTime_ < now) { nextFrameTime_ = now; }
        frameUpdate();
    }
}

Map::Direction Map::calcDirection(int fx, int fy, int tx, int ty) {
    (void)this;
    int dx = tx - fx, dy = ty - fy;
    if (std::abs(dx) > std::abs(dy)) {
        if (dx < 0) { return Map::DirLeft; }
        return Map::DirRight;
    }
    if (dy < 0) { return Map::DirUp; }
    return Map::DirDown;
}

}
