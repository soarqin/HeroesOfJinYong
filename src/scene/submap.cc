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

#include "data/grpdata.hh"
#include "data/event.hh"
#include "mem/savedata.hh"

#include <functional>

template <class R, class... Args>
constexpr auto argCounter(std::function<R(Args...)>) {
    return sizeof...(Args);
}

template <class P, class R, class... Args>
constexpr auto argCounter(R(P::*)(Args...)) {
    return sizeof...(Args);
}

template <class R, class... Args>
constexpr auto argCounter(R(Args...)) {
    return sizeof...(Args);
}

template <class P, class R, class M, class... Args>
struct returnTypeMatches {};

template <class P, class R, class M, class... Args>
struct returnTypeMatches<R(P::*)(Args...), P, M> {
    static constexpr bool value = std::is_same<R, M>::value;
};

namespace hojy::scene {

SubMap::SubMap(Renderer *renderer, std::uint32_t width, std::uint32_t height, float scale, std::int16_t id): Map(renderer, width, height, scale), subMapId_(id) {
    drawingTerrainTex2_ = Texture::createAsTarget(renderer_, 2048, 2048);
    drawingTerrainTex2_->enableBlendMode(true);

    mapWidth_ = mem::SubMapWidth;
    mapHeight_ = mem::SubMapHeight;
    char idxstr[8], grpstr[8];
    snprintf(idxstr, 8, "SDX%03d", id);
    snprintf(grpstr, 8, "SMP%03d", id);
    auto &submapData = data::gGrpData.lazyLoad(idxstr, grpstr);
    auto sz = submapData.size();
    for (size_t i = 0; i < sz; ++i) {
        textureMgr.loadFromRLE(i, submapData[i]);
    }

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
    cellInfo_.resize(size);

    auto &layers = mem::currSave.subMapLayerInfo[id]->data;
    auto &events = mem::currSave.subMapEventInfo[id]->events;
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

    currX_ = 19, currY_ = 20;
    resetTime();
    updateMainCharTexture();
}

SubMap::~SubMap() {
    delete drawingTerrainTex2_;
}

void SubMap::render() {
    Map::render();

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
    renderer_->renderTexture(drawingTerrainTex_, 0, 0, width_, height_, 0, 0, auxWidth_, auxHeight_);
    renderChar(charHeight_);
    renderer_->renderTexture(drawingTerrainTex2_, 0, 0, width_, height_, 0, 0, auxWidth_, auxHeight_);
}

void SubMap::handleKeyInput(Key key) {
    switch (key) {
    case KeyOK:
        doInteract();
        break;
    default:
        Map::handleKeyInput(key);
        break;
    }
}

bool SubMap::tryMove(int x, int y) {
    auto pos = y * mapWidth_ + x;
    auto &ci = cellInfo_[pos];
    if (ci.building || ci.isWater) {
        return true;
    }
    auto &layers = mem::currSave.subMapLayerInfo[subMapId_]->data;
    auto &events = mem::currSave.subMapEventInfo[subMapId_]->events;
    auto ev = layers[3][pos];
    if (ev >= 0 && events[ev].blocked) {
        return true;
    }

    currX_ = x;
    currY_ = y;
    drawDirty_ = true;
    currFrame_ = currFrame_ % 6 + 1;
    return true;
}

void SubMap::updateMainCharTexture() {
    if (resting_) {
        mainCharTex_ = textureMgr[2501 + int(direction_) * 7];
        return;
    }
    mainCharTex_ = textureMgr[2501 + int(direction_) * 7 + currFrame_];
}

template<class F, class P, size_t ...I>
typename std::enable_if<returnTypeMatches<F, P, void>::value, void>::type
runFunc(F f, P *p, const std::vector<std::int16_t> &evlist, size_t &index, std::index_sequence<I...>) {
    (p->*f)(evlist[I + index]...);
    index += sizeof...(I);
}

template<class F, class P, size_t ...I>
typename std::enable_if<returnTypeMatches<F, P, bool>::value, void>::type
runFunc(F f, P *p, const std::vector<std::int16_t> &evlist, size_t &index, std::index_sequence<I...>) {
    if ((p->*f)(evlist[I + index]...)) {
        index += sizeof...(I);
        index += evlist[index] + 2;
    } else {
        index += sizeof...(I);
        index += evlist[index + 1] + 2;
    }
}

void SubMap::doInteract() {
    int x, y;
    getFaceOffset(x, y);

    auto &layers = mem::currSave.subMapLayerInfo[subMapId_]->data;
    auto &events = mem::currSave.subMapEventInfo[subMapId_]->events;
    auto ev = layers[3][y * mapWidth_ + x];

#define OpRun(O, F)                                                    \
    case O: \
        runFunc(&SubMap::F, this, evlist, index, std::make_index_sequence<argCounter(&SubMap::F)>()); \
        break
    if (ev >= 0) {
        auto evt = events[ev].event1;
        if (evt > 0) {
            evt_ = ev;
            const auto &evlist = data::gEvent.event(evt);
            auto evsz = evlist.size();
            size_t index = 0;
            while (index < evsz) {
                auto op = evlist[index++];
                if (op == -1) { break; }
                switch (op) {
                OpRun(1, doTalk);
                OpRun(2, addItem);
                OpRun(3, modifyEvent);
                default:
                    break;
                }
            }
            evt_ = -1;
        }
    }
}

void SubMap::doTalk(std::int16_t talkId, std::int16_t portrait, std::int16_t position) {
}

void SubMap::addItem(std::int16_t itemId, std::int16_t itemCount) {

}

void SubMap::modifyEvent(std::int16_t subMapId, std::int16_t eventId, std::int16_t blocked, std::int16_t index,
                         std::int16_t event1, std::int16_t event2, std::int16_t event3, std::int16_t currTex,
                         std::int16_t endTex, std::int16_t begTex, std::int16_t texDelay, std::int16_t x,
                         std::int16_t y) {
    if (subMapId < 0) { subMapId = subMapId_; }
    if (eventId < 0) { eventId = evt_; }
    auto &ev = mem::currSave.subMapEventInfo[subMapId]->events[eventId];
    if (blocked > -2) { ev.blocked = blocked; }
    if (index > -2) { ev.index = index; }
    if (event1 > -2) { ev.event1 = event1; }
    if (event2 > -2) { ev.event2 = event2; }
    if (event3 > -2) { ev.event3 = event3; }
    if (endTex > -2) { ev.endTex = endTex; }
    if (begTex > -2) { ev.begTex = begTex; }
    if (texDelay > -2) { ev.texDelay = texDelay; }
    bool needTransport = false;
    if (x < 0) { x = ev.x; }
    if (y < 0) { y = ev.y; }
    if (x != ev.x || y != ev.y) {
        auto &layer = mem::currSave.subMapLayerInfo[subMapId]->data[3];
        layer[ev.y * mapWidth_ + ev.x] = -1;
        layer[y * mapWidth_ + x] = eventId;
        ev.x = x; ev.y = y;
    }
    if (currTex > -2) {
        ev.currTex = currTex;
        auto &ci = cellInfo_[y * mapWidth_ + x];
        auto texId = currTex >> 1;
        if (texId) {
            ci.event = textureMgr[texId];
        }
        drawDirty_ = true;
    }
}

}
