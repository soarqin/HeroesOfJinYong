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

#include "logic/rle.hh"
#include "colorpalette.hh"
#include "window.hh"
#include "window_command.hh"
#include "content/grpdata.hh"
#include "world/savedata.hh"
#include "util/file.hh"
#include "util/random.hh"
#include "core/config.hh"
#include <cstring>
#include <algorithm>
#include <limits>

#include <memory>

namespace hojy::scene {

enum {
    GlobalMapWidth = 480,
    GlobalMapHeight = 480,
};

namespace {

template<typename T>
bool loadMapArray(const std::string &name, std::size_t expected, std::vector<T> &result) {
    if (!util::File::getFileContent(core::config.dataFilePath(name), result)
        || result.size() < expected) {
        return false;
    }
    result.resize(expected);
    return true;
}

bool validCellCoordinate(std::int32_t x, std::int32_t y) {
    return x >= 0 && x < GlobalMapWidth && y >= 0 && y < GlobalMapHeight;
}

}

GlobalMap::GlobalMap(Renderer *renderer, int ix, int iy, int width, int height, std::pair<int, int> scale):
    MapWithEvent(renderer, ix, iy, width, height, scale),
    drawingTerrainTex2_(Texture::create(renderer_, auxWidth_, auxHeight_)) {
    if (!resourcesReady_ || !drawingTerrainTex2_
        || !drawingTerrainTex2_->enableBlendMode(true)) {
        resourcesReady_ = false;
        return;
    }
    miniMapTex_ = Texture::create(renderer_, 2 * (GlobalMapWidth + GlobalMapHeight - 1) + 1, GlobalMapWidth + GlobalMapHeight - 1 + 1);
    if (!miniMapTex_ || !miniMapTex_->enableBlendMode(true)) {
        resourcesReady_ = false;
        return;
    }
    mapWidth_ = GlobalMapWidth;
    mapHeight_ = GlobalMapHeight;
    cloudTexMgr_.setRenderer(renderer_);
    cloudTexMgr_.setPalette(gNormalPalette);
    if (!::hojy::content::GrpData::loadData("MMAP", texData_)
        || texData_.empty()
        || texData_.size() > static_cast<std::size_t>(std::numeric_limits<std::int16_t>::max())
        || !logic::validateRleData(texData_.front())) {
        resourcesReady_ = false;
        return;
    }
    for (const auto &textureData: texData_) {
        if (!textureData.empty() && !logic::validateRleData(textureData)) {
            resourcesReady_ = false;
            return;
        }
    }
    renderer_->enableLinear();
    ::hojy::content::GrpData::DataSet dset;
    if (::hojy::content::GrpData::loadData("CLOUD", dset)) {
        cloudTexMgr_.loadFromRLE(dset);
    }
    renderer_->enableLinear(false);
    {
        std::uint16_t arr[4] = {};
        std::memcpy(arr, texData_[0].data(), sizeof(arr));
        cellWidth_ = arr[0];
        cellHeight_ = arr[1];
        offsetX_ = arr[2];
        offsetY_ = arr[3];
    }
    if (cellWidth_ <= 0 || cellHeight_ <= 0) {
        resourcesReady_ = false;
        return;
    }
    int cellDiffX = cellWidth_ / 2;
    int cellDiffY = cellHeight_ / 2;
    if (cellDiffX <= 0 || cellDiffY <= 0) {
        resourcesReady_ = false;
        return;
    }
    auto size = mapWidth_ * mapHeight_;
    std::vector<std::uint16_t> earth_, surface_;
    std::vector<std::uint16_t> building, buildx, buildy;
    if (!loadMapArray("EARTH.002", static_cast<std::size_t>(size), earth_)
        || !loadMapArray("SURFACE.002", static_cast<std::size_t>(size), surface_)
        || !loadMapArray("BUILDING.002", static_cast<std::size_t>(size), building)
        || !loadMapArray("BUILDX.002", static_cast<std::size_t>(size), buildx)
        || !loadMapArray("BUILDY.002", static_cast<std::size_t>(size), buildy)) {
        resourcesReady_ = false;
        return;
    }
    std::vector<CellInfo> cellInfo(static_cast<std::size_t>(size));

    int pos = 0;
    for (int j = 0; j < mapHeight_; ++j) {
        for (int i = 0; i < mapWidth_; ++i, ++pos) {
            auto &ci = cellInfo[static_cast<std::size_t>(pos)];
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
            ci.earthId = n;
            ci.surfaceId = surface_[pos] >> 1;
            auto &n1 = building[pos];
            n1 >>= 1;
            if (n1 > 0) {
                ci.canWalk = false;
                if (n1 >= 1008 && n1 <= 1164 || n1 >= 1214 && n1 <= 1238) {
                    ci.type = 2;
                }
                if (n1 && n1 < texData_.size() && !texData_[n1].empty()) {
                    std::uint16_t arr[4] = {};
                    std::memcpy(arr, texData_[n1].data(), sizeof(arr));
                    auto deltaY = (arr[0] + 35) / 36 / 2;
                    if (n1 >= 1176 && n1 <= 1182 || n1 == 1352) {
                        deltaY = arr[1] / 18 + 1;
                    }
                    if (deltaY) {
                        if (!validCellCoordinate(i - deltaY, j - deltaY)) {
                            resourcesReady_ = false;
                            return;
                        }
                        auto &ci2 = cellInfo[(j - deltaY) * mapWidth_ + (i - deltaY)];
                        ci2.buildingId = n1;
                        ci2.buildingDeltaY = deltaY * cellHeight_;
                    } else {
                        ci.buildingId = n1;
                        ci.buildingDeltaY = 0;
                    }
                }
            }
        }
    }
    for (int index = 0; index < size; ++index) {
        const auto earthId = static_cast<std::size_t>(earth_[index] >> 1);
        const auto surfaceId = static_cast<std::size_t>(surface_[index] >> 1);
        const auto buildingId = static_cast<std::size_t>(building[index] >> 1);
        if (earthId >= texData_.size() || texData_[earthId].empty()
            || !logic::validateRleData(texData_[earthId])
            || (surfaceId > 0 && (surfaceId >= texData_.size()
                                  || texData_[surfaceId].empty()
                                  || !logic::validateRleData(texData_[surfaceId])))
            || (buildingId > 0 && (buildingId >= texData_.size()
                                   || texData_[buildingId].empty()
                                   || !logic::validateRleData(texData_[buildingId])))) {
            resourcesReady_ = false;
            return;
        }
    }
    building_ = std::move(building);
    buildx_ = std::move(buildx);
    buildy_ = std::move(buildy);
    cellInfo_ = std::move(cellInfo);
    resetTime();
    updateMainCharSpriteId();
}

GlobalMap::~GlobalMap() {
    delete drawingTerrainTex2_;
}

bool GlobalMap::load(std::int32_t initialX, std::int32_t initialY) {
    if ((initialX < 0) != (initialY < 0)) { return false; }
    const auto loadX = initialX >= 0 ? initialX : currX_;
    const auto loadY = initialY >= 0 ? initialY : currY_;
    if (!resourcesReady_
        || cellInfo_.size() != static_cast<std::size_t>(mapWidth_ * mapHeight_)
        || building_.size() != cellInfo_.size()
        || buildx_.size() != cellInfo_.size()
        || buildy_.size() != cellInfo_.size()
        || !validCellCoordinate(loadX, loadY)) {
        return false;
    }
    const auto *baseInfo = ::hojy::world::state::gSaveData.baseInfo.operator->();
    if (!baseInfo || !validCellCoordinate(baseInfo->shipX, baseInfo->shipY)
        || !validCellCoordinate(baseInfo->shipX1, baseInfo->shipY1)) {
        return false;
    }

    std::vector<MiniMapCell> candidateMiniMapCells;
    candidateMiniMapCells.reserve(cellInfo_.size());
    std::map<std::pair<std::int16_t, std::int16_t>, std::int16_t> nextEntries;
    const int miniMapStartX = 2 * (mapHeight_ - 1) + 1;
    const int miniMapStartY = 1;

    int pos = 0;
    for (int j = 0; j < mapHeight_; ++j) {
        for (int i = 0; i < mapWidth_; ++i, ++pos) {
            if (buildx_[pos] != 0
                && (buildx_[pos] >= mapWidth_ || buildy_[pos] >= mapHeight_)) {
                return false;
            }
            const int mmx = miniMapStartX + (i - j) * 2;
            const int mmy = miniMapStartY + (i + j);
            if (mmx < 1
                || mmx + 1 >= 2 * (GlobalMapWidth + GlobalMapHeight - 1) + 1
                || mmy < 1 || mmy >= GlobalMapWidth + GlobalMapHeight) {
                return false;
            }
            const auto &ci = cellInfo_[pos];
            const bool blocked = !ci.canWalk || (buildx_[pos] != 0
                && building_[buildy_[pos] * mapWidth_ + buildx_[pos]] != 0);
            candidateMiniMapCells.push_back({mmx, mmy, ci.earthId, blocked});
        }
    }

    const auto subMapSz = ::hojy::world::state::gSaveData.subMapInfo.size();
    if (subMapSz > static_cast<std::size_t>(std::numeric_limits<std::int16_t>::max())) {
        return false;
    }
    for (std::size_t i = 0; i < subMapSz; ++i) {
        const auto *smi = ::hojy::world::state::gSaveData.subMapInfo[i];
        if (!smi || !validCellCoordinate(smi->globalEnterX1, smi->globalEnterY1)) {
            return false;
        }
        const auto index = static_cast<std::int16_t>(i);
        auto ex = smi->globalEnterX1;
        auto ey = smi->globalEnterY1;
        nextEntries[std::make_pair(ex, ey)] = index;
        const auto markerX1 = miniMapStartX + (ex - ey) * 2;
        const auto markerY1 = miniMapStartY + (ex + ey);
        if (markerX1 < 1
            || markerX1 + 1 >= 2 * (GlobalMapWidth + GlobalMapHeight - 1) + 1
            || markerY1 < 1 || markerY1 >= GlobalMapWidth + GlobalMapHeight) {
            return false;
        }

        ex = smi->globalEnterX2;
        if (ex >= 0) {
            ey = smi->globalEnterY2;
            if (!validCellCoordinate(ex, ey)) { return false; }
            nextEntries[std::make_pair(ex, ey)] = index;
            const auto markerX2 = miniMapStartX + (ex - ey) * 2;
            const auto markerY2 = miniMapStartY + (ex + ey);
            if (markerX2 < 1
                || markerX2 + 1 >= 2 * (GlobalMapWidth + GlobalMapHeight - 1) + 1
                || markerY2 < 1 || markerY2 >= GlobalMapWidth + GlobalMapHeight) {
                return false;
            }
        }
    }

    miniMapCells_ = std::move(candidateMiniMapCells);
    subMapEntries_ = std::move(nextEntries);
    ++miniMapRevision_;
    if (miniMapRevision_ == 0) {
        miniMapRevision_ = 1;
        preparedMiniMapRevision_ = 0;
    }

    currX_ = cameraX_ = loadX;
    currY_ = cameraY_ = loadY;
    commitMiniPanelSnapshot({}, currX_, currY_);
    onShip_ = cellInfo_[currY_ * mapWidth_ + currX_].type == 1;
    if (core::config.shipLogicEnabled()) {
        showShip(!onShip_);
    }
    // Loading changes camera, protagonist placement and the optional ship
    // overlay as one logical transaction.  Force the presentation view to
    // rebuild from the committed snapshot on the next prepare pass.
    markWorldChanged();
    return true;
}

void GlobalMap::update() {
    MapWithEvent::update();
    for (int i = 0; i < 3; ++i) {
        auto &spriteId = cloudSpriteId_[i];
        if (spriteId < 0) {
            if (util::gRandom(2500)) { continue; }
            spriteId = static_cast<std::int16_t>(util::gRandom(4));
            cloudStartX_[i] = cameraX_; cloudStartY_[i] = cameraY_;
            cloudX_[i] = -width_ * 3 / 5;
            cloudY_[i] = int(util::gRandom(int(auxHeight_) + height_ / 10) + height_ / 20);
        }
    }
    for (int i = 0; i < 3; ++i) {
        if (cloudSpriteId_[i] < 0) { continue; }
        ++cloudX_[i];
        const int cellDiffX = cellWidth_ / 2;
        const int cloudcx = cloudStartX_[i] - cameraX_;
        const int cloudcy = cloudStartY_[i] - cameraY_;
        const int cloudx = (cloudcx - cloudcy) * cellDiffX * scale_.first / scale_.second
            + cloudX_[i] / 2;
        if (cloudx > width_ * 5 / 2) {
            cloudSpriteId_[i] = -1;
        }
    }
}

void GlobalMap::prepareRender() {
    if (!resourcesReady_ || !renderer_) { return; }
    MapWithEvent::prepareRender();
    for (int i = 0; i < 3; ++i) {
        const Texture *candidate = nullptr;
        if (cloudSpriteId_[i] >= 0) {
            candidate = cloudTexMgr_[cloudSpriteId_[i]];
        }
        // The cloud manager may be cleared/reloaded while the logical id is
        // unchanged.  Re-resolve every prepare pass so the view never keeps
        // a pointer into the previous manager generation.
        preparedCloud_[i] = candidate;
        preparedCloudSpriteId_[i] = cloudSpriteId_[i];
    }
    prepareMiniMapRender();
    if (worldPresentationNeedsPrepare()) {
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int camX = cameraX_, camY = cameraY_;
        int nx = int(auxWidth_) / 2 + cellWidth_ * 2;
        int ny = int(auxHeight_) / 2 + cellHeight_ * 2;
        int ocx = (nx / cellDiffX + ny / cellDiffY) / 2;
        int ocy = (ny / cellDiffY - nx / cellDiffX) / 2;
        int wcount = nx * 2 / cellWidth_;
        int hcount = (ny * 2 + 4 * cellHeight_) / cellDiffY;
        int aheight = int(auxHeight_);
        int otx = int(auxWidth_) / 2 - (ocx - ocy) * cellDiffX;
        int oty = aheight / 2 + cellDiffY - (ocx + ocy) * cellDiffY;
        ocx = camX - ocx; ocy = camY - ocy;
        int delta = -mapWidth_ + 1;
        int cx = ocx, cy = ocy, tx = otx, ty = oty;
        std::unique_ptr<Texture> candidateTerrain(Texture::create(renderer_, auxWidth_, auxHeight_));
        std::unique_ptr<Texture> candidateOverlay(Texture::create(renderer_, auxWidth_, auxHeight_));
        if (!candidateTerrain || !candidateOverlay
            || !candidateTerrain->enableBlendMode(true)
            || !candidateOverlay->enableBlendMode(true)) {
            return;
        }
        int terrainPitch, overlayPitch;
        TextureLock terrainLock(candidateTerrain.get(), terrainPitch);
        TextureLock overlayLock(candidateOverlay.get(), overlayPitch);
        if (!terrainLock.valid() || !overlayLock.valid()) { return; }
        auto *terrainPixels = terrainLock.pixels();
        auto *overlayPixels = overlayLock.pixels();
        memset(terrainPixels, 0, terrainPitch * auxHeight_ * sizeof(std::uint32_t));
        memset(overlayPixels, 0, overlayPitch * auxHeight_ * sizeof(std::uint32_t));
        auto *pixels = terrainPixels;
        int pitch = terrainPitch;
        const auto *colors = gNormalPalette.colors();
        bool renderOk = true;
        for (int j = hcount; j; --j) {
            int x = cx, y = cy;
            int dx = tx;
            int offset = y * mapWidth_ + x;
            for (int i = wcount; i; --i, dx += cellWidth_, offset += delta, ++x, --y) {
                if (x < 0 || x >= GlobalMapWidth || y < 0 || y >= GlobalMapHeight) {
                    if (!texData_.empty()) {
                        renderOk = renderOk && Texture::renderRLE(texData_[0], colors, pixels, pitch, aheight, dx, ty);
                    }
                    continue;
                }
                auto &ci = cellInfo_[offset];
                if (ci.earthId >= 0 && ci.earthId < static_cast<int>(texData_.size())) {
                    renderOk = renderOk && Texture::renderRLE(texData_[ci.earthId], colors, pixels, pitch, aheight, dx, ty);
                }
                if (ci.surfaceId > 0 && ci.surfaceId < static_cast<int>(texData_.size())) {
                    renderOk = renderOk && Texture::renderRLE(texData_[ci.surfaceId], colors, pixels, pitch, aheight, dx, ty);
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
        cx = ocx; cy = ocy; tx = otx; ty = oty;
        int charX = currX_, charY = currY_;
        for (int j = hcount; j; --j) {
            int x = cx, y = cy;
            int dx = tx;
            int offset = y * mapWidth_ + x;
            for (int i = wcount; i; --i, dx += cellWidth_, offset += delta, ++x, --y) {
                if (x < 0 || x >= GlobalMapWidth || y < 0 || y >= GlobalMapHeight) {
                    continue;
                }
                auto &ci = cellInfo_[offset];
                if (ci.buildingId > 0 && ci.buildingId < static_cast<int>(texData_.size())) {
                    renderOk = renderOk && Texture::renderRLE(texData_[ci.buildingId], colors, pixels, pitch, aheight, dx, ty + ci.buildingDeltaY);
                }
                if (x == charX && y == charY) {
                    pixels = overlayPixels;
                    pitch = overlayPitch;
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
        if (!renderOk) { return; }
        overlayLock.unlock();
        terrainLock.unlock();
        int miniMapStartX = 2 * (mapHeight_ - 1) + 1 + 2 * (cameraX_ - cameraY_);
        int miniMapStartY = 1 + cameraX_ + cameraY_;
        const auto nextMiniMapAuxX = miniMapStartX - miniMapAuxW_ / 2;
        const auto nextMiniMapAuxY = miniMapStartY - miniMapAuxH_ / 2;
        delete drawingTerrainTex_;
        drawingTerrainTex_ = candidateTerrain.release();
        delete drawingTerrainTex2_;
        drawingTerrainTex2_ = candidateOverlay.release();
        miniMapAuxX_ = nextMiniMapAuxX;
        miniMapAuxY_ = nextMiniMapAuxY;
        commitWorldPresentation();
    }
}

void GlobalMap::showShip(bool show) {
    int shipX0 = ::hojy::world::state::gSaveData.baseInfo->shipX;
    int shipY0 = ::hojy::world::state::gSaveData.baseInfo->shipY;
    auto &ci = cellInfo_[shipY0 * mapWidth_ + shipX0];
    if (show) {
        int shipX1 = ::hojy::world::state::gSaveData.baseInfo->shipX1;
        int shipY1 = ::hojy::world::state::gSaveData.baseInfo->shipY1;
        ci.buildingId = 3715 + int(calcDirection(shipX1, shipY1, shipX0, shipY0)) * 4;
        ci.buildingDeltaY = 0;
    } else {
        ci.buildingId = 0;
    }
}

bool GlobalMap::tryMove(int x, int y, bool checkEvent) {
    auto ite = subMapEntries_.find(std::make_pair(std::int16_t(x), std::int16_t(y)));
    if (ite != subMapEntries_.end()) {
        auto *subMapInfo = ::hojy::world::state::gSaveData.subMapInfo[ite->second];
        if (subMapInfo->enterCondition == 1) {
            return true;
        }
        if (subMapInfo->enterCondition == 2) {
            bool allow = false;
            for (auto id: ::hojy::world::state::gSaveData.baseInfo->members) {
                if (id < 0) { continue; }
                /* TODO: get this limit value from Z.DAT? */
                auto *charInfo = ::hojy::world::state::gSaveData.charInfo[id];
                if (charInfo && charInfo->speed >= 70) {
                    allow = true;
                    break;
                }
            }
            if (!allow) {
                return true;
            }
        }
        const auto subMapId = ite->second;
        const auto direction = int(direction_);
        postSceneCommand(this, [subMapId, direction](SceneCommandContext &context) {
            context.enterSubMap(subMapId, direction);
        });
        auto music = subMapInfo->enterMusic;
        if (music >= 0) {
            postSceneCommand(this, [music](SceneCommandContext &context) { context.playMusic(music); });
        }
        return true;
    }
    auto offset = y * mapWidth_ + x;
    if (!cellInfo_[offset].canWalk || buildx_[offset] != 0 && building_[buildy_[offset] * mapWidth_ + buildx_[offset]] != 0) {
        if (onShip_) {
            currMainCharFrame_ = (currMainCharFrame_ + 1) % 4;
        } else {
            currMainCharFrame_ = currMainCharFrame_ % 6 + 1;
        }
        return true;
    }
    bool lastOnShip = onShip_;
    if (cellInfo_[offset].type == 1) {
        if (core::config.shipLogicEnabled() && !lastOnShip) {
            if (::hojy::world::state::gSaveData.baseInfo->shipX != x ||
                ::hojy::world::state::gSaveData.baseInfo->shipY != y) {
                return true;
            }
        }
        onShip_ = true;
        currMainCharFrame_ = (currMainCharFrame_ + 1) % 4;
        ::hojy::world::state::gSaveData.baseInfo->shipX = x;
        ::hojy::world::state::gSaveData.baseInfo->shipY = y;
        ::hojy::world::state::gSaveData.baseInfo->shipX1 = currX_;
        ::hojy::world::state::gSaveData.baseInfo->shipY1 = currY_;
    } else {
        onShip_ = false;
        currMainCharFrame_ = currMainCharFrame_ % 6 + 1;
    }
    if (core::config.shipLogicEnabled() && lastOnShip != onShip_) { showShip(lastOnShip); }
    currX_ = x;
    currY_ = y;
    cameraX_ = x;
    cameraY_ = y;
    markWorldChanged();
    return true;
}

void GlobalMap::updateMainCharSpriteId() {
    if (onShip_) {
        mainCharSpriteId_ = 3715 + int(direction_) * 4 + currMainCharFrame_;
        return;
    }
    if (resting_) {
        mainCharSpriteId_ = 2529 + int(direction_) * 6 + currMainCharFrame_;
        return;
    }
    mainCharSpriteId_ = 2501 + int(direction_) * 7 + currMainCharFrame_;
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
