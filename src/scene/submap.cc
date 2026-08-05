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

#include "logic/rle.hh"
#include "window.hh"
#include "window_command.hh"
#include "colorpalette.hh"
#include "content/constants.hh"
#include "content/grpdata.hh"
#include "world/savedata.hh"
#include "world/strings.hh"
#include <fmt/format.h>

#include <array>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

namespace {

constexpr std::size_t subMapCellCount = static_cast<std::size_t>(
    ::hojy::content::SubMapWidth) * static_cast<std::size_t>(::hojy::content::SubMapHeight);

bool readSubMapTextureHeader(const ::hojy::content::GrpData::DataSet &textures,
                             std::int32_t &cellWidth,
                             std::int32_t &cellHeight,
                             std::int32_t &offsetX,
                             std::int32_t &offsetY) {
    if (textures.empty() || textures.size() > static_cast<std::size_t>(
            std::numeric_limits<std::int16_t>::max())
        || textures.front().size() < sizeof(std::uint16_t) * 4) {
        return false;
    }
    std::array<std::uint16_t, 4> values{};
    std::memcpy(values.data(), textures.front().data(), sizeof(values));
    if (values[0] < 2 || values[1] < 2
        || values[0] / 2 == 0 || values[1] / 2 == 0) {
        return false;
    }
    cellWidth = values[0];
    cellHeight = values[1];
    offsetX = values[2];
    offsetY = values[3];
    return true;
}

bool validSubMapId(std::int16_t subMapId) {
    if (subMapId < 0) { return false; }
    const auto index = static_cast<std::size_t>(subMapId);
    const auto &save = ::hojy::world::state::gSaveData;
    return index < save.subMapInfo.size()
        && index < save.subMapLayerInfo.size()
        && index < save.subMapEventInfo.size()
        && save.subMapInfo[subMapId] != nullptr;
}

bool validSubMapTextureData(const ::hojy::content::GrpData::DataSet &textures) {
    for (const auto &texture: textures) {
        if (!texture.empty() && !::hojy::scene::logic::validateRleData(texture)) {
            return false;
        }
    }
    return true;
}

bool decodeRequiredTexture(std::int16_t encoded,
                           const ::hojy::content::GrpData::DataSet &textures,
                           std::int16_t &id) {
    if (encoded < 0) { return false; }
    const auto decoded = encoded >> 1;
    if (decoded < 0 || static_cast<std::size_t>(decoded) >= textures.size()
        || textures[static_cast<std::size_t>(decoded)].empty()
        || !::hojy::scene::logic::validateRleData(
            textures[static_cast<std::size_t>(decoded)])) {
        return false;
    }
    id = decoded;
    return true;
}

bool decodeOptionalTexture(std::int16_t encoded,
                           const ::hojy::content::GrpData::DataSet &textures,
                           std::int16_t &id) {
    std::int16_t decoded = -1;
    if (!::hojy::scene::logic::decodeOptionalSubMapAsset(
            encoded, textures.size(), decoded)) {
        return false;
    }
    if (decoded > 0 && (textures[static_cast<std::size_t>(decoded)].empty()
                        || !::hojy::scene::logic::validateRleData(
                            textures[static_cast<std::size_t>(decoded)]))) {
        return false;
    }
    id = decoded;
    return true;
}

}

namespace hojy::scene {

SubMap::SubMap(Renderer *renderer, int ix, int iy, int width, int height, std::pair<int, int> scale):
    MapWithEvent(renderer, ix, iy, width, height, scale),
    drawingTerrainTex2_(Texture::create(renderer_, auxWidth_, auxHeight_)) {
    if (!resourcesReady_ || !drawingTerrainTex2_
        || !drawingTerrainTex2_->enableBlendMode(true)) {
        resourcesReady_ = false;
    }
}

SubMap::~SubMap() {
    delete drawingTerrainTex2_;
}

bool SubMap::load(std::int16_t subMapId, bool clearPresentation) {
    if (!resourcesReady_ || !validSubMapId(subMapId)) { return false; }

    try {
        const int mapWidth = ::hojy::content::SubMapWidth;
        const int mapHeight = ::hojy::content::SubMapHeight;
        const auto size = subMapCellCount;
        const auto &layerInfo = ::hojy::world::state::gSaveData.subMapLayerInfo[
            static_cast<std::size_t>(subMapId)];
        const auto &eventInfo = ::hojy::world::state::gSaveData.subMapEventInfo[
            static_cast<std::size_t>(subMapId)];
        const auto &layers = layerInfo->data;
        const auto &events = eventInfo->events;

        auto candidateTexData = texData_;
        auto candidateLoaded = subMapLoaded_;
        if (candidateLoaded.find(subMapId) == candidateLoaded.end()) {
            ::hojy::content::GrpData::DataSet sharedData;
            if (::hojy::content::GrpData::loadData("SDX", "SMP", sharedData)) {
                candidateTexData = std::move(sharedData);
                candidateLoaded.clear();
                for (std::int16_t i = 0; i < 1000; ++i) {
                    candidateLoaded.insert(i);
                }
            } else {
                ::hojy::content::GrpData::DataSet specificData;
                if (!::hojy::content::GrpData::loadData(
                        fmt::format("SDX{:03}", subMapId),
                        fmt::format("SMP{:03}", subMapId), specificData)) {
                    return false;
                }
                if (specificData.size() > candidateTexData.size()) {
                    candidateTexData.resize(specificData.size());
                }
                for (std::size_t i = 0; i < specificData.size(); ++i) {
                    if (!specificData[i].empty() && candidateTexData[i].empty()) {
                        candidateTexData[i] = std::move(specificData[i]);
                    }
                }
                candidateLoaded.insert(subMapId);
            }
        }

        std::int32_t cellWidth = 0, cellHeight = 0, offsetX = 0, offsetY = 0;
        if (!readSubMapTextureHeader(
                candidateTexData, cellWidth, cellHeight, offsetX, offsetY)
            || !validSubMapTextureData(candidateTexData)) {
            return false;
        }
        const int cellDiffX = cellWidth / 2;
        const int cellDiffY = cellHeight / 2;
        if (cellDiffX <= 0 || cellDiffY <= 0) { return false; }

        std::vector<std::int32_t> candidateEventLoop(
            ::hojy::content::SubMapEventCount, 0);
        std::vector<std::int32_t> candidateEventDelay(
            ::hojy::content::SubMapEventCount, 0);
        std::vector<CellInfo> candidateCellInfo(size);
        std::array<logic::SubMapEventRecord, logic::SubMapEventCount> candidateEvents{};
        for (std::size_t eventIndex = 0; eventIndex < candidateEvents.size(); ++eventIndex) {
            const auto &event = events[eventIndex];
            candidateEvents[eventIndex] = logic::SubMapEventRecord{
                event.blocked, event.index, event.currTex, event.endTex,
                event.begTex, event.texDelay, event.x, event.y,
            };
        }
        const auto validEventTexture = [&candidateTexData](std::int16_t value) {
            if (value < 0) { return value == -1; }
            const auto id = static_cast<std::size_t>(value >> 1);
            return id < candidateTexData.size()
                && !candidateTexData[id].empty()
                && logic::validateRleData(candidateTexData[id]);
        };
        std::vector<std::int16_t> candidateEventLayer(
            layers[3], layers[3] + static_cast<std::ptrdiff_t>(size));
        logic::SubMapStateSnapshot candidateSnapshot;
        if (!logic::buildSubMapStateSnapshot(
                candidateEvents, candidateEventLayer, mapWidth, mapHeight,
                candidateTexData.size(), validEventTexture,
                candidateSnapshot)) {
            return false;
        }
        const auto candidateMapName = GETSUBMAPNAME(subMapId);

        int x = (mapHeight - 1) * cellDiffX + offsetX;
        int y = offsetY;
        int pos = 0;
        for (int j = mapHeight; j; --j) {
            int tx = x, ty = y;
            for (int i = mapWidth; i; --i, ++pos, tx += cellDiffX, ty += cellDiffY) {
                (void)tx;
                (void)ty;
                auto &ci = candidateCellInfo[static_cast<std::size_t>(pos)];
                if (!decodeRequiredTexture(layers[0][pos], candidateTexData, ci.earthId)
                    || !decodeOptionalTexture(layers[1][pos], candidateTexData, ci.buildingId)
                    || !decodeOptionalTexture(layers[2][pos], candidateTexData, ci.decorationId)) {
                    return false;
                }
                ci.blocked = ci.earthId >= 179 && ci.earthId <= 181
                    || ci.earthId == 261 || ci.earthId == 511
                    || ci.earthId >= 662 && ci.earthId <= 665 || ci.earthId == 674;
                if (ci.buildingId >= 0
                    && candidateTexData[static_cast<std::size_t>(ci.buildingId)].empty()) {
                    ci.blocked = true;
                }
                ci.eventId = candidateSnapshot.eventAssetIds[
                    static_cast<std::size_t>(pos)];
                ci.buildingDeltaY = layers[4][pos];
                ci.decorationDeltaY = layers[5][pos];
            }
            x -= cellDiffX;
            y += cellDiffY;
        }

        if (clearPresentation) {
            cleanupEvents();
        }
        mapWidth_ = mapWidth;
        mapHeight_ = mapHeight;
        cellWidth_ = cellWidth;
        cellHeight_ = cellHeight;
        offsetX_ = offsetX;
        offsetY_ = offsetY;
        texData_ = std::move(candidateTexData);
        subMapLoaded_ = std::move(candidateLoaded);
        eventLoop_ = std::move(candidateEventLoop);
        eventDelay_ = std::move(candidateEventDelay);
        activeEvents_ = candidateSnapshot.activeEvents;
        cellInfo_ = std::move(candidateCellInfo);
        subMapId_ = subMapId;
        markWorldChanged();
        commitMiniPanelSnapshot(std::move(candidateMapName), currX_, currY_);
        resetFrame();
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

bool SubMap::unload(bool clearPresentation) {
    if (clearPresentation) {
        cleanupEvents();
    }
    subMapId_ = -1;
    mapWidth_ = 0;
    mapHeight_ = 0;
    cellWidth_ = 0;
    cellHeight_ = 0;
    offsetX_ = 0;
    offsetY_ = 0;
    cameraX_ = 0;
    cameraY_ = 0;
    charHeight_ = 0;
    cellInfo_.clear();
    eventLoop_.clear();
    eventDelay_.clear();
    activeEvents_.fill(false);
    markWorldChanged();
    commitMiniPanelSnapshot({}, 0, 0);
    return true;
}

bool SubMap::tryMove(int x, int y, bool checkEvent) {
    if (!resourcesReady_ || !validSubMapId(subMapId_)
        || x < 0 || x >= mapWidth_ || y < 0 || y >= mapHeight_
        || cellInfo_.size() != subMapCellCount) {
        return false;
    }
    auto pos = y * mapWidth_ + x;
    auto &ci = cellInfo_[pos];
    if (ci.buildingId > 0 || ci.blocked) {
        return true;
    }
    auto &layers = ::hojy::world::state::gSaveData.subMapLayerInfo[subMapId_]->data;
    auto &events = ::hojy::world::state::gSaveData.subMapEventInfo[subMapId_]->events;
    auto ev = layers[3][pos];
    if (ev >= 0 && (ev >= ::hojy::content::SubMapEventCount
                    || !activeEvents_[static_cast<std::size_t>(ev)]
                    || events[ev].blocked)) {
        return true;
    }
    const auto oldX = currX_;
    const auto oldY = currY_;
    currX_ = x;
    currY_ = y;
    cameraX_ = x;
    cameraY_ = y;
    markWorldChanged();
    if (oldX != currX_ || oldY != currY_) {
        markMiniPanelChanged();
    }
    currMainCharFrame_ = currMainCharFrame_ % 6 + 1;
    if (checkEvent) {
        onMove();
    }
    const auto &subMapInfo = ::hojy::world::state::gSaveData.subMapInfo[subMapId_];
    for (int i = 0; i < 3; ++i) {
        if (subMapInfo->exitX[i] == currX_ && subMapInfo->exitY[i] == currY_) {
            const auto direction = int(direction_);
            postSceneCommand(this, [direction](SceneCommandContext &context) { context.exitToGlobalMap(direction); });
            if (subMapInfo->exitMusic >= 0) {
                const auto music = subMapInfo->exitMusic;
                postSceneCommand(this, [music](SceneCommandContext &context) { context.playMusic(music); });
            }
            return true;
        }
    }
    if (subMapInfo->switchSubMap >= 0 && subMapInfo->switchSubMapX == currX_ && subMapInfo->switchSubMapY == currY_) {
        const auto subMapId = subMapInfo->switchSubMap;
        if (!validSubMapId(subMapId)) { return true; }
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
    return true;
}

void SubMap::updateMainCharSpriteId() {
    if (animEventId_[0] < 0) {
        mainCharSpriteId_ = animCurrTex_[0] >> 1;
        return;
    }
    if (resting_) {
        mainCharSpriteId_ = 2501 + int(direction_) * 7;
        return;
    }
    mainCharSpriteId_ = 2501 + int(direction_) * 7 + currMainCharFrame_;
}

void SubMap::setCellSpriteId(int x, int y, int layer, std::int16_t spriteId) {
    if (!resourcesReady_ || x < 0 || x >= mapWidth_ || y < 0 || y >= mapHeight_
        || cellInfo_.size() != subMapCellCount || spriteId < -1
        || (spriteId >= 0 && static_cast<std::size_t>(spriteId) >= texData_.size())) {
        return;
    }
    switch (layer) {
    case 0:
        cellInfo_[y * mapWidth_ + x].earthId = spriteId;
        break;
    case 1:
        cellInfo_[y * mapWidth_ + x].buildingId = spriteId;
        break;
    case 2:
        cellInfo_[y * mapWidth_ + x].decorationId = spriteId;
        break;
    case 3:
        cellInfo_[y * mapWidth_ + x].eventId = spriteId;
        break;
    default:
        return;
    }
    markWorldChanged();
}

void SubMap::synchronizeCommittedSubMapState(
        std::int16_t subMapId,
        const logic::SubMapStateSnapshot &snapshot) noexcept {
    if (subMapId != subMapId_
        || snapshot.eventAssetIds.size() != cellInfo_.size()) {
        return;
    }
    activeEvents_ = snapshot.activeEvents;
    for (std::size_t pos = 0; pos < cellInfo_.size(); ++pos) {
        cellInfo_[pos].eventId = snapshot.eventAssetIds[pos];
    }
    markWorldChanged();
}

void SubMap::frameUpdate() {
    if (!resourcesReady_ || !validSubMapId(subMapId_)
        || eventLoop_.size() != ::hojy::content::SubMapEventCount
        || eventDelay_.size() != ::hojy::content::SubMapEventCount
        || cellInfo_.size() != subMapCellCount) {
        return;
    }
    MapWithEvent::frameUpdate();
    auto &evlist = ::hojy::world::state::gSaveData.subMapEventInfo[subMapId_];
    for (std::size_t index = 0;
         index < static_cast<std::size_t>(::hojy::content::SubMapEventCount);
         ++index) {
        if (index >= activeEvents_.size() || !activeEvents_[index]) { continue; }
        auto &ev = evlist->events[index];
        if (ev.index < 0
            || static_cast<std::size_t>(ev.index) >= eventLoop_.size()) {
            continue;
        }
        const auto clockSlot = static_cast<std::size_t>(ev.index);
        const auto previousTex = ev.currTex;
        logic::SubMapAnimationState state{
            ev.currTex, eventLoop_[clockSlot], eventDelay_[clockSlot],
        };
        if (!logic::advanceSubMapAnimation(
                state, ev.begTex, ev.endTex, ev.texDelay)) {
            eventLoop_[clockSlot] = state.loop;
            eventDelay_[clockSlot] = state.delay;
            continue;
        }
        ev.currTex = static_cast<std::int16_t>(state.current);
        eventLoop_[clockSlot] = state.loop;
        eventDelay_[clockSlot] = state.delay;
        if (ev.currTex == previousTex) { continue; }
        auto &ci = cellInfo_[ev.y * mapWidth_ + ev.x];
        ci.eventId = ev.currTex < 0 ? -1 : ev.currTex >> 1;
        markWorldChanged();
    }
}

}
