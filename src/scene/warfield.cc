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

#include "warfield.hh"

#include "colorpalette.hh"
#include "data/grpdata.hh"
#include "data/warfielddata.hh"
#include "mem/savedata.hh"
#include <map>

namespace hojy::scene {

WarField::WarField(Renderer *renderer, int x, int y, int width, int height, float scale):
    Map(renderer, x, y, width, height, scale) {
}

WarField::~WarField() {
    delete maskTex_;
}

bool WarField::load(std::int16_t warId) {
    warId_ = warId;
    const auto *info = data::gWarFieldData.info(warId);
    auto warMapId = info->warFieldId;
    const auto &layers = data::gWarFieldData.layers(warMapId)->layers;
    if (warMapLoaded_.find(warMapId) == warMapLoaded_.end()) {
        mapWidth_ = data::WarFieldWidth;
        mapHeight_ = data::WarFieldHeight;
        char idxstr[8], grpstr[8];
        snprintf(idxstr, 8, "WDX%03d", warMapId);
        snprintf(grpstr, 8, "WMP%03d", warMapId);
        auto &warMapData = data::gGrpData.lazyLoad(idxstr, grpstr);
        if (warMapData.empty() || !textureMgr_.mergeFromRLE(warMapData)) {
            return false;
        }
        warMapLoaded_.insert(warMapId);
        if (!maskTex_) {
            maskTex_ = new Texture;
            maskTex_->loadFromRLE(renderer_, warMapData[0], gMaskPalette);
            maskTex_->enableBlendMode(true);
        }
    }
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
        }
        x -= cellDiffX; y += cellDiffY;
    }

    subMapId_ = warMapId;
    return true;
}

bool WarField::getDefaultChars(std::set<std::int16_t> &chars) const {
    const auto *info = data::gWarFieldData.info(warId_);
    if (info->autoAlly[0] >= 0) { return false; }
    for (auto &id: info->ally) {
        if (id >= 0) { chars.insert(id); }
    }
    return true;
}

void WarField::putChars(const std::vector<std::int16_t> &chars) {
    const auto *info = data::gWarFieldData.info(warId_);
    if (info->autoAlly[0] >= 0) {
        for (size_t i = 0; i < data::TeamMemberCount; ++i) {
            auto id = info->autoAlly[i];
            if (id < 0) { continue; }
            const auto *charInfo = mem::gSaveData.charInfo[id];
            charQueue_.emplace_back(CharInfo {0, id, info->allyX[i], info->allyY[i], DirLeft,
                                              charInfo->speed, charInfo->maxHp, charInfo->maxMp, data::StaminaMax, 0});
        }
    } else {
        std::map<std::int16_t, size_t> charMap;
        std::set<size_t> indices;
        for (size_t i = 0; i < data::TeamMemberCount; ++i) {
            auto id = info->ally[i];
            if (id >= 0) { charMap[id] = i; }
            else { indices.insert(i); }
        }
        for (auto id: chars) {
            const auto *charInfo = mem::gSaveData.charInfo[id];
            auto ite = charMap.find(id);
            size_t index;
            if (ite != charMap.end()) {
                index = ite->second;
            } else {
                index = *indices.begin();
                indices.erase(indices.begin());
            }
            charQueue_.emplace_back(CharInfo{0, id, info->allyX[index], info->allyY[index], DirLeft,
                                             charInfo->speed, charInfo->hp, charInfo->mp, charInfo->stamina, 0});
        }
    }
    for (size_t i = 0; i < data::WarFieldEnemyCount; ++i) {
        auto id = info->enemy[i];
        if (id < 0) { continue; }
        const auto *charInfo = mem::gSaveData.charInfo[id];
        charQueue_.emplace_back(CharInfo {1, id, info->enemyX[i], info->enemyY[i], DirRight,
                                          charInfo->speed, charInfo->maxHp, charInfo->maxMp, data::StaminaMax, 0});
    }
    std::sort(charQueue_.begin(), charQueue_.end(), [](const CharInfo &c0, const CharInfo &c1) {
        return c0.speed > c1.speed;
    });
    for (auto &ci: charQueue_) {
        auto &cell = cellInfo_[ci.y * mapWidth_ + ci.x];
        cell.charId = ci.id;
        cell.charTex = textureMgr_[2553 + 4 * ci.id];
    }
    currX_ = charQueue_[0].x;
    currY_ = charQueue_[0].y;
}

void WarField::render() {
    Map::render();

    if (drawDirty_) {
        drawDirty_ = false;
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int curX = currX_, curY = currY_;
        int nx = int(auxWidth_) / 2 + int(cellWidth_ * scale_);
        int ny = int(auxHeight_) / 2 + int(cellHeight_ * scale_);
        int wcount = nx * 2 / cellWidth_;
        int hcount = (ny * 2 + int(float(2 * cellHeight_) * scale_)) / cellDiffY;
        int cx, cy, tx, ty;
        int delta = -mapWidth_ + 1;

        renderer_->setTargetTexture(drawingTerrainTex_);
        renderer_->setClipRect(0, 0, 2048, 2048);
        renderer_->clear(0, 0, 0, 0);

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
                if (x < 0 || x >= data::WarFieldWidth || y < 0 || y >= data::WarFieldHeight) {
                    continue;
                }
                auto &ci = cellInfo_[offset];
                renderer_->renderTexture(ci.earth, dx, ty);
                if (ci.charTex) {
                    maskTex_->setBlendColor(192, 192, 192, 204);
                    renderer_->renderTexture(maskTex_, dx, ty);
                }
                if (ci.building) {
                    renderer_->renderTexture(ci.building, dx, ty);
                }
                if (ci.charTex) {
                    renderer_->renderTexture(ci.charTex, dx, ty);
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
}

void WarField::handleKeyInput(Node::Key key) {
    Map::handleKeyInput(key);
}

bool WarField::tryMove(int x, int y, bool checkEvent) {
    return Map::tryMove(x, y, checkEvent);
}

void WarField::updateMainCharTexture() {
    Map::updateMainCharTexture();
}

}
