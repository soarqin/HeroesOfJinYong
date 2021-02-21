#include "map.hh"

#include "texture.hh"
#include "data/colorpalette.hh"
#include "data/grpdata.hh"
#include "core/config.hh"
#include "util/file.hh"

#include <algorithm>
#include <chrono>

namespace hojy::scene {

enum {
    MAP_WIDTH = 480,
    MAP_HEIGHT = 480,
};

Map::Map(Renderer *renderer, std::uint32_t width, std::uint32_t height): Node(renderer, width, height) {
    mapTextureMgr.setRenderer(renderer_);
    mapTextureMgr.setPalette(data::normalPalette.colors(), data::normalPalette.size());
    auto &mmapData = data::grpData["MMAP"];
    auto sz = mmapData.size();
    for (size_t i = 0; i < sz; ++i) {
        mapTextureMgr.loadFromRLE(i, mmapData[i]);
    }
    mapWidth_ = MAP_WIDTH;
    mapHeight_ = MAP_HEIGHT;
    {
        auto &tex = mapTextureMgr[0];
        cellWidth_ = tex.width();
        cellHeight_ = tex.height();
        offsetX_ = tex.originX();
        offsetY_ = tex.originY();
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
    for (int j = mapWidth_; j; --j) {
        int tx = x, ty = y;
        for (int i = mapHeight_; i; --i, ++pos, tx += cellDiffX, ty += cellDiffY) {
            auto &ci = cellInfo_[pos];
            auto &n = earth_[pos];
            ci.x = tx;
            ci.y = ty;
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
            ci.earth = &mapTextureMgr[n];
            auto &n0 = surface_[pos];
            n0 >>= 1;
            if (n0) {
                ci.surface = &mapTextureMgr[n0];
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
                const auto *tex2 = &mapTextureMgr[n1];
                auto centerX = tx - tex2->originX() + tex2->width() / 2;
                auto centerY = ty - tex2->originY() + (tex2->height() < 36 ? tex2->height() * 4 / 5 : tex2->height() * 3 / 4);
                buildingTex_.emplace_back(BuildingTex { centerY * texWidth_ + centerX, tx, ty, tex2 });
            }
        }
        x -= cellDiffX; y += cellDiffY;
    }

    std::sort(buildingTex_.begin(), buildingTex_.end(), BuildingTexComp());
    drawingTerrainTex_ = Texture::createAsTarget(renderer_, 2048, 2048);
    drawingTerrainTex_->enableBlendMode(true);
    drawingBuildingTex_[0] = Texture::createAsTarget(renderer_, 2048, 1024);
    drawingBuildingTex_[0]->enableBlendMode(true);
    drawingBuildingTex_[1] = Texture::createAsTarget(renderer_, 2048, 1024);
    drawingBuildingTex_[1]->enableBlendMode(true);
    currX_ = 242, currY_ = 294;
    moveDirty_ = true;
    resetTime();
    updateMainCharTexture();
}

Map::~Map() {
    delete drawingTerrainTex_;
    delete drawingBuildingTex_[0];
    delete drawingBuildingTex_[1];
}

void Map::render() {
    checkTime();
    if (moveDirty_) {
        moveDirty_ = false;
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int curX = currX_, curY = currY_;
        int nx = int(width_) / 2 + cellWidth_ * 2;
        int ny = int(height_) / 2 + cellHeight_ * 2;
        int cx = (nx / cellDiffX + ny / cellDiffY) / 2;
        int cy = (ny / cellDiffY - nx / cellDiffX) / 2;
        int wcount = nx * 2 / cellWidth_;
        int hcount = ny * 2 / cellDiffY;
        int tx = int(width_) / 2 - (cx - cy) * cellDiffX;
        int ty = int(height_) / 2 - (cx + cy) * cellDiffY;
        cx = curX - cx; cy = curY - cy;
        renderer_->setTargetTexture(drawingTerrainTex_);
        renderer_->setClipRect(0, 0, 2048, 2048);
        int delta = -mapWidth_ + 1;
        for (int j = hcount; j; --j) {
            int x = cx, y = cy;
            int dx = tx;
            int offset = y * mapWidth_ + x;
            for (int i = wcount; i && y; --i, dx += cellWidth_, offset += delta, ++x, --y) {
                auto &ci = cellInfo_[offset];
                renderer_->renderTexture(ci.earth, dx, ty);
                if (ci.surface) {
                    renderer_->renderTexture(ci.surface, dx, ty);
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

        int ox = (mapHeight_ + curX - curY - 1) * cellDiffX + offsetX_ - int(width_ / 2);
        int oy = (curX + curY) * cellDiffY + offsetY_ - int(height_ / 2);
        int myy = int(height_) / 2 + oy;
        int l = ox - 128, t = oy - 128, r = ox + int(width_) + 128, b = oy + int(height_) + 128;
        auto ite = std::lower_bound(buildingTex_.begin(), buildingTex_.end(), BuildingTex {t * texWidth_ + l, 0, 0, nullptr}, BuildingTexComp());
        auto ite_mid = std::upper_bound(buildingTex_.begin(), buildingTex_.end(), BuildingTex {myy * texWidth_, 0, 0, nullptr}, BuildingTexComp());
        auto ite_end = std::upper_bound(buildingTex_.begin(), buildingTex_.end(), BuildingTex {b * texWidth_ + r, 0, 0, nullptr}, BuildingTexComp());
        renderer_->setTargetTexture(drawingBuildingTex_[0]);
        renderer_->setClipRect(0, 0, 2048, 1024);
        renderer_->fill(0, 0, 0, 0);
        while (ite != ite_mid) {
            if (ite->x < l || ite->x >= r) {
                ++ite;
                continue;
            }
            renderer_->renderTexture(ite->tex, ite->x - ox, ite->y - oy);
            ++ite;
        }
        renderer_->setTargetTexture(drawingBuildingTex_[1]);
        renderer_->setClipRect(0, 0, 2048, 1024);
        renderer_->fill(0, 0, 0, 0);
        while (ite != ite_end) {
            if (ite->x < l || ite->x >= r) {
                ++ite;
                continue;
            }
            renderer_->renderTexture(ite->tex, ite->x - ox, ite->y - oy);
            ++ite;
        }
        renderer_->setTargetTexture(nullptr);
        renderer_->unsetClipRect();
    }
    renderer_->fill(0, 0, 0, 0);
    renderer_->renderTexture(drawingTerrainTex_, 0, 0, width_, height_);
    renderer_->renderTexture(drawingBuildingTex_[0], 0, 0, width_, height_);
    renderer_->renderTexture(mainCharTex_, int(width_) / 2, int(height_) / 2);
    renderer_->renderTexture(drawingBuildingTex_[1], 0, 0, width_, height_);
}

void Map::setPosition(int x, int y) {
    currX_ = x;
    currY_ = y;
    currFrame_ = 0;
    resting_ = false;
    moveDirty_ = true;
    auto offset = y * mapWidth_ + x;
    onShip_ = cellInfo_[offset].type == 1;
    resetTime();
    updateMainCharTexture();
}

void Map::move(Map::Direction direction) {
    int x = currX_, y = currY_;
    direction_ = direction;
    switch (direction) {
    case DirUp:
        if (y > 0) { --y; }
        break;
    case DirRight:
        if (x < mapWidth_ - 1) { ++x; }
        break;
    case DirLeft:
        if (x > 0) { --x; }
        break;
    case DirDown:
        if (y < mapHeight_ - 1) { ++y; }
        break;
    }
    auto offset = y * mapWidth_ + x;
    if (!cellInfo_[offset].canWalk || buildx_[offset] != 0 && building_[buildy_[offset] * mapWidth_ + buildx_[offset]] != 0) {
        resetTime();
        updateMainCharTexture();
        return;
    }
    currX_ = x;
    currY_ = y;
    moveDirty_ = true;
    onShip_ = cellInfo_[offset].type == 1;
    if (onShip_) {
        currFrame_ = (currFrame_ + 1) % 4;
    } else {
        currFrame_ = currFrame_ % 6 + 1;
    }
    resetTime();
    updateMainCharTexture();
}

void Map::updateMainCharTexture() {
    if (onShip_) {
        mainCharTex_ = &mapTextureMgr[3715 + int(direction_) * 4 + currFrame_];
        return;
    }
    if (resting_) {
        mainCharTex_ = &mapTextureMgr[2529 + int(direction_) * 6 + currFrame_];
        return;
    }
    mainCharTex_ = &mapTextureMgr[2501 + int(direction_) * 7 + currFrame_];
}

void Map::resetTime() {
    if (onShip_) { return; }
    resting_ = false;
    nextTime_ = std::chrono::steady_clock::now() + std::chrono::seconds(currFrame_ > 0 ? 2 : 5);
}

void Map::checkTime() {
    if (onShip_) { return; }
    if (resting_) {
        if (std::chrono::steady_clock::now() < nextTime_) {
            return;
        }
        currFrame_ = (currFrame_ + 1) % 6;
        nextTime_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
        updateMainCharTexture();
        return;
    }
    if (std::chrono::steady_clock::now() < nextTime_) {
        return;
    }
    if (currFrame_ > 0) {
        currFrame_ = 0;
        nextTime_ = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    } else {
        currFrame_ = 0;
        resting_ = true;
        nextTime_ = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    }
    updateMainCharTexture();
}

}
