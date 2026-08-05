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

#pragma once

#include "mapwithevent.hh"

#include <map>

namespace hojy::scene {

class GlobalMap final: public MapWithEvent {
    struct CellInfo {
        std::int16_t earthId, surfaceId, buildingId;
        int buildingDeltaY;
        bool canWalk;
        /* 0-land 1-water 2-wood */
        std::uint8_t type;
    };
public:
    GlobalMap(Renderer *renderer, int x, int y, int width, int height, std::pair<int, int> scale);
    ~GlobalMap() override;

    bool load(std::int32_t initialX = -1, std::int32_t initialY = -1);
    void update() override;
    void prepareRender() override;
    void render() const override;
    [[nodiscard]] bool onShip() const { return onShip_; }

protected:
    void showShip(bool show);
    bool tryMove(int x, int y, bool checkEvent) override;
    void updateMainCharSpriteId() override;
    void resetTime() override;
    bool checkTime() override;

private:
    struct MiniMapCell final {
        std::int32_t x = 0;
        std::int32_t y = 0;
        std::int16_t earthId = -1;
        bool blocked = false;
    };

    bool onShip_ = false;
    void prepareMiniMapRender();
    Texture *drawingTerrainTex2_ = nullptr;
    std::vector<std::uint16_t> building_, buildx_, buildy_;
    std::vector<CellInfo> cellInfo_;
    std::vector<MiniMapCell> miniMapCells_;
    TextureMgr cloudTexMgr_;
    int cloudStartX_[3] = {}, cloudStartY_[3] = {};
    int cloudX_[3] = {}, cloudY_[3] = {};
    std::int16_t cloudSpriteId_[3] = {-1, -1, -1};
    std::int16_t preparedCloudSpriteId_[3] = {-2, -2, -2};
    const Texture *preparedCloud_[3] = {};
    std::map<std::pair<std::int16_t, std::int16_t>, std::int16_t> subMapEntries_;
    std::uint64_t miniMapRevision_ = 1;
    std::uint64_t preparedMiniMapRevision_ = 0;
};

}
