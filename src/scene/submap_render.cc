#include "submap.hh"

#include "colorpalette.hh"
#include "content/constants.hh"

#include <cstring>
#include <memory>

namespace hojy::scene {
namespace {

constexpr std::size_t subMapCellCount = static_cast<std::size_t>(
    ::hojy::content::SubMapWidth)
    * static_cast<std::size_t>(::hojy::content::SubMapHeight);

}

void SubMap::forceMainCharTexture(std::int16_t id) {
    mainCharSpriteId_ = id;
}

void SubMap::prepareRender() {
    if (!resourcesReady_ || cellInfo_.size() != subMapCellCount
        || cellWidth_ < 2 || cellHeight_ < 2) {
        return;
    }
    MapWithEvent::prepareRender();
    if (worldPresentationNeedsPrepare()) {
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int curX = currX_, curY = currY_;
        int camX = cameraX_, camY = cameraY_;
        int aheight = int(auxHeight_);
        int nx = int(auxWidth_) / 2 + cellWidth_ * 2;
        int ny = aheight / 2 + cellHeight_ * 2;
        int wcount = nx * 2 / cellWidth_;
        int hcount = (ny * 2 + 4 * cellHeight_) / cellDiffY;
        int cx, cy, tx, ty;
        int delta = -mapWidth_ + 1;

        std::unique_ptr<Texture> candidateTerrain(
            Texture::create(renderer_, auxWidth_, auxHeight_));
        std::unique_ptr<Texture> candidateOverlay(
            Texture::create(renderer_, auxWidth_, auxHeight_));
        if (!candidateTerrain || !candidateOverlay
            || !candidateTerrain->enableBlendMode(true)
            || !candidateOverlay->enableBlendMode(true)) {
            return;
        }
        int terrainPitch, overlayPitch;
        TextureLock terrainLock(candidateTerrain.get(), terrainPitch);
        TextureLock overlayLock(candidateOverlay.get(), overlayPitch);
        if (!terrainLock.valid() || !overlayLock.valid()) { return; }
        auto *terrainPixels = terrainLock.pixels();
        auto *overlayPixels = overlayLock.pixels();
        memset(terrainPixels, 0,
               terrainPitch * auxHeight_ * sizeof(std::uint32_t));
        memset(overlayPixels, 0,
               overlayPitch * auxHeight_ * sizeof(std::uint32_t));
        auto *pixels = terrainPixels;
        int pitch = terrainPitch;
        int nextCharHeight = charHeight_;
        const auto *colors = gNormalPalette.colors();
        bool renderOk = true;

        cx = (nx / cellDiffX + ny / cellDiffY) / 2;
        cy = (ny / cellDiffY - nx / cellDiffX) / 2;
        tx = int(auxWidth_) / 2 - (cx - cy) * cellDiffX;
        ty = int(auxHeight_) / 2 + cellDiffY - (cx + cy) * cellDiffY;
        cx = camX - cx; cy = camY - cy;
        const int texCount = static_cast<int>(texData_.size());
        for (int j = hcount; j; --j) {
            int x = cx, y = cy;
            int dx = tx;
            int offset = y * mapWidth_ + x;
            for (int i = wcount; i; --i, dx += cellWidth_,
                 offset += delta, ++x, --y) {
                if (x < 0 || x >= ::hojy::content::SubMapWidth
                    || y < 0 || y >= ::hojy::content::SubMapHeight) {
                    continue;
                }
                auto &ci = cellInfo_[offset];
                auto h = ci.buildingDeltaY;
                if (ci.earthId >= 0 && ci.earthId < texCount) {
                    renderOk = Texture::renderRLE(
                        texData_[ci.earthId], colors, pixels, pitch,
                        aheight, dx, ty) && renderOk;
                }
                if (ci.buildingId > 0 && ci.buildingId < texCount) {
                    renderOk = Texture::renderRLE(
                        texData_[ci.buildingId], colors, pixels, pitch,
                        aheight, dx, ty - h) && renderOk;
                }
                if (x == curX && y == curY) {
                    pixels = overlayPixels;
                    pitch = overlayPitch;
                    nextCharHeight = h;
                }
                if (ci.eventId > 0 && ci.eventId < texCount) {
                    renderOk = Texture::renderRLE(
                        texData_[ci.eventId], colors, pixels, pitch,
                        aheight, dx, ty - h) && renderOk;
                }
                if (ci.decorationId > 0 && ci.decorationId < texCount) {
                    renderOk = Texture::renderRLE(
                        texData_[ci.decorationId], colors, pixels, pitch,
                        aheight, dx, ty - ci.decorationDeltaY) && renderOk;
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
        if (!renderOk) { return; }
        overlayLock.unlock();
        terrainLock.unlock();
        delete drawingTerrainTex_;
        drawingTerrainTex_ = candidateTerrain.release();
        delete drawingTerrainTex2_;
        drawingTerrainTex2_ = candidateOverlay.release();
        charHeight_ = nextCharHeight;
        commitWorldPresentation();
    }
}

void SubMap::render() const {
    if (!resourcesReady_ || !renderer_ || !drawingTerrainTex_
        || !drawingTerrainTex2_) {
        return;
    }
    Map::render();
    renderer_->clear(0, 0, 0, 255);
    renderer_->renderTexture(drawingTerrainTex_, x_, y_, width_, height_,
                             0, 0, auxWidth_, auxHeight_);
    renderChar(charHeight_);
    renderer_->renderTexture(drawingTerrainTex2_, x_, y_, width_, height_,
                             0, 0, auxWidth_, auxHeight_);
    renderMiniPanel();
}

}
