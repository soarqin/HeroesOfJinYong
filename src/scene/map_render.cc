#include "mapwithevent.hh"

namespace hojy::scene {

void MapWithEvent::prepareRender() {
    Map::prepareRender();
    const Texture *candidate = nullptr;
    if (mainCharSpriteId_ >= 0) {
        candidate = getOrLoadTexture(mainCharSpriteId_);
    }
    // Resolve on every prepare pass.  Texture managers can be cleared and
    // rebuilt without changing the logical SpriteId; retaining the old
    // pointer in that case would leave the render view dangling.
    preparedMainCharTex_ = candidate;
    preparedMainCharSpriteId_ = mainCharSpriteId_;
}

void MapWithEvent::renderChar(int deltaY) const {
    if (!showChar_ || !preparedMainCharTex_) { return; }
    int dx = currX_ - cameraX_;
    int dy = currY_ - cameraY_;
    int cellDiffY = cellHeight_ / 2;
    int offsetX = (dx - dy) * cellWidth_ / 2;
    int offsetY = (dx + dy) * cellDiffY;
    renderer_->renderTexture(preparedMainCharTex_,
                             x_ + (width_ >> 1) + offsetX * scale_.first / scale_.second,
                             y_ + (height_ >> 1)
                                 + (offsetY + cellDiffY - deltaY) * scale_.first / scale_.second,
                             scale_);
}

}
