#include "map.hh"

#include "texture.hh"
#include "data/colorpalette.hh"
#include "data/grpdata.hh"
#include "core/config.hh"
#include "util/File.hh"

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
    }
    int cellDiffX = cellWidth_ / 2;
    int cellDiffY = cellHeight_ / 2;
    texWidth_ = (mapWidth_ + mapHeight_) * cellDiffX;
    texHeight_ = (mapWidth_ + mapHeight_) * cellDiffY;
    texWCount_ = (texWidth_ + TEX_WIDTH_EACH - 1) / TEX_WIDTH_EACH;
    texHCount_ = (texHeight_ + TEX_WIDTH_EACH - 1) / TEX_WIDTH_EACH;
    textures_.resize(texWCount_ * texHCount_);
    for (auto *&tex : textures_) {
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
            renderer_->setTargetTexture(textures_[nc]);
            auto idx0 = earth_[pos];
            renderer_->renderTexture(&mapTextureMgr[idx0], cx, cy, true);
            auto idx1 = surface_[pos];
            if (idx1) {
                renderer_->renderTexture(&mapTextureMgr[idx1], cx, cy, true);
            }
            auto idx2 = building_[pos];
            if (idx2) {
                renderer_->renderTexture(&mapTextureMgr[idx2], cx, cy, true);
            }
            if (wext) {
                int vx = cx - TEX_WIDTH_EACH;
                renderer_->setTargetTexture(textures_[nc + 1]);
                renderer_->renderTexture(&mapTextureMgr[idx0], vx, cy, true);
                if (idx1) {
                    renderer_->renderTexture(&mapTextureMgr[idx1], vx, cy, true);
                }
                if (idx2) {
                    renderer_->renderTexture(&mapTextureMgr[idx2], vx, cy, true);
                }
                if (hext) {
                    int vy = cy - TEX_WIDTH_EACH;
                    renderer_->setTargetTexture(textures_[nc + texWCount_ + 1]);
                    renderer_->renderTexture(&mapTextureMgr[idx0], vx, vy, true);
                    if (idx1) {
                        renderer_->renderTexture(&mapTextureMgr[idx1], vx, vy, true);
                    }
                    if (idx2) {
                        renderer_->renderTexture(&mapTextureMgr[idx2], vx, vy, true);
                    }
                }
            }
            if (hext) {
                int vy = cy - TEX_WIDTH_EACH;
                renderer_->setTargetTexture(textures_[nc + texWCount_]);
                renderer_->renderTexture(&mapTextureMgr[idx0], cx, vy, true);
                if (idx1) {
                    renderer_->renderTexture(&mapTextureMgr[idx1], cx, vy, true);
                }
                if (idx2) {
                    renderer_->renderTexture(&mapTextureMgr[idx2], cx, vy, true);
                }
            }
            tx += cellDiffX; ty += cellDiffY;
            ++pos;
        }
        x -= cellDiffX; y += cellDiffY;
    }
    renderer_->setTargetTexture(nullptr);
}

Map::~Map() {
    for (auto *tex: textures_) {
        delete tex;
    }
    textures_.clear();
}

void Map::render() {
    int curX = 0, curY = mapHeight_ - 1;
    int cellDiffX = cellWidth_ / 2;
    int cellDiffY = cellHeight_ / 2;
    int x = (mapHeight_ + curX - curY) * cellDiffX - int(width_ / 2);
    int y = (curX + curY + 2) * cellDiffY - int(height_ / 2);
    int cx = x / TEX_WIDTH_EACH;
    int cy = y / TEX_WIDTH_EACH;
    x %= TEX_WIDTH_EACH;
    y %= TEX_WIDTH_EACH;
    auto idx = cx + cy * texWCount_;
    renderer_->renderTexture(textures_[idx], -x, -y);
    bool wext = x + width_ > TEX_WIDTH_EACH;
    bool hext = y + width_ > TEX_WIDTH_EACH;
    if (wext) {
        renderer_->renderTexture(textures_[idx + 1], TEX_WIDTH_EACH-x, -y);
        if (hext) {
            renderer_->renderTexture(textures_[idx + texWCount_ + 1], TEX_WIDTH_EACH-x, TEX_WIDTH_EACH-y);
        }
    }
    if (hext) {
        renderer_->renderTexture(textures_[idx + texWCount_], -x, TEX_WIDTH_EACH-y);
    }
    renderer_->renderTexture(&mapTextureMgr[2501], width_ / 2, height_ / 2);
    /*
    renderer_->renderTexture(textures_[1], 320 - TEX_WIDTH_EACH, 240 - TEX_WIDTH_EACH);
    renderer_->renderTexture(textures_[2], 320, 240 - TEX_WIDTH_EACH);
    renderer_->renderTexture(textures_[6], 320 - TEX_WIDTH_EACH, 240);
    renderer_->renderTexture(textures_[7], 320, 240);
     */
}

}
