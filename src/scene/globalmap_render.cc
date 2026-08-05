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

#include "globalmap.hh"

#include "colorpalette.hh"

#include <algorithm>
#include <map>
#include <memory>

namespace {

enum {
    GlobalMapWidth = 480,
    GlobalMapHeight = 480,
};

}

namespace hojy::scene {

void GlobalMap::prepareMiniMapRender() {
    if (miniMapRevision_ == preparedMiniMapRevision_) { return; }
    const auto miniMapWidth = static_cast<std::int16_t>(
        2 * (GlobalMapWidth + GlobalMapHeight - 1) + 1);
    const auto miniMapHeight = static_cast<std::int16_t>(
        GlobalMapWidth + GlobalMapHeight);
    std::unique_ptr<Texture> candidate(
        Texture::create(renderer_, miniMapWidth, miniMapHeight));
    if (!candidate || !candidate->enableBlendMode(true)) { return; }
    int pitch = 0;
    TextureLock lock(candidate.get(), pitch);
    if (!lock.valid() || pitch < candidate->width()) { return; }
    const auto pixelCount = static_cast<std::size_t>(pitch)
        * static_cast<std::size_t>(candidate->height());
    std::fill_n(lock.pixels(), pixelCount, 0U);
    const auto *colors = gNormalPalette.colors();
    std::map<std::int16_t, std::uint32_t> colorMap;
    bool renderOk = true;
    const auto markPoint = [&](int x, int y, std::uint32_t color) {
        if (x < 1 || x + 1 >= candidate->width()
            || y < 1 || y >= candidate->height()) {
            return false;
        }
        const auto offset = x + y * pitch;
        auto *pixels = lock.pixels();
        pixels[offset - 1] = color;
        pixels[offset] = color;
        pixels[offset + 1] = color;
        pixels[offset - pitch] = color;
        return true;
    };
    for (const auto &cell: miniMapCells_) {
        std::uint32_t color = 0x202020U;
        if (!cell.blocked && cell.earthId >= 0
            && static_cast<std::size_t>(cell.earthId) < texData_.size()) {
            const auto ite = colorMap.find(cell.earthId);
            if (ite == colorMap.end()) {
                color = Texture::calcRLEAvgColor(
                    texData_[cell.earthId], colors);
                colorMap.emplace(cell.earthId, color);
            } else {
                color = ite->second;
            }
        }
        if (!markPoint(cell.x, cell.y, color | 0xE0000000u)) {
            renderOk = false;
            break;
        }
    }
    if (renderOk) {
        for (const auto &entry: subMapEntries_) {
            const auto ex = entry.first.first;
            const auto ey = entry.first.second;
            renderOk = markPoint(
                2 * (mapHeight_ - 1) + 1 + (ex - ey) * 2,
                1 + ex + ey, 0xE040C0C0);
            if (!renderOk) { break; }
        }
    }
    if (!renderOk) { return; }
    lock.unlock();
    if (renderer_->targetTexture() == miniMapTex_) { return; }
    auto *previous = miniMapTex_;
    miniMapTex_ = candidate.release();
    delete previous;
    preparedMiniMapRevision_ = miniMapRevision_;
}

void GlobalMap::render() const {
    Map::render();
    renderer_->clear(0, 0, 0, 255);
    renderer_->renderTexture(drawingTerrainTex_, x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
    renderChar();
    renderer_->renderTexture(drawingTerrainTex2_, x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
    for (int i = 0; i < 3; ++i) {
        const auto *c = preparedCloud_[i];
        if (!c) {
            continue;
        }
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int cloudcx = cloudStartX_[i] - cameraX_, cloudcy = cloudStartY_[i] - cameraY_;
        int cloudx = (cloudcx - cloudcy) * cellDiffX * scale_.first / scale_.second + cloudX_[i] / 2;
        int cloudy = (cloudcx + cloudcy) * cellDiffY * scale_.first / scale_.second + cloudY_[i];
        renderer_->renderTexture(c, cloudx, cloudy, scale_);
    }
    renderMiniPanel();
}

}
