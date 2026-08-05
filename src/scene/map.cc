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
#include <fmt/xchar.h>

#include <cmath>
#include <limits>
#include <utility>

namespace {

std::uint32_t scaledExtent(int extent, std::pair<int, int> scale) {
    if (extent <= 0 || scale.first <= 0 || scale.second <= 0) { return 0; }
    const auto value = static_cast<std::int64_t>(extent) * scale.second / scale.first;
    if (value <= 0 || value > std::numeric_limits<std::int16_t>::max()) { return 0; }
    return static_cast<std::uint32_t>(value);
}

std::uint64_t frameInterval() {
    const auto speed = hojy::core::config.animationSpeed();
    if (speed <= 0.f) { return 0; }
    return static_cast<std::uint64_t>(std::round(1000000.f / 15.f / speed));
}

}

namespace hojy::scene {

Map::Map(Renderer *renderer, int x, int y, int width, int height, std::pair<int, int> scale): Node(renderer, x, y, width, height),
    scale_(scale), auxWidth_(scaledExtent(width, scale)), auxHeight_(scaledExtent(height, scale)),
    drawingTerrainTex_(Texture::create(renderer_, auxWidth_, auxHeight_)),
    miniPanelTex_(Texture::createAsTarget(renderer_, 256, 256)),
    eachFrameTime_(frameInterval()) {
    textureMgr_.clear();
    textureMgr_.setRenderer(renderer_);
    textureMgr_.setPalette(gNormalPalette);
    if (drawingTerrainTex_) { (void)drawingTerrainTex_->enableBlendMode(true); }
    if (miniPanelTex_) { (void)miniPanelTex_->enableBlendMode(true); }

    resourcesReady_ = renderer_ != nullptr && scale_.first > 0 && scale_.second > 0
        && auxWidth_ > 0 && auxHeight_ > 0
        && drawingTerrainTex_ != nullptr && drawingTerrainTex_->valid()
        && miniPanelTex_ != nullptr && miniPanelTex_->valid();
    if (!resourcesReady_) { return; }

    auto windowBorder = core::config.windowBorder();
    miniMapW_ = width_ / 4 - windowBorder;
    miniMapH_ = height_ / 4 - windowBorder;
    miniMapX_ = x_ + width_ - miniMapW_ - windowBorder + 1;
    miniMapY_ = y_ + windowBorder;
    miniMapAuxW_ = miniMapW_ * scale_.second / scale_.first;
    miniMapAuxH_ = miniMapH_ * scale_.second / scale_.first;
}

Map::~Map() {
    delete miniMapTex_;
    delete drawingTerrainTex_;
    delete miniPanelTex_;
}

const std::string &Map::texData(std::int16_t id) const {
    if (id < 0 || id >= texData_.size()) {
        static const std::string dummy;
        return dummy;
    }
    return texData_[id];
}

void Map::resetFrame() {
    frames_ = 0;
    nextFrameTime_ = phaseTime();
    resetTime();
}

void Map::advanceCompatibilityFrame() {
    ++frames_;
    frameUpdate();
}

void Map::prepareRender() {
    if (!resourcesReady_) { return; }
    prepareMiniPanel();
}

void Map::render() const {
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

void Map::markWorldChanged() noexcept {
    ++worldRevision_;
    if (worldRevision_ == 0) {
        worldRevision_ = 1;
        preparedWorldRevision_ = 0;
    }
}

void Map::markMiniPanelChanged() noexcept {
    miniPanelSnapshot_.x = currX_;
    miniPanelSnapshot_.y = currY_;
    ++miniPanelRevision_;
    if (miniPanelRevision_ == 0) {
        miniPanelRevision_ = 1;
        preparedMiniPanelRevision_ = 0;
    }
}

void Map::commitMiniPanelSnapshot(std::wstring mapName,
                                  std::int32_t x,
                                  std::int32_t y) {
    miniPanelSnapshot_.mapName = std::move(mapName);
    miniPanelSnapshot_.x = x;
    miniPanelSnapshot_.y = y;
    ++miniPanelRevision_;
    if (miniPanelRevision_ == 0) {
        miniPanelRevision_ = 1;
        preparedMiniPanelRevision_ = 0;
    }
}

bool Map::worldPresentationNeedsPrepare() const noexcept {
    return preparedWorldRevision_ != worldRevision_;
}

void Map::commitWorldPresentation() noexcept {
    preparedWorldRevision_ = worldRevision_;
}

void Map::prepareMiniPanel() {
    if (!core::config.showMapMiniPanel()
        || preparedMiniPanelRevision_ == miniPanelRevision_) {
        return;
    }
    std::unique_ptr<Texture> candidate(Texture::createAsTarget(renderer_, 256, 256));
    if (!candidate || !candidate->enableBlendMode(true)) {
        candidate.reset();
        return;
    }
    int w = 0;
    int h = 0;
    try {
        RenderTargetGuard target(renderer_, candidate.get());
        if (!target.valid()) { return; }
        renderer_->clear(0, 0, 0, 0);
        auto *ttf = renderer_->ttf();
        int smallFontSize = std::max(8, (ttf->fontSize() * 2 / 3 + 1) & ~1);
        auto lineheight = smallFontSize + TextLineSpacing;
        auto windowBorder = core::config.windowBorder() * 2 / 3;
        h = windowBorder * 2 + lineheight - TextLineSpacing;
        int w0 = 0, w1;
        const std::wstring *name = nullptr;
        if (!miniPanelSnapshot_.mapName.empty()) {
            name = &miniPanelSnapshot_.mapName;
            h += lineheight;
            if (!ttf->prepareText(*name, smallFontSize)) { return; }
            w0 = ttf->preparedStringWidth(*name, smallFontSize);
        }
        std::wstring coordStr = fmt::format(
            L"({},{})", miniPanelSnapshot_.x, miniPanelSnapshot_.y);
        if (!ttf->prepareText(coordStr, smallFontSize)) { return; }
        w1 = ttf->preparedStringWidth(coordStr, smallFontSize);
        w = std::max(w0, w1) + windowBorder * 2;
        renderer_->fillRoundedRect(0, 0, w, h, windowBorder, 64, 64, 64, 208);
        renderer_->drawRoundedRect(0, 0, w, h, windowBorder, 208, 208, 208, 224);
        ttf->setColor(192, 192, 192);
        int y = windowBorder;
        if (name) {
            ttf->renderPrepared(*name, (w - w0) / 2, y, false, smallFontSize);
            y += lineheight;
        }
        ttf->renderPrepared(coordStr, (w - w1) / 2, y, false, smallFontSize);
    } catch (...) {
        throw;
    }
    delete miniPanelTex_;
    miniPanelTex_ = candidate.release();
    miniPanelX_ = width_ - w - core::config.windowBorder();
    miniPanelY_ = core::config.windowBorder();
    miniPanelReady_ = true;
    preparedMiniPanelRevision_ = miniPanelRevision_;
}

void Map::renderMiniPanel() const {
    auto minimap = core::config.showMinimap() && miniMapTex_ != nullptr;
    if (minimap) {
        renderer_->drawRoundedRect(miniMapX_ - 1, miniMapY_ - 1, miniMapW_ + 2, miniMapH_ + 2, 2, 208, 208, 208, 224);
        renderer_->renderTexture(miniMapTex_, miniMapX_, miniMapY_, miniMapW_, miniMapH_, miniMapAuxX_, miniMapAuxY_, miniMapAuxW_, miniMapAuxH_, true);
        auto rad = std::max(1, scale_.first / scale_.second);
        if (rad > 1) {
            renderer_->fillCircle(miniMapX_ + miniMapW_ / 2, miniMapY_ + miniMapH_ / 2, rad + 1, 252, 32, 32, 224);
        } else {
            renderer_->drawCircle(miniMapX_ + miniMapW_ / 2, miniMapY_ + miniMapH_ / 2, rad, 252, 32, 32, 224);
        }
    }
    if (core::config.showMapMiniPanel() && miniPanelReady_ && miniPanelTex_) {
        renderer_->renderTexture(miniPanelTex_, miniPanelX_,
                                 minimap ? (miniPanelY_ + core::config.windowBorder() + miniMapH_) : miniPanelY_, true);
    }
}

const Texture *Map::getOrLoadTexture(std::int16_t id) {
    if (!resourcesReady_ || id < 0 || static_cast<std::size_t>(id) >= texData_.size()) {
        return nullptr;
    }
    const auto *tex = textureMgr_[id];
    if (tex) { return tex; }
    return textureMgr_.loadFromRLE(texData_[id], id);
}

}
