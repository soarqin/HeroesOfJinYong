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
#include "mem/strings.hh"
#include "core/config.hh"
#include <fmt/format.h>
#include <chrono>

namespace hojy::scene {

Map::Map(Renderer *renderer, int x, int y, int width, int height, std::pair<int, int> scale): Node(renderer, x, y, width, height),
    scale_(scale), auxWidth_(width_ * scale.second / scale.first), auxHeight_(height_ * scale.second / scale.first),
    drawDirty_(true), drawingTerrainTex_(Texture::create(renderer_, width, height)),
    miniPanelTex_(Texture::createAsTarget(renderer_, 256, 256)),
    eachFrameTime_(std::chrono::microseconds(int(1000000.f / 15.f / core::config.animationSpeed()))) {
    textureMgr_.clear();
    textureMgr_.setRenderer(renderer_);
    textureMgr_.setPalette(gNormalPalette);
    drawingTerrainTex_->enableBlendMode(true);
    miniPanelTex_->enableBlendMode(true);
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

void Map::showMiniPanel() {
    if (!core::config.showMapMiniPanel()) {
        return;
    }
    if (miniPanelDirty_) {
        miniPanelDirty_ = false;
        renderer_->setTargetTexture(miniPanelTex_);
        renderer_->clear(0, 0, 0, 0);
        auto *ttf = renderer_->ttf();
        int smallFontSize = std::max(8, (ttf->fontSize() * 2 / 3 + 1) & ~1);
        auto lineheight = smallFontSize + TextLineSpacing;
        auto windowBorder = core::config.windowBorder() * 2 / 3;
        int h = windowBorder * 2 + lineheight - TextLineSpacing;
        int w0 = 0, w1;
        const std::wstring *name = nullptr;
        if (subMapId_ >= 0) {
            name = &GETSUBMAPNAME(subMapId_);
            h += lineheight;
            w0 = ttf->stringWidth(*name, smallFontSize);
        }
        std::wstring coordStr = fmt::format(L"({},{})", currX_, currY_);
        w1 = ttf->stringWidth(coordStr, smallFontSize);
        int w = std::max(w0, w1) + windowBorder * 2;
        renderer_->fillRoundedRect(0, 0, w, h, windowBorder, 64, 64, 64, 208);
        renderer_->drawRoundedRect(0, 0, w, h, windowBorder, 224, 224, 224, 255);
        ttf->setColor(192, 192, 192);
        int y = windowBorder;
        if (name) {
            ttf->render(*name, (w - w0) / 2, y, false, smallFontSize);
            y += lineheight;
        }
        ttf->render(coordStr, (w - w1) / 2, y, false, smallFontSize);
        renderer_->setTargetTexture(nullptr);
        miniPanelX_ = width_ - w - windowBorder;
        miniPanelY_ = windowBorder;
    }
    renderer_->renderTexture(miniPanelTex_, miniPanelX_, miniPanelY_, true);
}

const Texture *Map::getOrLoadTexture(std::int16_t id) {
    const auto *tex = textureMgr_[id];
    if (tex) { return tex; }
    return textureMgr_.loadFromRLE(texData_[id], id);
}

}
