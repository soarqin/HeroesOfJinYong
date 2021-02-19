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
};

Map::Map(Renderer *renderer) : Node(renderer) {
    mapTextureMgr.setRenderer(renderer_);
    mapTextureMgr.setPalette(data::normalPalette.colors(), data::normalPalette.size());
    auto &mmapData = data::grpData["MMAP"];
    auto sz = mmapData.size();
    for (size_t i = 0; i < sz; ++i) {
        mapTextureMgr.loadFromRLE(i, mmapData[i]);
    }
    {
        auto &tex = mapTextureMgr[0];
        cellWidth_ = tex.width();
        cellHeight_ = tex.height();
    }

    width_ = MAP_WIDTH;
    height_ = MAP_HEIGHT;
    auto size = width_ * height_;
    earth_.resize(size);
    surface_.resize(size);
    building_.resize(size);
    buildx_.resize(size);
    buildy_.resize(size);
    util::File::getFileContent(core::config.dataFilePath("EARTH.002"), earth_);
    util::File::getFileContent(core::config.dataFilePath("SURFACE.002"), surface_);
    util::File::getFileContent(core::config.dataFilePath("BUILDING.002"), building_);
    util::File::getFileContent(core::config.dataFilePath("BUILDX.002"), buildx_);
    util::File::getFileContent(core::config.dataFilePath("BUILDY.002"), buildy_);
    for (auto &n: earth_) { n >>= 1; }
    for (auto &n: surface_) { n >>= 1; }
    for (auto &n: building_) { n >>= 1; }
}

void Map::render() {
    int cellDiffX = cellWidth_ / 2;
    int cellDiffY = cellHeight_ / 2;
    int width = width_;
    int cx = 320, cy = 50;
    for (int y = 0; y < 30; ++y) {
        int tx = cx, ty = cy;
        auto pos = y * width;
        for (int x = 0; x < 30; ++x) {
            tx += cellDiffX; ty += cellDiffY;
            renderer_->renderTexture(&scene::mapTextureMgr[earth_[pos]], tx, ty);
            auto idx = surface_[pos];
            if (idx) {
                renderer_->renderTexture(&scene::mapTextureMgr[idx], tx, ty);
            }
            idx = building_[pos];
            if (idx) {
                renderer_->renderTexture(&scene::mapTextureMgr[idx], tx, ty);
            }
            ++pos;
        }
        cx -= cellDiffX; cy += cellDiffY;
    }
}

}
