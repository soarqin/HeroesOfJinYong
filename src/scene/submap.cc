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
#include "data/grpdata.hh"
#include "mem/savedata.hh"
#include <fmt/format.h>

namespace hojy::scene {

SubMap::SubMap(Renderer *renderer, int ix, int iy, int width, int height, float scale):
    MapWithEvent(renderer, ix, iy, width, height, scale),
    drawingTerrainTex2_(Texture::createAsTarget(renderer_, width, height)) {
    drawingTerrainTex2_->enableBlendMode(true);
}

SubMap::~SubMap() {
    delete drawingTerrainTex2_;
}

bool SubMap::load(std::int16_t subMapId) {
    if (subMapLoaded_.find(subMapId) == subMapLoaded_.end()) {
        mapWidth_ = data::SubMapWidth;
        mapHeight_ = data::SubMapHeight;
        data::GrpData::DataSet dset;
        if (!data::GrpData::loadData(fmt::format("SDX{:03}", subMapId), fmt::format("SMP{:03}", subMapId), dset)) {
            return false;
        }
        if (!textureMgr_.mergeFromRLE(dset)) {
            return false;
        }
        subMapLoaded_.insert(subMapId);
    }
    cleanupEvents();
    eventLoop_.clear();
    eventDelay_.clear();
    eventLoop_.resize(data::SubMapEventCount);
    eventDelay_.resize(data::SubMapEventCount);
    {
        auto *tex = textureMgr_[0];
        cellWidth_ = tex->width();
        cellHeight_ = tex->height();
        offsetX_ = tex->originX();
        offsetY_ = tex->originY();
    }
    int cellDiffX = cellWidth_ / 2;
    int cellDiffY = cellHeight_ / 2;
    texWidth_ = (mapWidth_ + mapHeight_) * cellDiffX;
    texHeight_ = (mapWidth_ + mapHeight_) * cellDiffY;

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
            ci.isWater = texId >= 179 && texId <= 181 || texId == 261 || texId == 511 || texId >= 662 && texId <= 665 || texId == 674;
            ci.earth = textureMgr_[texId];
            texId = layers[1][pos] >> 1;
            if (texId) {
                ci.building = textureMgr_[texId];
            }
            texId = layers[2][pos] >> 1;
            if (texId) {
                ci.decoration = textureMgr_[texId];
            }
            auto ev = layers[3][pos];
            if (ev >= 0) {
                texId = events[ev].currTex >> 1;
                if (texId) {
                    ci.event = textureMgr_[texId];
                }
            }
            ci.buildingDeltaY = layers[4][pos];
            ci.decorationDeltaY = layers[5][pos];
        }
        x -= cellDiffX; y += cellDiffY;
    }

    subMapId_ = subMapId;
    return true;
}

void SubMap::forceMainCharTexture(std::int16_t id) {
    mainCharTex_ = textureMgr_[id];
    drawDirty_ = true;
}

void SubMap::render() {
    MapWithEvent::render();

    if (drawDirty_) {
        drawDirty_ = false;
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int curX = currX_, curY = currY_;
        int camX = cameraX_, camY = cameraY_;
        int nx = int(auxWidth_) / 2 + cellWidth_ * 2;
        int ny = int(auxHeight_) / 2 + cellHeight_ * 2;
        int wcount = nx * 2 / cellWidth_;
        int hcount = (ny * 2 + 4 * cellHeight_) / cellDiffY;
        int cx, cy, tx, ty;
        int delta = -mapWidth_ + 1;

        renderer_->setTargetTexture(drawingTerrainTex_);
        renderer_->setClipRect(0, 0, 2048, 2048);
        renderer_->clear(0, 0, 0, 0);

/* NOTE: Do we really need to do this?
 *       Earth with height > 0 should not stack with =0 ones
 *       So I just comment it out
        cx = (nx / cellDiffX + ny / cellDiffY) / 2;
        cy = (ny / cellDiffY - nx / cellDiffX) / 2;
        tx = int(auxWidth_) / 2 - (cx - cy) * cellDiffX;
        ty = int(auxHeight_) / 2 - (cx + cy) * cellDiffY;
        cx = curX - cx; cy = curY - cy;
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
                    renderer_->renderTexture(ci.earth, dx, ty);
                /* } */
                if (ci.building) {
                    renderer_->renderTexture(ci.building, dx, ty - h);
                }
                if (x == curX && y == curY) {
                    renderer_->setTargetTexture(drawingTerrainTex2_);
                    renderer_->setClipRect(0, 0, 2048, 2048);
                    renderer_->clear(0, 0, 0, 0);
                    charHeight_ = h;
                }
                if (ci.event) {
                    renderer_->renderTexture(ci.event, dx, ty - h);
                }
                if (ci.decoration) {
                    renderer_->renderTexture(ci.decoration, dx, ty - ci.decorationDeltaY);
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
        renderer_->setTargetTexture(nullptr);
        renderer_->unsetClipRect();
    }

    renderer_->clear(0, 0, 0, 0);
    renderer_->renderTexture(drawingTerrainTex_, x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
    renderChar(charHeight_);
    renderer_->renderTexture(drawingTerrainTex2_, x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
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
    if (ci.building || ci.isWater) {
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
        mainCharTex_ = textureMgr_[animCurrTex_[0] >> 1];
        return;
    }
    if (resting_) {
        mainCharTex_ = textureMgr_[2501 + int(direction_) * 7];
        return;
    }
    mainCharTex_ = textureMgr_[2501 + int(direction_) * 7 + currMainCharFrame_];
}

void SubMap::setCellTexture(int x, int y, int layer, std::int16_t tex) {
    auto *texobj = tex <= 0 ? nullptr : textureMgr_[tex];
    switch (layer) {
    case 0:
        cellInfo_[y * mapWidth_ + x].earth = texobj;
        break;
    case 1:
        cellInfo_[y * mapWidth_ + x].building = texobj;
        break;
    case 2:
        cellInfo_[y * mapWidth_ + x].decoration = texobj;
        break;
    case 3:
        cellInfo_[y * mapWidth_ + x].event = texobj;
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
        ci.event = textureMgr_[ev.currTex >> 1];
        drawDirty_ = true;
    }
}

}
