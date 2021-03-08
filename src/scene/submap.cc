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
        mapWidth_ = mem::SubMapWidth;
        mapHeight_ = mem::SubMapHeight;
        char idxstr[8], grpstr[8];
        snprintf(idxstr, 8, "SDX%03d", subMapId);
        snprintf(grpstr, 8, "SMP%03d", subMapId);
        auto &submapData = data::gGrpData.lazyLoad(idxstr, grpstr);
        if (submapData.empty() || !textureMgr.mergeFromRLE(submapData)) {
            return false;
        }
        subMapLoaded_.insert(subMapId);
    }
    eventLoop_.clear();
    eventDelay_.clear();
    eventLoop_.resize(mem::SubMapEventCount);
    eventDelay_.resize(mem::SubMapEventCount);
    frames_ = 0;
    nextEventCheck_ = gWindow->currTime();
    {
        auto *tex = textureMgr[0];
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
            ci.earth = textureMgr[texId];
            texId = layers[1][pos] >> 1;
            if (texId) {
                ci.building = textureMgr[texId];
            }
            texId = layers[2][pos] >> 1;
            if (texId) {
                ci.decoration = textureMgr[texId];
            }
            auto ev = layers[3][pos];
            if (ev >= 0) {
                texId = events[ev].currTex >> 1;
                if (texId) {
                    ci.event = textureMgr[texId];
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
    mainCharTex_ = textureMgr[id];
    drawDirty_ = true;
}

void SubMap::render() {
    MapWithEvent::render();

    if (drawDirty_) {
        drawDirty_ = false;
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int curX = currX_, curY = currY_;
        int nx = int(auxWidth_) / 2 + int(cellWidth_ * scale_);
        int ny = int(auxHeight_) / 2 + int(cellHeight_ * scale_);
        int wcount = nx * 2 / cellWidth_;
        int hcount = (ny * 2 + int(2 * cellHeight_ * scale_)) / cellDiffY;
        int cx, cy, tx, ty;
        int delta = -mapWidth_ + 1;

        renderer_->setTargetTexture(drawingTerrainTex_);
        renderer_->setClipRect(0, 0, 2048, 2048);
        renderer_->fill(0, 0, 0, 0);

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
                if (x < 0 || x >= mem::SubMapWidth || y < 0 || y >= mem::SubMapHeight) {
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
        ty = int(auxHeight_) / 2 - (cx + cy) * cellDiffY;
        cx = curX - cx; cy = curY - cy;
        for (int j = hcount; j; --j) {
            int x = cx, y = cy;
            int dx = tx;
            int offset = y * mapWidth_ + x;
            for (int i = wcount; i; --i, dx += cellWidth_, offset += delta, ++x, --y) {
                if (x < 0 || x >= mem::SubMapWidth || y < 0 || y >= mem::SubMapHeight) {
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
                    renderer_->fill(0, 0, 0, 0);
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

    renderer_->fill(0, 0, 0, 0);
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
    drawDirty_ = true;
    currFrame_ = currFrame_ % 6 + 1;
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
    if (checkEvent) {
        onMove();
    }
    return true;
}

void SubMap::updateMainCharTexture() {
    if (animEventId_ < 0) {
        mainCharTex_ = textureMgr[animCurrTex_ >> 1];
        return;
    }
    if (resting_) {
        mainCharTex_ = textureMgr[2501 + int(direction_) * 7];
        return;
    }
    mainCharTex_ = textureMgr[2501 + int(direction_) * 7 + currFrame_];
}

void SubMap::setCellTexture(int x, int y, int layer, std::int16_t tex) {
    if (tex < 0) { return; }
    switch (layer) {
    case 0:
        cellInfo_[y * mapWidth_ + x].earth = textureMgr[tex];
        break;
    case 1:
        cellInfo_[y * mapWidth_ + x].building = textureMgr[tex];
        break;
    case 2:
        cellInfo_[y * mapWidth_ + x].decoration = textureMgr[tex];
        break;
    case 3:
        cellInfo_[y * mapWidth_ + x].event = textureMgr[tex];
        break;
    default:
        return;
    }
    drawDirty_ = true;
}

void SubMap::updateEventTextures() {
    MapWithEvent::updateEventTextures();
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
        ci.event = textureMgr[ev.currTex >> 1];
        drawDirty_ = true;
    }
}

}
