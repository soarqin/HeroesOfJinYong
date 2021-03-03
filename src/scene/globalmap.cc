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

#include "texture.hh"
#include "data/grpdata.hh"
#include "util/file.hh"
#include "core/config.hh"

#include <algorithm>

namespace hojy::scene {

enum {
    GlobalMapWidth = 480,
    GlobalMapHeight = 480,
};

GlobalMap::GlobalMap(Renderer *renderer, std::uint32_t width, std::uint32_t height): Map(renderer, width, height) {
    mapWidth_ = GlobalMapWidth;
    mapHeight_ = GlobalMapHeight;
    auto &mmapData = data::grpData.lazyLoad("MMAP");
    auto sz = mmapData.size();
    for (size_t i = 0; i < sz; ++i) {
        textureMgr.loadFromRLE(i, mmapData[i]);
    }
    {
        auto *tex = textureMgr[0];
        cellWidth_ = tex->width();
        cellHeight_ = tex->height();
        offsetX_ = tex->originX();
        offsetY_ = tex->originY();
        deepWaterTex_ = tex;
    }
    int cellDiffX = cellWidth_ / 2;
    int cellDiffY = cellHeight_ / 2;
    texWidth_ = (mapWidth_ + mapHeight_) * cellDiffX;
    texHeight_ = (mapWidth_ + mapHeight_) * cellDiffY;

    auto size = mapWidth_ * mapHeight_;
    util::File::getFileContent(core::config.dataFilePath("EARTH.002"), earth_);
    util::File::getFileContent(core::config.dataFilePath("SURFACE.002"), surface_);
    util::File::getFileContent(core::config.dataFilePath("BUILDING.002"), building_);
    util::File::getFileContent(core::config.dataFilePath("BUILDX.002"), buildx_);
    util::File::getFileContent(core::config.dataFilePath("BUILDY.002"), buildy_);
    earth_.resize(size);
    surface_.resize(size);
    building_.resize(size);
    buildx_.resize(size);
    buildy_.resize(size);
    cellInfo_.resize(size);

    int x = (mapHeight_ - 1) * cellDiffX + offsetX_;
    int y = offsetY_;
    int pos = 0;
    for (int j = mapHeight_; j; --j) {
        int tx = x, ty = y;
        for (int i = mapWidth_; i; --i, ++pos, tx += cellDiffX, ty += cellDiffY) {
            auto &ci = cellInfo_[pos];
            auto &n = earth_[pos];
            n >>= 1;
            if (n) {
                if (n == 419 || n >= 306 && n <= 335) {
                    ci.type = 1;
                } else if (n >= 179 && n <= 181 || n >= 253 && n <= 335 || n >= 508 && n <= 511) {
                    ci.type = 1;
                    ci.canWalk = true;
                } else if (n > 0) {
                    ci.canWalk = true;
                }
            }
            ci.earth = textureMgr[n];
            auto &n0 = surface_[pos];
            n0 >>= 1;
            if (n0) {
                ci.surface = textureMgr[n0];
            } else {
                ci.surface = nullptr;
            }
            auto &n1 = building_[pos];
            n1 >>= 1;
            if (n1 > 0) {
                ci.canWalk = false;
                if (n1 >= 1008 && n1 <= 1164 || n1 >= 1214 && n1 <= 1238) {
                    ci.type = 2;
                }
                const auto *tex2 = textureMgr[n1];
                if (tex2) {
                    auto centerX = tx - tex2->originX() + tex2->width() / 2;
                    auto centerY =
                        ty - tex2->originY() + (tex2->height() < 36 ? tex2->height() * 4 / 5 : tex2->height() * 3 / 4);
                    buildingTex_.emplace_back(BuildingTex{centerY * texWidth_ + centerX, tx, ty, tex2});
                }
            }
        }
        x -= cellDiffX; y += cellDiffY;
    }

    std::sort(buildingTex_.begin(), buildingTex_.end(), BuildingTexComp());
    drawingBuildingTex_[0] = Texture::createAsTarget(renderer_, 2048, 1024);
    drawingBuildingTex_[0]->enableBlendMode(true);
    drawingBuildingTex_[1] = Texture::createAsTarget(renderer_, 2048, 1024);
    drawingBuildingTex_[1]->enableBlendMode(true);
    currX_ = 242, currY_ = 294;
    resetTime();
    updateMainCharTexture();
}

GlobalMap::~GlobalMap() {
    delete drawingBuildingTex_[0];
    delete drawingBuildingTex_[1];
}

void GlobalMap::render() {
    Map::render();
    if (moveDirty_) {
        moveDirty_ = false;
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int curX = currX_, curY = currY_;
        int nx = int(width_) / 2 + cellWidth_;
        int ny = int(height_) / 2 + cellHeight_;
        int cx = (nx / cellDiffX + ny / cellDiffY) / 2;
        int cy = (ny / cellDiffY - nx / cellDiffX) / 2;
        int wcount = nx * 2 / cellWidth_;
        int hcount = ny * 2 / cellDiffY;
        int tx = int(width_) / 2 - (cx - cy) * cellDiffX;
        int ty = int(height_) / 2 - (cx + cy) * cellDiffY;
        cx = curX - cx; cy = curY - cy;
        renderer_->setTargetTexture(drawingTerrainTex_);
        renderer_->setClipRect(0, 0, 2048, 2048);
        renderer_->fill(0, 0, 0, 0);
        int delta = -mapWidth_ + 1;
        for (int j = hcount; j; --j) {
            int x = cx, y = cy;
            int dx = tx;
            int offset = y * mapWidth_ + x;
            for (int i = wcount; i; --i, dx += cellWidth_, offset += delta, ++x, --y) {
                if (x < 0 || x >= GlobalMapWidth || y < 0 || y >= GlobalMapHeight) {
                    continue;
                }
                if (x >= 0 && y >= 0) {
                    auto &ci = cellInfo_[offset];
                    renderer_->renderTexture(ci.earth, dx, ty);
                    if (ci.surface) {
                        renderer_->renderTexture(ci.surface, dx, ty);
                    }
                } else {
                    renderer_->renderTexture(deepWaterTex_, dx, ty);
                }
            }
            if (j % 2) {
                ++cx;
                tx += cellDiffX;
                ty += cellDiffY;
            } else {
                ++cy;
                tx -= cellDiffX;
                ty += cellDiffY;
            }
        }

        int ox = (mapHeight_ + curX - curY - 1) * cellDiffX + offsetX_ - int(width_ / 2);
        int oy = (curX + curY) * cellDiffY + offsetY_ - int(height_ / 2);
        int myy = int(height_) / 2 + oy;
        int l = ox - cellWidth_ * 2, t = oy - cellHeight_ * 2, r = ox + int(width_) + cellWidth_ * 2, b = oy + int(height_) + cellHeight_ * 6;
        auto ite = std::lower_bound(buildingTex_.begin(), buildingTex_.end(), BuildingTex {t * texWidth_ + l, 0, 0, nullptr}, BuildingTexComp());
        auto ite_mid = std::upper_bound(buildingTex_.begin(), buildingTex_.end(), BuildingTex {myy * texWidth_, 0, 0, nullptr}, BuildingTexComp());
        auto ite_end = std::upper_bound(buildingTex_.begin(), buildingTex_.end(), BuildingTex {b * texWidth_ + r, 0, 0, nullptr}, BuildingTexComp());
        renderer_->setTargetTexture(drawingBuildingTex_[0]);
        renderer_->setClipRect(0, 0, 2048, 1024);
        renderer_->fill(0, 0, 0, 0);
        while (ite != ite_mid) {
            if (ite->x < l || ite->x >= r) {
                ++ite;
                continue;
            }
            renderer_->renderTexture(ite->tex, ite->x - ox, ite->y - oy);
            ++ite;
        }
        renderer_->setTargetTexture(drawingBuildingTex_[1]);
        renderer_->setClipRect(0, 0, 2048, 1024);
        renderer_->fill(0, 0, 0, 0);
        while (ite != ite_end) {
            if (ite->x < l || ite->x >= r) {
                ++ite;
                continue;
            }
            renderer_->renderTexture(ite->tex, ite->x - ox, ite->y - oy);
            ++ite;
        }
        renderer_->setTargetTexture(nullptr);
        renderer_->unsetClipRect();
    }
    renderer_->fill(0, 0, 0, 0);
    renderer_->renderTexture(drawingTerrainTex_, 0, 0, width_, height_);
    renderer_->renderTexture(drawingBuildingTex_[0], 0, 0, width_, height_);
    renderChar();
    renderer_->renderTexture(drawingBuildingTex_[1], 0, 0, width_, height_);
}

bool GlobalMap::tryMove(int x, int y) {
    auto offset = y * mapWidth_ + x;
    if (!cellInfo_[offset].canWalk || buildx_[offset] != 0 && building_[buildy_[offset] * mapWidth_ + buildx_[offset]] != 0) {
        return true;
    }
    currX_ = x;
    currY_ = y;
    moveDirty_ = true;
    onShip_ = cellInfo_[offset].type == 1;
    if (onShip_) {
        currFrame_ = (currFrame_ + 1) % 4;
    } else {
        currFrame_ = currFrame_ % 6 + 1;
    }
    return true;
}

void GlobalMap::updateMainCharTexture() {
    if (onShip_) {
        mainCharTex_ = textureMgr[3715 + int(direction_) * 4 + currFrame_];
        return;
    }
    if (resting_) {
        mainCharTex_ = textureMgr[2529 + int(direction_) * 6 + currFrame_];
        return;
    }
    mainCharTex_ = textureMgr[2501 + int(direction_) * 7 + currFrame_];
}

void GlobalMap::resetTime() {
    if (onShip_) { return; }
    Map::resetTime();
}

void GlobalMap::checkTime() {
    if (onShip_) { return; }
    Map::checkTime();
}

}
