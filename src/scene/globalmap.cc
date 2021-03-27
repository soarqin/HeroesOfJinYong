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
#include "window.hh"
#include "data/grpdata.hh"
#include "mem/savedata.hh"
#include "util/file.hh"
#include "util/random.hh"
#include "core/config.hh"

namespace hojy::scene {

enum {
    GlobalMapWidth = 480,
    GlobalMapHeight = 480,
};

GlobalMap::GlobalMap(Renderer *renderer, int ix, int iy, int width, int height, std::pair<int, int> scale): MapWithEvent(renderer, ix, iy, width, height, scale) {
    mapWidth_ = GlobalMapWidth;
    mapHeight_ = GlobalMapHeight;
    cloudTexMgr_.setRenderer(renderer_);
    cloudTexMgr_.setPalette(gNormalPalette);
    data::GrpData::DataSet dset;
    if (data::GrpData::loadData("MMAP", dset)) {
        textureMgr_.loadFromRLE(dset);
    }
    renderer_->enableLinear();
    dset.clear();
    if (data::GrpData::loadData("CLOUD", dset)) {
        cloudTexMgr_.loadFromRLE(dset);
    }
    renderer_->enableLinear(false);
    {
        auto *tex = textureMgr_[0];
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
            ci.earth = textureMgr_[n];
            auto &n0 = surface_[pos];
            n0 >>= 1;
            if (n0) {
                ci.surface = textureMgr_[n0];
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
                const auto *tex2 = textureMgr_[n1];
                if (tex2) {
                    auto centerX = tx - tex2->originX() + tex2->width() / 2;
                    auto th = tex2->height();
                    if (th < 36) {
                        th = th * 4 / 5;
                    } else {
                        th = th * 2 / 3;
                    }
                    auto centerY = ty - tex2->originY() + th;
                    buildingTex_.emplace(centerY * texWidth_ + centerX, BuildingTex{tx, ty, tex2});
                }
            }
        }
        x -= cellDiffX; y += cellDiffY;
    }

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

void GlobalMap::load() {
    onShip_ = cellInfo_[currY_ * mapWidth_ + currX_].type == 1;
    if (core::config.shipLogicEnabled()) {
        showShip(!onShip_);
    }
}

void GlobalMap::render() {
    MapWithEvent::render();
    if (drawDirty_) {
        drawDirty_ = false;
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int camX = cameraX_, camY = cameraY_;
        int nx = int(auxWidth_) / 2 + cellWidth_ * 2;
        int ny = int(auxHeight_) / 2 + cellHeight_ * 2;
        int cx = (nx / cellDiffX + ny / cellDiffY) / 2;
        int cy = (ny / cellDiffY - nx / cellDiffX) / 2;
        int wcount = nx * 2 / cellWidth_;
        int hcount = (ny * 2 + 4 * cellHeight_) / cellDiffY;
        int tx = int(auxWidth_) / 2 - (cx - cy) * cellDiffX;
        int ty = int(auxHeight_) / 2 + cellDiffY - (cx + cy) * cellDiffY;
        cx = camX - cx; cy = camY - cy;
        renderer_->setTargetTexture(drawingTerrainTex_);
        renderer_->clear(0, 0, 0, 0);
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

        int ox = (mapHeight_ + camX - camY - 1) * cellDiffX + offsetX_ - int(auxWidth_ / 2);
        int oy = (camX + camY) * cellDiffY + offsetY_ - int(auxHeight_ / 2) - cellDiffY;
        int dx = currX_ - cameraX_;
        int dy = currY_ - cameraY_;
        int offsetY = (dx + dy) * cellDiffY;
        int myy = int(auxHeight_) / 2 + oy + offsetY;
        int l = ox - cellWidth_ * 2, t = oy - cellHeight_ * 2, r = ox + int(auxWidth_) + cellWidth_ * 2, b = oy + int(auxHeight_) + cellHeight_ * 6;
        auto ite = buildingTex_.lower_bound(t * texWidth_ + l);
        auto ite_mid = buildingTex_.upper_bound(myy * texWidth_);
        auto ite_end = buildingTex_.upper_bound(b * texWidth_ + r);
        renderer_->setTargetTexture(drawingBuildingTex_[0]);
        renderer_->clear(0, 0, 0, 0);
        while (ite != ite_mid) {
            if (ite->second.x < l || ite->second.x >= r) {
                ++ite;
                continue;
            }
            renderer_->renderTexture(ite->second.tex, ite->second.x - ox, ite->second.y - oy);
            ++ite;
        }
        renderer_->setTargetTexture(drawingBuildingTex_[1]);
        renderer_->clear(0, 0, 0, 0);
        while (ite != ite_end) {
            if (ite->second.x < l || ite->second.x >= r) {
                ++ite;
                continue;
            }
            renderer_->renderTexture(ite->second.tex, ite->second.x - ox, ite->second.y - oy);
            ++ite;
        }
        renderer_->setTargetTexture(nullptr);
    }
    renderer_->clear(0, 0, 0, 0);
    renderer_->renderTexture(drawingTerrainTex_, x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
    renderer_->renderTexture(drawingBuildingTex_[0], x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
    renderChar();
    renderer_->renderTexture(drawingBuildingTex_[1], x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
    for (int i = 0; i < 2; ++i) {
        auto &c = cloud_[i];
        if (!c) {
            if (util::gRandom(2000)) { continue; }
            c = cloudTexMgr_[util::gRandom(4)];
            cloudStartX_[i] = cameraX_; cloudStartY_[i] = cameraY_;
            cloudX_[i] = -width_ * 3 / 5;
            cloudY_[i] = int(util::gRandom(int(auxHeight_) + height_ / 10) + height_ / 20);
        }
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int cloudcx = cloudStartX_[i] - cameraX_, cloudcy = cloudStartY_[i] - cameraY_;
        int cloudx = (cloudcx - cloudcy) * cellDiffX * scale_.first / scale_.second + cloudX_[i]++ / 2;
        int cloudy = (cloudcx + cloudcy) * cellDiffY * scale_.first / scale_.second + cloudY_[i];
        if (cloudx > width_ * 5 / 2) {
            c = nullptr;
        } else {
            renderer_->renderTexture(c, cloudx, cloudy, scale_);
        }
    }
}

void GlobalMap::showShip(bool show) {
    int cellDiffX = cellWidth_ / 2;
    int cellDiffY = cellHeight_ / 2;
    int x = (mapHeight_ - 1) * cellDiffX + offsetX_;
    int y = offsetY_;
    int shipX0 = mem::gSaveData.baseInfo->shipX;
    int shipY0 = mem::gSaveData.baseInfo->shipY;
    int x0 = x + (shipX0 - shipY0) * cellDiffX;
    int y0 = y + (shipX0 + shipY0) * cellDiffY;
    if (show) {
        int shipX1 = mem::gSaveData.baseInfo->shipX1;
        int shipY1 = mem::gSaveData.baseInfo->shipY1;
        const auto *shipTex = textureMgr_[3715 + int(calcDirection(shipX1, shipY1, shipX0, shipY0)) * 4];
        buildingTex_.emplace(y0 * texWidth_ + x0, BuildingTex{x0, y0, shipTex});
    } else {
        buildingTex_.erase(y0 * texWidth_ + x0);
    }
}

bool GlobalMap::tryMove(int x, int y, bool checkEvent) {
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
        auto *subMapInfo = mem::gSaveData.subMapInfo[ite->second];
        if (subMapInfo->enterCondition == 1) {
            return true;
        }
        if (subMapInfo->enterCondition == 2) {
            bool allow = false;
            for (auto id: mem::gSaveData.baseInfo->members) {
                if (id < 0) { continue; }
                /* TODO: get this limit value from Z.DAT? */
                auto *charInfo = mem::gSaveData.charInfo[id];
                if (charInfo && charInfo->speed >= 70) {
                    allow = true;
                    break;
                }
            }
            if (!allow) {
                return true;
            }
        }
        gWindow->enterSubMap(ite->second, int(direction_));
        auto music = subMapInfo->enterMusic;
        if (music >= 0) {
            gWindow->playMusic(music);
        }
        return true;
    }
    auto offset = y * mapWidth_ + x;
    if (!cellInfo_[offset].canWalk || buildx_[offset] != 0 && building_[buildy_[offset] * mapWidth_ + buildx_[offset]] != 0) {
        return true;
    }
    bool lastOnShip = onShip_;
    if (cellInfo_[offset].type == 1) {
        if (core::config.shipLogicEnabled() && !lastOnShip) {
            if (mem::gSaveData.baseInfo->shipX != x ||
                mem::gSaveData.baseInfo->shipY != y) {
                return true;
            }
        }
        onShip_ = true;
        currMainCharFrame_ = (currMainCharFrame_ + 1) % 4;
        mem::gSaveData.baseInfo->shipX = x;
        mem::gSaveData.baseInfo->shipY = y;
        mem::gSaveData.baseInfo->shipX1 = currX_;
        mem::gSaveData.baseInfo->shipY1 = currY_;
    } else {
        onShip_ = false;
        currMainCharFrame_ = currMainCharFrame_ % 6 + 1;
    }
    if (core::config.shipLogicEnabled() && lastOnShip != onShip_) { showShip(lastOnShip); }
    currX_ = x;
    currY_ = y;
    cameraX_ = x;
    cameraY_ = y;
    drawDirty_ = true;
    return true;
}

void GlobalMap::updateMainCharTexture() {
    if (onShip_) {
        mainCharTex_ = textureMgr_[3715 + int(direction_) * 4 + currMainCharFrame_];
        return;
    }
    if (resting_) {
        mainCharTex_ = textureMgr_[2529 + int(direction_) * 6 + currMainCharFrame_];
        return;
    }
    mainCharTex_ = textureMgr_[2501 + int(direction_) * 7 + currMainCharFrame_];
}

void GlobalMap::resetTime() {
    if (onShip_) { return; }
    MapWithEvent::resetTime();
}

bool GlobalMap::checkTime() {
    if (onShip_) { return false; }
    return MapWithEvent::checkTime();
}

}
