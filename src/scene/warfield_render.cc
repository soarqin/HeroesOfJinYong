#include "warfield.hh"

#include "colorpalette.hh"
#include "effect.hh"
#include "node_helpers.hh"
#include "statusview.hh"
#include "warfield_load.hh"
#include "content/constants.hh"
#include "core/config.hh"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>

namespace hojy::scene {
void Warfield::prepareRender() {
    if (presentationTextureResetRequested_) {
        textureMgr_.clear();
        presentationTextureResetRequested_ = false;
    }
    if (presentationCleanupRequested_) {
        removeAllChildren();
        fadeNode_ = nullptr;
        fadePostAction_ = nullptr;
        runFadePostAction_ = false;
        presentationCleanupRequested_ = false;
    }
    if (statusPanelReleaseRequested_) {
        delete statusPanel_;
        statusPanel_ = nullptr;
        statusPanelReleaseRequested_ = false;
        renderedStatusSnapshotRevision_ = 0;
    }
    if (pendingFinishMessages_) {
        auto request = std::move(*pendingFinishMessages_);
        pendingFinishMessages_.reset();
        presentFinishMessages(std::move(request));
    }
    if (!resourcesReady_ || cellInfo_.empty() || cellWidth_ < 2 || cellHeight_ < 2) {
        return;
    }
    Map::prepareRender();
    if ((stage_ == Idle || stage_ == PlayerMenu || stage_ == Moving)
        && !statusPanel_ && renderer_) {
        auto candidate = std::make_unique<StatusView>(
            renderer_, x_, y_, width_, height_);
        candidate->setHeadTextureProvider(headTextureProvider_);
        statusPanel_ = candidate.release();
    }
    if ((stage_ == Idle || stage_ == PlayerMenu || stage_ == Moving)
        && statusPanel_ && statusSnapshot_
        && renderedStatusSnapshotRevision_ != statusSnapshotRevision_) {
        auto *status = dynamic_cast<StatusView *>(statusPanel_);
        if (status) {
            status->show(*statusSnapshot_);
            status->setBattleAnchor(
                currentActor_ && currentActor_->side == 1,
                width_, height_,
                core::config.windowBorder());
            renderedStatusSnapshotRevision_ = statusSnapshotRevision_;
        }
    }
    bool acting = stage_ == Acting;
    if (acting && effectTexIdx_ >= 3) {
        const auto fontSize = 12 * scale_.first / scale_.second;
        for (const auto &number: popupNumbers_) {
            if (!renderer_->ttf()->prepareText(number.str, fontSize)) {
                return;
            }
        }
    }
    if (worldPresentationNeedsPrepare()) {
        std::vector<const std::string *> effectOverlay(cellInfo_.size(), nullptr);
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int curX = cameraX_, curY = cameraY_;
        auto aheight = int(auxHeight_);
        int nx = int(auxWidth_) / 2 + cellWidth_ * 2;
        int ny = aheight / 2 + cellHeight_ * 2;
        int wcount = nx * 2 / cellWidth_;
        int hcount = (ny * 2 + 4 * cellHeight_) / cellDiffY;
        int cx, cy, tx, ty;
        int delta = -mapWidth_ + 1;

        cx = (nx / cellDiffX + ny / cellDiffY) / 2;
        cy = (ny / cellDiffY - nx / cellDiffX) / 2;
        tx = int(auxWidth_) / 2 - (cx - cy) * cellDiffX;
        ty = int(auxHeight_) / 2 + cellDiffY - (cx + cy) * cellDiffY;
        cx = curX - cx; cy = curY - cy;
        bool selecting = stage_ == MoveSelecting || stage_ == AttackSelecting;
        bool movingOrActing = acting || stage_ == Moving;
        auto *ch = currentActor_;
        if (acting && effectOverlaySnapshot_.frameIndex >= 0) {
            const auto &effectFrames = gEffect[
                static_cast<std::int16_t>(effectOverlaySnapshot_.effectAssetId)];
            if (!effectFrames.empty()) {
                const auto frame = static_cast<std::size_t>(
                    effectOverlaySnapshot_.frameIndex);
                const auto *texture = frame < effectFrames.size()
                    ? &effectFrames[frame] : &effectFrames.back();
                for (const auto index: effectOverlaySnapshot_.cellIndices) {
                    if (index < effectOverlay.size()) {
                        effectOverlay[index] = texture;
                    }
                }
            }
        }
        const auto *colors = gNormalPalette.colors();
        auto candidateTerrain = std::unique_ptr<Texture>(Texture::create(renderer_, auxWidth_, auxHeight_));
        auto candidateOverlay = std::unique_ptr<Texture>(Texture::create(renderer_, auxWidth_, auxHeight_));
        if (!candidateTerrain || !candidateOverlay
            || !candidateTerrain->enableBlendMode(true)
            || !candidateOverlay->enableBlendMode(true)) {
            return;
        }
        int pitch, pitch2;
        TextureLock terrainLock(candidateTerrain.get(), pitch);
        TextureLock overlayLock(candidateOverlay.get(), pitch2);
        if (!terrainLock.valid() || !overlayLock.valid()) { return; }
        std::uint32_t *pixels = terrainLock.pixels();
        std::uint32_t *pixels2 = overlayLock.pixels();
        memset(pixels, 0, pitch * auxHeight_ * sizeof(std::uint32_t));
        memset(pixels2, 0, pitch2 * auxHeight_ * sizeof(std::uint32_t));
        bool renderOk = true;
        std::array<std::uint32_t, 256> maskColors{};
        for (int j = hcount; j; --j) {
            int x = cx, y = cy;
            int dx = tx;
            int offset = y * mapWidth_ + x;
            for (int i = wcount; i; --i, dx += cellWidth_, offset += delta, ++x, --y) {
                if (x < 0 || x >= ::hojy::content::WarFieldWidth || y < 0 || y >= ::hojy::content::WarFieldHeight) {
                    continue;
                }
                auto &ci = cellInfo_[offset];
                renderOk = renderOk && Texture::renderRLE(detail::warfieldTextureAt(texData_, ci.earthId),
                                                          colors, pixels, pitch, aheight, dx, ty);
                if (!movingOrActing) {
                    if (ci.insideMovingArea == 2) {
                        maskColors[254] = 0xA0A0A0A0u;
                        renderOk = renderOk && Texture::renderRLEBlending(detail::warfieldTextureAt(texData_, 0),
                                                                          maskColors.data(), pixels, pitch, aheight, dx, ty);
                    } else if (ci.charInfo) {
                        maskColors[254] = 0x80A0A0A0u;
                        renderOk = renderOk && Texture::renderRLEBlending(detail::warfieldTextureAt(texData_, 0),
                                                                          maskColors.data(), pixels, pitch, aheight, dx, ty);
                    } else if (selecting && !ci.insideMovingArea) {
                        maskColors[254] = 0xD0A0A0A0u;
                        renderOk = renderOk && Texture::renderRLEBlending(detail::warfieldTextureAt(texData_, 0),
                                                                          maskColors.data(), pixels, pitch, aheight, dx, ty);
                    }
                }
                if (ci.buildingId > 0) {
                    renderOk = renderOk && Texture::renderRLE(detail::warfieldTextureAt(texData_, ci.buildingId),
                                                              colors, pixels2, pitch2, aheight, dx, ty);
                } else {
                    if (ci.charInfo) {
                        if (acting && ci.charInfo == ch && fightTex_ && fightTexIdx_ >= 0 && fightTexIdx_ < fightTex_->size()) {
                            renderOk = renderOk && Texture::renderRLE((*fightTex_)[fightTexIdx_], colors, pixels2, pitch2, aheight, dx, ty);
                        } else {
                            const auto textureId = 2553 + 4 * ci.charInfo->texId
                                + int(ci.charInfo->direction);
                            renderOk = renderOk && Texture::renderRLE(detail::warfieldTextureAt(texData_, textureId),
                                                                      colors, pixels2, pitch2, aheight, dx, ty);
                        }
                    }
                    const auto *effectData = effectOverlay[offset];
                    if (effectData) {
                        renderOk = renderOk && Texture::renderRLE(*effectData, colors, pixels2, pitch2, aheight, dx, ty);
                    }
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
        delete drawingTerrainTex2_;
        drawingTerrainTex2_ = candidateOverlay.release();
        delete drawingTerrainTex_;
        drawingTerrainTex_ = candidateTerrain.release();
        commitWorldPresentation();
    }
    if (stage_ == Idle || stage_ == PlayerMenu || stage_ == Moving) {
        detail::invokeIfPresent(
            statusPanel_, [](Node &panel) { panel.dispatchPrepareRender(); });
    }
}

void Warfield::render() const {
    if (!resourcesReady_ || !renderer_ || !drawingTerrainTex_ || !drawingTerrainTex2_) {
        return;
    }
    Map::render();
    bool acting = stage_ == Acting;
    renderer_->clear(0, 0, 0, 0);
    renderer_->renderTexture(drawingTerrainTex_, x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
    renderer_->renderTexture(drawingTerrainTex2_, x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
    if (acting && effectTexIdx_ >= 3) {
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int ax = int(auxWidth_) / 2, ay = int(auxHeight_) / 2 + cellDiffY;
        auto fsize = 12 * scale_.first / scale_.second;
        for (auto &n: popupNumbers_) {
            int deltax = n.x - cameraX_, deltay = n.y - cameraY_;
            int texX = (ax + (deltax - deltay) * cellDiffX) * scale_.first / scale_.second;
            int texY = (ay + (deltax + deltay) * cellDiffY - cellDiffY * 3 - fsize - effectTexIdx_ * 2) * scale_.first / scale_.second;
            auto *ttf = renderer_->ttf();
            texX -= ttf->preparedStringWidth(n.str, fsize) / 2;
            ttf->renderPrepared(n.str, texX + 1, texY, false, fsize,
                                static_cast<std::uint8_t>((n.r + 256) / 2),
                                static_cast<std::uint8_t>((n.g + 256) / 2),
                                static_cast<std::uint8_t>((n.b + 256) / 2));
            ttf->renderPrepared(n.str, texX, texY, false, fsize, n.r, n.g, n.b);
        }
    }
    if (stage_ == Idle || stage_ == PlayerMenu || stage_ == Moving) {
        detail::invokeIfPresent(
            statusPanel_, [](Node &panel) { panel.dispatchRender(); });
    }
}

}
