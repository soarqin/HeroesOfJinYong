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

#include "data/grpdata.hh"
#include "data/warfielddata.hh"
#include "mem/savedata.hh"
#include <map>

namespace hojy::scene {

WarField::WarField(Renderer *renderer, int x, int y, int width, int height, float scale):
    Map(renderer, x, y, width, height, scale),
    drawingTerrainTex2_(Texture::createAsTarget(renderer_, width, height)) {
    drawingTerrainTex2_->enableBlendMode(true);
}

WarField::~WarField() {
    delete drawingTerrainTex2_;
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

void WarField::getDefaultChars(std::set<std::int16_t> &chars, std::set<std::int16_t> &autoChars) const {
    const auto *info = data::gWarFieldData.info(warId_);
    for (auto &id: info->ally) {
        if (id >= 0) { chars.insert(id); }
    }
    for (auto &id: info->autoAlly) {
        if (id >= 0) { autoChars.insert(id); }
    }
}

void WarField::putChars(const std::vector<std::int16_t> &chars) {
    const auto *info = data::gWarFieldData.info(warId_);
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
        if (ite == charMap.end()) {
            index = ite->second;
        } else {
            index = *indices.begin();
            indices.erase(indices.begin());
        }
        std::int16_t x = info->allyX[index], y = info->allyY[index];
        charQueue_.emplace_back(CharInfo {id, charInfo->speed, x, y, charInfo->hp, charInfo->mp,
                                          charInfo->stamina, 0});
    }
}

void WarField::render() {
    Map::render();
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
