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

#include "submap.hh"

#include "window.hh"
#include "colorpalette.hh"
#include "data/grpdata.hh"
#include "mem/savedata.hh"
#include <fmt/format.h>

namespace hojy::scene {

SubMap::SubMap(Renderer *renderer, int ix, int iy, int width, int height, std::pair<int, int> scale):
    MapWithEvent(renderer, ix, iy, width, height, scale),
    drawingTerrainTex2_(Texture::create(renderer_, auxWidth_, auxHeight_)) {
    drawingTerrainTex2_->enableBlendMode(true);
}

SubMap::~SubMap() {
    delete drawingTerrainTex2_;
}

bool SubMap::load(std::int16_t subMapId) {
    if (subMapLoaded_.find(subMapId) == subMapLoaded_.end()) {
        mapWidth_ = data::SubMapWidth;
        mapHeight_ = data::SubMapHeight;
        if (data::GrpData::loadData("SDX", "SMP", texData_)) {
            for (std::int16_t i = 0; i < 1000; ++i) {
                subMapLoaded_.insert(i);
            }
        } else {
            data::GrpData::DataSet dset;
            if (!data::GrpData::loadData(fmt::format("SDX{:03}", subMapId), fmt::format("SMP{:03}", subMapId), dset)) {
                return false;
            }
            if (dset.size() > texData_.size()) {
                texData_.resize(dset.size());
            }
            for (size_t i = 0; i < dset.size(); ++i) {
                if (dset[i].empty()) { continue; }
                if (!texData_[i].empty()) { continue; }
                texData_[i] = std::move(dset[i]);
            }
            subMapLoaded_.insert(subMapId);
        }
    }
    cleanupEvents();
    eventLoop_.clear();
    eventDelay_.clear();
    eventLoop_.resize(data::SubMapEventCount);
    eventDelay_.resize(data::SubMapEventCount);
    {
        const auto *arr = reinterpret_cast<const uint16_t*>(texData_[0].data());
        cellWidth_ = arr[0];
        cellHeight_ = arr[1];
        offsetX_ = arr[2];
        offsetY_ = arr[3];
    }
    int cellDiffX = cellWidth_ / 2;
    int cellDiffY = cellHeight_ / 2;
    auto size = mapWidth_ * mapHeight_;
    cellInfo_.clear();
    cellInfo_.resize(size);

    auto &layers = mem::gSaveData.subMapLayerInfo[subMapId]->data;
    auto &events = mem::gSaveData.subMapEventInfo[subMapId]->events;
    int x = (mapHeight_ - 1) * cellDiffX + offsetX_;
    int y = offsetY_;
    int pos = 0;
    for (int j = mapHeight_; j; --j) {
        int tx = x, ty = y;
        for (int i = mapWidth_; i; --i, ++pos, tx += cellDiffX, ty += cellDiffY) {
            auto &ci = cellInfo_[pos];
            auto texId = layers[0][pos] >> 1;
            ci.blocked = texId >= 179 && texId <= 181 || texId == 261 || texId == 511 || texId >= 662 && texId <= 665 || texId == 674;
            ci.earthId = texId;
            ci.buildingId = layers[1][pos] >> 1;
            if (ci.buildingId >= 0 && texData_[ci.buildingId].empty()) {
                ci.blocked = true;
            }
            ci.decorationId = layers[2][pos] >> 1;
            auto ev = layers[3][pos];
            if (ev >= 0) {
                ci.eventId = events[ev].currTex >> 1;
            }
            ci.buildingDeltaY = layers[4][pos];
            ci.decorationDeltaY = layers[5][pos];
        }
        x -= cellDiffX; y += cellDiffY;
    }
    resetFrame();

    subMapId_ = subMapId;
    return true;
}

void SubMap::forceMainCharTexture(std::int16_t id) {
    mainCharTex_ = getOrLoadTexture(id);
    drawDirty_ = true;
}

void SubMap::render() {
    Map::render();

    if (drawDirty_) {
        drawDirty_ = false;
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int curX = currX_, curY = currY_;
        int camX = cameraX_, camY = cameraY_;
        int aheight = int(auxHeight_);
        int nx = int(auxWidth_) / 2 + cellWidth_ * 2;
        int ny = aheight / 2 + cellHeight_ * 2;
        int wcount = nx * 2 / cellWidth_;
        int hcount = (ny * 2 + 4 * cellHeight_) / cellDiffY;
        int cx, cy, tx, ty;
        int delta = -mapWidth_ + 1;

        const auto *colors = gNormalPalette.colors();
        auto *curTex = drawingTerrainTex_;
        int pitch;
        std::uint32_t *pixels = curTex->lock(pitch);
        memset(pixels, 0, pitch * auxHeight_ * sizeof(std::uint32_t));

/* NOTE: Do we really need to do this?
 *       Earth with height > 0 should not stack with =0 ones
 *       So I just comment it out
        cx = (nx / cellDiffX + ny / cellDiffY) / 2;
        cy = (ny / cellDiffY - nx / cellDiffX) / 2;
        tx = int(auxWidth_) / 2 - (cx - cy) * cellDiffX;
        ty = int(auxHeight_) / 2 + cellDiffY - (cx + cy) * cellDiffY;
        cx = camX - cx; cy = camY - cy;
        for (int j = hcount; j; --j) {
            int x = cx, y = cy;
            int dx = tx;
            int offset = y * mapWidth_ + x;
            for (int i = wcount; i; --i, dx += cellWidth_, offset += delta, ++x, --y) {
                if (x < 0 || x >= data::SubMapWidth || y < 0 || y >= data::SubMapHeight) {
                    continue;
                }
                auto &ci = cellInfo_[offset];
                auto h = ci.buildingDeltaY;
                if (h == 0) {
                    renderer_->renderTexture(ci.earth, dx, ty);
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
 */
        cx = (nx / cellDiffX + ny / cellDiffY) / 2;
        cy = (ny / cellDiffY - nx / cellDiffX) / 2;
        tx = int(auxWidth_) / 2 - (cx - cy) * cellDiffX;
        ty = int(auxHeight_) / 2 + cellDiffY - (cx + cy) * cellDiffY;
        cx = camX - cx; cy = camY - cy;
        int texCount = texData_.size();
        for (int j = hcount; j; --j) {
            int x = cx, y = cy;
            int dx = tx;
            int offset = y * mapWidth_ + x;
            for (int i = wcount; i; --i, dx += cellWidth_, offset += delta, ++x, --y) {
                if (x < 0 || x >= data::SubMapWidth || y < 0 || y >= data::SubMapHeight) {
                    continue;
                }
                auto &ci = cellInfo_[offset];
                auto h = ci.buildingDeltaY;
                /* if (h > 0) {  NOTE: commented out, see notes above */
                Texture::renderRLE(texData_[ci.earthId], colors, pixels, pitch, aheight, dx, ty);
                /* } */
                if (ci.buildingId > 0 && ci.buildingId < texCount) {
                    Texture::renderRLE(texData_[ci.buildingId], colors, pixels, pitch, aheight, dx, ty - h);
                }
                if (x == curX && y == curY) {
                    curTex->unlock();
                    curTex = drawingTerrainTex2_;
                    pixels = curTex->lock(pitch);
                    memset(pixels, 0, pitch * auxHeight_ * sizeof(std::uint32_t));
                    charHeight_ = h;
                }
                if (ci.eventId > 0 && ci.eventId < texCount) {
                    Texture::renderRLE(texData_[ci.eventId], colors, pixels, pitch, aheight, dx, ty - h);
                }
                if (ci.decorationId > 0 && ci.decorationId < texCount) {
                    Texture::renderRLE(texData_[ci.decorationId], colors, pixels, pitch, aheight, dx, ty - ci.decorationDeltaY);
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
        curTex->unlock();
    }

    renderer_->clear(0, 0, 0, 255);
    renderer_->renderTexture(drawingTerrainTex_, x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
    renderChar(charHeight_);
    renderer_->renderTexture(drawingTerrainTex2_, x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
    showMiniPanel();
}

void SubMap::handleKeyInput(Key key) {
    if (currEventPaused_) { return; }
    switch (key) {
    case KeyOK: case KeySpace:
        doInteract();
        break;
    default:
        MapWithEvent::handleKeyInput(key);
        break;
    }
}

bool SubMap::tryMove(int x, int y, bool checkEvent) {
    auto pos = y * mapWidth_ + x;
    auto &ci = cellInfo_[pos];
    if (ci.buildingId || ci.blocked) {
        return true;
    }
    auto &layers = mem::gSaveData.subMapLayerInfo[subMapId_]->data;
    auto &events = mem::gSaveData.subMapEventInfo[subMapId_]->events;
    auto ev = layers[3][pos];
    if (ev >= 0 && events[ev].blocked) {
        return true;
    }
    currX_ = x;
    currY_ = y;
    cameraX_ = x;
    cameraY_ = y;
    drawDirty_ = true;
    currMainCharFrame_ = currMainCharFrame_ % 6 + 1;
    if (checkEvent) {
        onMove();
    }
    const auto &subMapInfo = mem::gSaveData.subMapInfo[subMapId_];
    for (int i = 0; i < 3; ++i) {
        if (subMapInfo->exitX[i] == currX_ && subMapInfo->exitY[i] == currY_) {
            gWindow->exitToGlobalMap(int(direction_));
            if (subMapInfo->exitMusic >= 0) {
                gWindow->playMusic(subMapInfo->exitMusic);
            }
            return true;
        }
    }
    if (subMapInfo->switchSubMap >= 0 && subMapInfo->switchSubMapX == currX_ && subMapInfo->switchSubMapY == currY_) {
        gWindow->enterSubMap(subMapInfo->switchSubMap, int(direction_));
        auto music = subMapInfo->enterMusic;
        if (music >= 0) {
            gWindow->playMusic(music);
        }
        return true;
    }
    return true;
}

void SubMap::updateMainCharTexture() {
    if (animEventId_[0] < 0) {
        mainCharTex_ = getOrLoadTexture(animCurrTex_[0] >> 1);
        return;
    }
    if (resting_) {
        mainCharTex_ = getOrLoadTexture(2501 + int(direction_) * 7);
        return;
    }
    mainCharTex_ = getOrLoadTexture(2501 + int(direction_) * 7 + currMainCharFrame_);
}

void SubMap::setCellTexture(int x, int y, int layer, std::int16_t tex) {
    switch (layer) {
    case 0:
        cellInfo_[y * mapWidth_ + x].earthId = tex;
        break;
    case 1:
        cellInfo_[y * mapWidth_ + x].buildingId = tex;
        break;
    case 2:
        cellInfo_[y * mapWidth_ + x].decorationId = tex;
        break;
    case 3:
        cellInfo_[y * mapWidth_ + x].eventId = tex;
        break;
    default:
        return;
    }
    drawDirty_ = true;
}

void SubMap::frameUpdate() {
    MapWithEvent::frameUpdate();
    auto &evlist = mem::gSaveData.subMapEventInfo[subMapId_];
    for (auto &ev: evlist->events) {
        if (ev.x <= 0) { break; }
        if (ev.begTex == ev.endTex) { continue; }
        if (ev.currTex == ev.begTex) {
            if (eventDelay_[ev.index]) {
                if (--eventDelay_[ev.index] == 0) {
                    eventLoop_[ev.index] = 0;
                }
                continue;
            }
        }
        if (ev.currTex == ev.endTex) {
            ev.currTex = ev.begTex;
            if (++eventLoop_[ev.index] == 3) {
                eventDelay_[ev.index] = ev.texDelay - std::abs(ev.endTex - ev.begTex);
            }
        } else {
            int step = ev.begTex < ev.endTex ? 1 : -1;
            ev.currTex += step;
        }
        auto &ci = cellInfo_[ev.y * mapWidth_ + ev.x];
        ci.eventId = ev.currTex >> 1;
        drawDirty_ = true;
    }
}

}
