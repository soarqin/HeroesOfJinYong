#include "map.hh"

#include "texture.hh"
#include "data/colorpalette.hh"
#include "data/grpdata.hh"
#include "core/config.hh"
#include "util/file.hh"

#include <algorithm>

namespace hojy::scene {

enum {
    MAP_WIDTH = 480,
    MAP_HEIGHT = 480,
    TEX_WIDTH_EACH = 4096,
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
    texWCount_ = (texWidth_ + TEX_WIDTH_EACH - 1) / TEX_WIDTH_EACH;
    texHCount_ = (texHeight_ + TEX_WIDTH_EACH - 1) / TEX_WIDTH_EACH;
    terrainTex_.resize(texWCount_ * texHCount_);
    for (auto *&tex : terrainTex_) {
        tex = Texture::createAsTarget(renderer_, TEX_WIDTH_EACH, TEX_WIDTH_EACH);
    }

    auto size = mapWidth_ * mapHeight_;
    util::File::getFileContent(core::config.dataFilePath("EARTH.002"), earth_);
    util::File::getFileContent(core::config.dataFilePath("SURFACE.002"), surface_);
    util::File::getFileContent(core::config.dataFilePath("BUILDING.002"), building_);
    util::File::getFileContent(core::config.dataFilePath("BUILDX.002"), buildx_);
    util::File::getFileContent(core::config.dataFilePath("BUILDY.002"), buildy_);
    for (auto &n: earth_) { n >>= 1; }
    for (auto &n: surface_) { n >>= 1; }
    for (auto &n: building_) { n >>= 1; }
    earth_.resize(size);
    surface_.resize(size);
    building_.resize(size);
    buildx_.resize(size);
    buildy_.resize(size);

    renderer_->unsetClipRect();
    int x = (mapHeight_ - 1) * cellDiffX;
    int y = 0;
    int pos = 0;
    for (int j = 0; j < mapWidth_; ++j) {
        int tx = x, ty = y;
        for (int i = 0; i < mapHeight_; ++i) {
            int nx = tx / TEX_WIDTH_EACH;
            int ny = ty / TEX_WIDTH_EACH;
            int nc = nx + ny * texWCount_;
            int cx = tx % TEX_WIDTH_EACH;
            int cy = ty % TEX_WIDTH_EACH;
            bool wext = cx + cellWidth_ > TEX_WIDTH_EACH;
            bool hext = cy + cellHeight_ > TEX_WIDTH_EACH;
            renderer_->setTargetTexture(terrainTex_[nc]);
            auto idx0 = earth_[pos];
            const auto *tex0 = &mapTextureMgr[idx0];
            const Texture *tex1 = nullptr;
            renderer_->renderTexture(tex0, cx, cy, true);
            auto idx1 = surface_[pos];
            if (idx1) {
                tex1 = &mapTextureMgr[idx1];
                renderer_->renderTexture(tex1, cx, cy, true);
            }
            if (wext) {
                int vx = cx - TEX_WIDTH_EACH;
                renderer_->setTargetTexture(terrainTex_[nc + 1]);
                renderer_->renderTexture(tex0, vx, cy, true);
                if (idx1) {
                    renderer_->renderTexture(tex1, vx, cy, true);
                }
                if (hext) {
                    int vy = cy - TEX_WIDTH_EACH;
                    renderer_->setTargetTexture(terrainTex_[nc + texWCount_ + 1]);
                    renderer_->renderTexture(tex0, vx, vy, true);
                    if (idx1) {
                        renderer_->renderTexture(tex1, vx, vy, true);
                    }
                }
            }
            if (hext) {
                int vy = cy - TEX_WIDTH_EACH;
                renderer_->setTargetTexture(terrainTex_[nc + texWCount_]);
                renderer_->renderTexture(tex0, cx, vy, true);
                if (idx1) {
                    renderer_->renderTexture(tex1, cx, vy, true);
                }
            }
            tx += cellDiffX; ty += cellDiffY;
            ++pos;
        }
        x -= cellDiffX; y += cellDiffY;
    }
    x = (mapHeight_ - 1) * cellDiffX + offsetX_;
    y = offsetY_;
    pos = 0;
    for (int j = 0; j < mapWidth_; ++j) {
        int tx = x, ty = y;
        for (int i = 0; i < mapHeight_; ++i, ++pos, tx += cellDiffX, ty += cellDiffY) {
            auto idx2 = building_[pos];
            if (!idx2) {
                continue;
            }
            const auto *tex2 = &mapTextureMgr[idx2];
            auto centerX = tx - tex2->originX() + tex2->width() / 2;
            auto centerY = ty - tex2->originY() + tex2->height() * 3 / 4;
            buildingTex_.emplace_back(BuildingTex { centerY * texWidth_ + centerX, tx, ty, tex2 });
        }
        x -= cellDiffX; y += cellDiffY;
    }
    renderer_->setTargetTexture(nullptr);
    std::sort(buildingTex_.begin(), buildingTex_.end(), BuildingTexComp());
    drawingBuildingTex_[0] = Texture::createAsTarget(renderer_, TEX_WIDTH_EACH, TEX_WIDTH_EACH);
    drawingBuildingTex_[0]->enableBlendMode(true);
    drawingBuildingTex_[1] = Texture::createAsTarget(renderer_, TEX_WIDTH_EACH, TEX_WIDTH_EACH);
    drawingBuildingTex_[1]->enableBlendMode(true);
    currX_ = 242, currY_ = 294;
    moveDirty_ = true;
}

Map::~Map() {
    for (auto *tex: terrainTex_) {
        delete tex;
    }
    terrainTex_.clear();
}

void Map::render() {
    int curX = currX_, curY = currY_;
    int cellDiffX = cellWidth_ / 2;
    int cellDiffY = cellHeight_ / 2;
    int ox = (mapHeight_ + curX - curY - 1) * cellDiffX + offsetX_ - int(width_ / 2);
    int oy = (curX + curY) * cellDiffY + offsetY_ - int(height_ / 2);
    int cx = ox / TEX_WIDTH_EACH;
    int cy = oy / TEX_WIDTH_EACH;
    int x = ox % TEX_WIDTH_EACH;
    int y = oy % TEX_WIDTH_EACH;
    auto idx = cx + cy * texWCount_;
    if (moveDirty_) {
        moveDirty_ = false;
        int myx = int(width_) / 2 + ox, myy = int(height_) / 2 + oy;
        int l = ox - 128, t = oy - 128, r = ox + int(width_) + 128, b = oy + int(height_) + 128;
        auto ite = std::lower_bound(buildingTex_.begin(), buildingTex_.end(), BuildingTex {t * texWidth_ + l, 0, 0, nullptr}, BuildingTexComp());
        auto ite_mid = std::upper_bound(buildingTex_.begin(), buildingTex_.end(), BuildingTex {myy * texWidth_, 0, 0, nullptr}, BuildingTexComp());
        auto ite_end = std::upper_bound(buildingTex_.begin(), buildingTex_.end(), BuildingTex {b * texWidth_ + r, 0, 0, nullptr}, BuildingTexComp());
        renderer_->setTargetTexture(drawingBuildingTex_[0]);
        renderer_->setClipRect(0, 0, TEX_WIDTH_EACH, TEX_WIDTH_EACH);
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
        renderer_->setClipRect(0, 0, TEX_WIDTH_EACH, TEX_WIDTH_EACH);
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
    }
    renderer_->setClipRect(0, 0, width_, height_);
    renderer_->renderTexture(terrainTex_[idx], -x, -y);
    bool wext = x + width_ > TEX_WIDTH_EACH;
    bool hext = y + height_ > TEX_WIDTH_EACH;
    if (wext) {
        renderer_->renderTexture(terrainTex_[idx + 1], TEX_WIDTH_EACH-x, -y);
        if (hext) {
            renderer_->renderTexture(terrainTex_[idx + texWCount_ + 1], TEX_WIDTH_EACH-x, TEX_WIDTH_EACH-y);
        }
    }
    if (hext) {
        renderer_->renderTexture(terrainTex_[idx + texWCount_], -x, TEX_WIDTH_EACH-y);
    }
    renderer_->renderTexture(drawingBuildingTex_[0], 0, 0);
    renderer_->renderTexture(&mapTextureMgr[2501 + int(direction_) * 7], int(width_) / 2, int(height_) / 2);
    renderer_->renderTexture(drawingBuildingTex_[1], 0, 0);
}

void Map::setPosition(int x, int y) {
    currX_ = x;
    currY_ = y;
    moveDirty_ = true;
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
    if (buildx_[offset] != 0 && building_[buildy_[offset] * mapWidth_ + buildx_[offset]] != 0) {
        return;
    }
    currX_ = x;
    currY_ = y;
    moveDirty_ = true;
}

}
