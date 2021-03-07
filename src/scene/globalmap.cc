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

#include "window.hh"
#include "data/grpdata.hh"
#include "mem/savedata.hh"
#include "util/file.hh"
#include "core/config.hh"

#include <algorithm>

namespace hojy::scene {

enum {
    GlobalMapWidth = 480,
    GlobalMapHeight = 480,
};

GlobalMap::GlobalMap(Renderer *renderer, int ix, int iy, int width, int height, float scale): MapWithEvent(renderer, ix, iy, width, height, scale) {
    mapWidth_ = GlobalMapWidth;
    mapHeight_ = GlobalMapHeight;
    auto &mmapData = data::gGrpData.lazyLoad("MMAP");
    textureMgr.loadFromRLE(mmapData);
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
    drawingBuildingTex_[0] = Texture::createAsTarget(renderer_, width_, height_);
    drawingBuildingTex_[0]->enableBlendMode(true);
    drawingBuildingTex_[1] = Texture::createAsTarget(renderer_, width_, height_);
    drawingBuildingTex_[1]->enableBlendMode(true);
    resetTime();
    updateMainCharTexture();
}

GlobalMap::~GlobalMap() {
    delete drawingBuildingTex_[0];
    delete drawingBuildingTex_[1];
}

void GlobalMap::render() {
    MapWithEvent::render();
    if (drawDirty_) {
        drawDirty_ = false;
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int curX = currX_, curY = currY_;
        int nx = int(auxWidth_) / 2 + int(cellWidth_ * scale_);
        int ny = int(auxHeight_) / 2 + int(cellHeight_ * scale_);
        int cx = (nx / cellDiffX + ny / cellDiffY) / 2;
        int cy = (ny / cellDiffY - nx / cellDiffX) / 2;
        int wcount = nx * 2 / cellWidth_;
        int hcount = (ny * 2 + int(2 * cellHeight_ * scale_)) / cellDiffY;
        int tx = int(auxWidth_) / 2 - (cx - cy) * cellDiffX;
        int ty = int(auxHeight_) / 2 - (cx + cy) * cellDiffY;
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

        int ox = (mapHeight_ + curX - curY - 1) * cellDiffX + offsetX_ - int(auxWidth_ / 2);
        int oy = (curX + curY) * cellDiffY + offsetY_ - int(auxHeight_ / 2);
        int myy = int(auxHeight_) / 2 + oy;
        int l = ox - cellWidth_ * 2, t = oy - cellHeight_ * 2, r = ox + int(auxWidth_) + cellWidth_ * 2, b = oy + int(auxHeight_) + cellHeight_ * 6;
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
    renderer_->renderTexture(drawingTerrainTex_, x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
    renderer_->renderTexture(drawingBuildingTex_[0], x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
    renderChar();
    renderer_->renderTexture(drawingBuildingTex_[1], x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
}

bool GlobalMap::tryMove(int x, int y) {
    if (subMapEntries_.empty()) {
        auto subMapSz = mem::gSaveData.subMapInfo.size();
        for (size_t i = 0; i < subMapSz; ++i) {
            const auto &smi = mem::gSaveData.subMapInfo[i];
            auto ex = smi->globalEnterX1;
            auto ey = smi->globalEnterY1;
            subMapEntries_[std::make_pair(ex, ey)] = i;
            ex = smi->globalEnterX2;
            ey = smi->globalEnterY2;
            subMapEntries_[std::make_pair(ex, ey)] = i;
        }
    }
    auto ite = subMapEntries_.find(std::make_pair(std::int16_t(x), std::int16_t(y)));
    if (ite != subMapEntries_.end()) {
        auto *smi = mem::gSaveData.subMapInfo[ite->second];
        if (smi->enterCondition == 1) {
            return true;
        }
        /* TODO: check speed */
        if (smi->enterCondition == 2) {
            return true;
        }
        gWindow->enterSubMap(ite->second, int(direction_));
        auto music = smi->enterMusic;
        if (music >= 0) {
            gWindow->playMusic(music);
        }
        return true;
    }
    auto offset = y * mapWidth_ + x;
    if (!cellInfo_[offset].canWalk || buildx_[offset] != 0 && building_[buildy_[offset] * mapWidth_ + buildx_[offset]] != 0) {
        return true;
    }
    currX_ = x;
    currY_ = y;
    drawDirty_ = true;
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

bool GlobalMap::checkTime() {
    if (onShip_) { return false; }
    return MapWithEvent::checkTime();
}

}
