#include "warfield.hh"

#include "colorpalette.hh"
#include "effect.hh"
#include "node_helpers.hh"
#include "warfield_load.hh"
#include "content/constants.hh"
#include "world/savedata.hh"

#include <algorithm>
#include <cstdint>
#include <cstring>

namespace hojy::scene {
void Warfield::render() {
    Map::render();

    bool acting = stage_ == Acting;
    if (drawDirty_) {
        std::vector<const std::string *> effectOverlay(cellInfo_.size(), nullptr);
        drawDirty_ = false;
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
        if (acting && ch && effectTexIdx_ >= 0
            && effectId_ >= 0
            && !gEffect[effectId_].empty()) {
            const auto *skillInfo = actId_ > 0 ? ::hojy::world::state::gSaveData.skillInfo[actId_] : nullptr;
            const auto &effTexData = gEffect[effectId_];
            const auto *tex = effectTexIdx_ < effTexData.size() ? &effTexData[effectTexIdx_] : &effTexData.back();
            auto mw = mapWidth_;
            if (skillInfo == nullptr || skillInfo->attackAreaType == 0) {
                auto sx = cursorX_, sy = cursorY_;
                effectOverlay[sy * mw + sx] = tex;
            } else {
                switch (skillInfo->attackAreaType) {
                case 1: {
                    auto sx = cameraX_, sy = cameraY_, st = sy * mw;
                    int r = skillInfo->selRange[actLevel_];
                    for (int i = r; i; --i) {
                        switch (ch->direction) {
                        case Map::DirUp:
                            if (sy >= i) {
                                auto &ci = cellInfo_[st - i * mw + sx];
                                if (ci.buildingId <= 0) { effectOverlay[st - i * mw + sx] = tex; }
                            }
                            break;
                        case Map::DirRight:
                            if (sx + i < mapWidth_) {
                                auto &ci = cellInfo_[st + sx + i];
                                if (ci.buildingId <= 0) { effectOverlay[st + sx + i] = tex; }
                            }
                            break;
                        case Map::DirLeft:
                            if (sx >= i) {
                                auto &ci = cellInfo_[st + sx - i];
                                if (ci.buildingId <= 0) { effectOverlay[st + sx - i] = tex; }
                            }
                            break;
                        case Map::DirDown:
                            if (sy + i < mapHeight_) {
                                auto &ci = cellInfo_[st + i * mw + sx];
                                if (ci.buildingId <= 0) { effectOverlay[st + i * mw + sx] = tex; }
                            }
                            break;
                        default:
                            break;
                        }
                    }
                    break;
                }
                case 2: {
                    auto sx = cameraX_, sy = cameraY_, st = sy * mw;
                    int r = skillInfo->selRange[actLevel_];
                    for (int i = r; i; --i) {
                        if (sy >= i) {
                            auto &ci = cellInfo_[st - i * mw + sx];
                            if (ci.buildingId <= 0) { effectOverlay[st - i * mw + sx] = tex; }
                        }
                        if (sx + i < mapWidth_) {
                            auto &ci = cellInfo_[st + sx + i];
                            if (ci.buildingId <= 0) { effectOverlay[st + sx + i] = tex; }
                        }
                        if (sx >= i) {
                            auto &ci = cellInfo_[st + sx - i];
                            if (ci.buildingId <= 0) { effectOverlay[st + sx - i] = tex; }
                        }
                        if (sy + i < mapHeight_) {
                            auto &ci = cellInfo_[st + i * mw + sx];
                            if (ci.buildingId <= 0) { effectOverlay[st + i * mw + sx] = tex; }
                        }
                    }
                    break;
                }
                case 3: {
                    auto sx = cursorX_, sy = cursorY_;
                    int r = skillInfo->selRange[actLevel_];
                    for (int j = -r; j <= r; ++j) {
                        auto ry = sy + j;
                        if (ry < 0 || ry >= mapHeight_) { continue; }
                        for (int i = -r; i <= r; ++i) {
                            auto rx = sx + i;
                            if (rx < 0 || rx >= mapWidth_) { continue; }
                            auto &ci = cellInfo_[ry * mw + rx];
                            if (ci.buildingId <= 0) { effectOverlay[ry * mw + rx] = tex; }
                        }
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }
        const auto *colors = gNormalPalette.colors();
        int pitch, pitch2;
        std::uint32_t *pixels = drawingTerrainTex_->lock(pitch);
        std::uint32_t *pixels2 = drawingTerrainTex2_->lock(pitch2);
        memset(pixels, 0, pitch * auxHeight_ * sizeof(std::uint32_t));
        memset(pixels2, 0, pitch * auxHeight_ * sizeof(std::uint32_t));
        for (int j = hcount; j; --j) {
            int x = cx, y = cy;
            int dx = tx;
            int offset = y * mapWidth_ + x;
            for (int i = wcount; i; --i, dx += cellWidth_, offset += delta, ++x, --y) {
                if (x < 0 || x >= ::hojy::content::WarFieldWidth || y < 0 || y >= ::hojy::content::WarFieldHeight) {
                    continue;
                }
                auto &ci = cellInfo_[offset];
                Texture::renderRLE(detail::warfieldTextureAt(texData_, ci.earthId),
                                   colors, pixels, pitch, aheight, dx, ty);
                if (!movingOrActing) {
                    static std::uint32_t maskColors[256] = {0};
                    if (ci.insideMovingArea == 2) {
                        maskColors[254] = 0xA0A0A0A0u;
                        Texture::renderRLEBlending(detail::warfieldTextureAt(texData_, 0),
                                                   maskColors, pixels, pitch, aheight, dx, ty);
                    } else if (ci.charInfo) {
                        maskColors[254] = 0x80A0A0A0u;
                        Texture::renderRLEBlending(detail::warfieldTextureAt(texData_, 0),
                                                   maskColors, pixels, pitch, aheight, dx, ty);
                    } else if (selecting && !ci.insideMovingArea) {
                        maskColors[254] = 0xD0A0A0A0u;
                        Texture::renderRLEBlending(detail::warfieldTextureAt(texData_, 0),
                                                   maskColors, pixels, pitch, aheight, dx, ty);
                    }
                }
                if (ci.buildingId > 0) {
                    Texture::renderRLE(detail::warfieldTextureAt(texData_, ci.buildingId),
                                       colors, pixels2, pitch2, aheight, dx, ty);
                } else {
                    if (ci.charInfo) {
                        if (acting && ci.charInfo == ch && fightTex_ && fightTexIdx_ >= 0 && fightTexIdx_ < fightTex_->size()) {
                            Texture::renderRLE((*fightTex_)[fightTexIdx_], colors, pixels2, pitch2, aheight, dx, ty);
                        } else {
                            const auto textureId = 2553 + 4 * ci.charInfo->texId
                                + int(ci.charInfo->direction);
                            Texture::renderRLE(detail::warfieldTextureAt(texData_, textureId),
                                               colors, pixels2, pitch2, aheight, dx, ty);
                        }
                    }
                    const auto *effectData = effectOverlay[offset];
                    if (effectData) {
                        Texture::renderRLE(*effectData, colors, pixels2, pitch2, aheight, dx, ty);
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
        drawingTerrainTex2_->unlock();
        drawingTerrainTex_->unlock();
    }
    renderer_->clear(0, 0, 0, 0);
    renderer_->renderTexture(drawingTerrainTex_, x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
    renderer_->renderTexture(drawingTerrainTex2_, x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
    if (acting && effectTexIdx_ >= 3) {
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int ax = int(auxWidth_) / 2, ay = int(auxHeight_) / 2 + cellDiffY;
        auto *ttf = renderer_->ttf();
        auto fsize = 12 * scale_.first / scale_.second;
        for (auto &n: popupNumbers_) {
            int deltax = n.x - cameraX_, deltay = n.y - cameraY_;
            int texX = (ax + (deltax - deltay) * cellDiffX) * scale_.first / scale_.second;
            int texY = (ay + (deltax + deltay) * cellDiffY - cellDiffY * 3 - fsize - effectTexIdx_ * 2) * scale_.first / scale_.second;
            texX -= ttf->stringWidth(n.str, fsize) / 2;
            ttf->setColor((n.r + 256) / 2, (n.g + 256) / 2, (n.b + 256) / 2);
            ttf->render(n.str, texX + 1, texY, false, fsize);
            ttf->setColor(n.r, n.g, n.b);
            ttf->render(n.str, texX, texY, false, fsize);
        }
    }
    if (stage_ == Idle || stage_ == PlayerMenu || stage_ == Moving) {
        detail::invokeIfPresent(
            statusPanel_, [](Node &panel) { panel.render(); });
    }
}

}
