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

#include "warfield.hh"

#include "battle/ai_policy.hh"
#include "battle/ai_strategy.hh"
#include "battle/attack_area.hh"
#include "battle/combat_rules.hh"
#include "battle/game_random.hh"
#include "battle/turn_order.hh"
#include "colorpalette.hh"
#include "menu.hh"
#include "charlistmenu.hh"
#include "statusview.hh"
#include "node_helpers.hh"
#include "warfield_load.hh"
#include "itemview.hh"
#include "window.hh"
#include "effect.hh"
#include "data/grpdata.hh"
#include "data/warfielddata.hh"
#include "mem/savedata.hh"
#include "mem/strings.hh"
#include "core/config.hh"
#include "util/random.hh"
#include <fmt/xchar.h>
#include <algorithm>
#include <limits>
#include <map>
#include <memory>

namespace hojy::scene {

Warfield::Warfield(Renderer *renderer, int x, int y, int width, int height, std::pair<int, int> scale):
    Map(renderer, x, y, width, height, scale),
    drawingTerrainTex2_(Texture::create(renderer, auxWidth_, auxHeight_)) {
    drawingTerrainTex2_->enableBlendMode(true);
    fightTexData_.resize(FightTextureListCount);
    for (size_t i = 0; i < FightTextureListCount; ++i) {
        data::GrpData::loadData(fmt::format("FIGHT{:03}.IDX", i), fmt::format("FIGHT{:03}.GRP", i), fightTexData_[i]);
    }
}

Warfield::~Warfield() {
    delete drawingTerrainTex2_;
    delete statusPanel_;
}

void Warfield::clearActionState(bool clearPopupNumbers) {
    actIndex_ = -1;
    actId_ = -1;
    actLevel_ = 0;
    actItemSlot_ = -1;
    skillLevelup_ = false;
    effectId_ = -1;
    effectTexIdx_ = -1;
    fightTexIdx_ = -1;
    fightTexCount_ = 0;
    fightFrame_ = 0;
    attackTimesLeft_ = 0;
    fightTex_ = nullptr;
    if (clearPopupNumbers) {
        popupNumbers_.clear();
    }
}

void Warfield::cleanup() {
    removeAllChildren();
    fadeNode_ = nullptr;
    fadePostAction_ = nullptr;
    runFadePostAction_ = false;
    pendingAutoAction_ = nullptr;
    resumeAutoAttack_ = false;
    currentActor_ = nullptr;
    for (auto &cell: cellInfo_) {
        cell.charInfo = nullptr;
        cell.effectData = nullptr;
        cell.insideMovingArea = 0;
    }
    turnOrder_.clear();
    charQueue_.clear();
    chars_.clear();
    round_ = 0;
    stage_ = Idle;
    knowledge_[0] = knowledge_[1] = 0;
    cursorX_ = 0;
    cursorY_ = 0;
    autoControl_ = false;
    won_ = false;
    selCells_.clear();
    movingPath_.clear();
    drawDirty_ = true;
    clearActionState(true);
}

bool Warfield::load(std::int16_t warId) {
    const auto *info = data::gWarfieldData.info(warId);
    if (!info) { return false; }
    const auto warMapId = info->warFieldId;
    const auto *warfieldLayers = data::gWarfieldData.layers(warMapId);
    if (!warfieldLayers) { return false; }
    const auto &layers = warfieldLayers->layers;
    const bool mapCached = warMapLoaded_.find(warMapId) != warMapLoaded_.end();
    detail::WarfieldTextureLoad loadedTextures;
    if (mapCached) {
        if (!detail::readWarfieldTextureHeader(texData_, loadedTextures)) {
            return false;
        }
    } else {
        if (!detail::loadWarfieldTextures(
                fmt::format("WDX{:03}", warMapId),
                fmt::format("WMP{:03}", warMapId),
                [](const std::string &idx, const std::string &grp,
                   data::GrpData::DataSet &textures) {
                    return data::GrpData::loadData(idx, grp, textures);
                },
                loadedTextures)) {
            return false;
        }
    }

    const int mapWidth = data::WarFieldWidth;
    const int mapHeight = data::WarFieldHeight;
    const int cellDiffX = loadedTextures.cellWidth / 2;
    const int cellDiffY = loadedTextures.cellHeight / 2;
    const auto size = mapWidth * mapHeight;
    const auto &textureData = mapCached ? texData_ : loadedTextures.textures;
    if (!detail::validateWarfieldTextureIds(
            layers[0], layers[1], static_cast<std::size_t>(size),
            textureData.size())) {
        return false;
    }
    std::vector<CellInfo> cellInfo(static_cast<size_t>(size));

    int x = (mapHeight - 1) * cellDiffX + loadedTextures.offsetX;
    int y = loadedTextures.offsetY;
    int pos = 0;
    for (int j = mapHeight; j; --j) {
        int tx = x, ty = y;
        for (int i = mapWidth; i; --i, ++pos, tx += cellDiffX, ty += cellDiffY) {
            auto &ci = cellInfo[static_cast<size_t>(pos)];
            auto texId = layers[0][pos] >> 1;
            ci.earthId = texId;
            ci.buildingId = layers[1][pos] >> 1;
            ci.blocked = ci.buildingId > 0 || texId >= 179 && texId <= 181 || texId == 261 || texId == 511 || texId >= 662 && texId <= 665 || texId == 674;
        }
        x -= cellDiffX; y += cellDiffY;
    }

    auto nextWarMapLoaded = warMapLoaded_;
    if (!mapCached) {
        detail::commitWarfieldTextureCache(
            nextWarMapLoaded, warMapId, loadedTextures.shared);
    }
    std::unique_ptr<StatusView> newStatusPanel;
    if (!statusPanel_) {
        newStatusPanel = std::make_unique<StatusView>(
            renderer_, x_, y_, width_, height_);
    }

    cleanup();
    warId_ = warId;
    mapWidth_ = mapWidth;
    mapHeight_ = mapHeight;
    cellWidth_ = loadedTextures.cellWidth;
    cellHeight_ = loadedTextures.cellHeight;
    offsetX_ = loadedTextures.offsetX;
    offsetY_ = loadedTextures.offsetY;
    if (!mapCached) {
        textureMgr_.clear();
        texData_ = std::move(loadedTextures.textures);
        warMapLoaded_ = std::move(nextWarMapLoaded);
    }
    cellInfo_ = std::move(cellInfo);

    subMapId_ = warMapId;
    resetFrame();
    if (newStatusPanel) { statusPanel_ = newStatusPanel.release(); }
    return true;
}

bool Warfield::getDefaultChars(std::set<std::int16_t> &chars) const {
    const auto *info = data::gWarfieldData.info(warId_);
    if (!info) { return false; }
    if (info->forceMembers[0] >= 0) { return false; }
    for (auto &id: info->defaultMembers) {
        if (id >= 0) { chars.insert(id); }
    }
    return true;
}

void Warfield::putChars(const std::vector<std::int16_t> &chars) {
    const auto *info = data::gWarfieldData.info(warId_);
    if (!info || cellInfo_.empty()) { return; }
    if (info->forceMembers[0] >= 0) {
        for (size_t i = 0; i < data::TeamMemberCount; ++i) {
            auto id = info->forceMembers[i];
            if (id < 0) { continue; }
            auto *charInfo = mem::gSaveData.charInfo[id];
            if (!charInfo) { continue; }
            chars_.emplace_back(CharInfo {0, id, charInfo->headId, info->memberX[i], info->memberY[i], DirLeft,
                                          *charInfo});
        }
    } else {
        std::map<std::int16_t, size_t> charMap;
        std::set<size_t> indices;
        for (size_t i = 0; i < data::TeamMemberCount; ++i) {
            auto id = info->defaultMembers[i];
            if (id >= 0) { charMap[id] = i; }
            else { indices.insert(i); }
        }
        for (auto id: chars) {
            auto *charInfo = mem::gSaveData.charInfo[id];
            if (!charInfo) { continue; }
            auto ite = charMap.find(id);
            size_t index;
            if (ite != charMap.end()) {
                index = ite->second;
            } else {
                if (indices.empty()) { continue; }
                index = *indices.begin();
                indices.erase(indices.begin());
            }
            chars_.emplace_back(CharInfo{0, id, charInfo->headId, info->memberX[index], info->memberY[index],
                                         DirLeft, *charInfo});
        }
    }
    for (size_t i = 0; i < data::WarFieldEnemyCount; ++i) {
        auto id = info->enemy[i];
        if (id < 0) { continue; }
        auto *charInfo = mem::gSaveData.charInfo[id];
        if (!charInfo) { continue; }
        chars_.emplace_back(CharInfo {1, id, charInfo->headId, info->enemyX[i], info->enemyY[i],
                                      DirRight, *charInfo});
    }
    auto ite = chars_.begin();
    while (ite != chars_.end()) {
        auto &ci = *ite;
        if (ci.x < 0 || ci.x >= mapWidth_ || ci.y < 0 || ci.y >= mapHeight_) {
            ite = chars_.erase(ite);
            continue;
        }
        auto &cell = cellInfo_[ci.y * mapWidth_ + ci.x];
        /* NOTE: remove duplicate chars */
        if (cell.charInfo != nullptr) {
            ite = chars_.erase(ite);
            continue;
        }
        ci.aiEntryStats = battle::snapshotAiStats(ci.info);
        ci.attack = ci.aiEntryStats.attack;
        ci.defence = ci.info.defence;
        ci.persistentEntryMaxMp = ci.info.maxMp;
        mem::addUpPropFromEquipToChar(&ci.info);
        ci.aiEquipmentBonusStats = battle::captureAiEquipmentBonuses(
            ci.aiEntryStats, ci.info);
        ci.battleEntryMaxMp = ci.info.maxMp;
        if (ci.side == 1) {
            ci.info.hp = ci.info.maxHp;
            ci.info.mp = ci.info.maxMp;
            ci.info.stamina = data::StaminaMax;
        }
        cell.charInfo = &ci;
        ++ite;
    }
    turnOrder_.reserve(chars_.size());
    for (auto &ci: chars_) {
        turnOrder_.emplace_back(&ci);
    }
    recalcKnowledge();
    frameUpdate();
    if (info->music >= 0) {
        gWindow->playMusic(info->music);
    }
}

void Warfield::render() {
    Map::render();

    bool acting = stage_ == Acting;
    if (drawDirty_) {
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
            const auto *skillInfo = actId_ > 0 ? mem::gSaveData.skillInfo[actId_] : nullptr;
            const auto &effTexData = gEffect[effectId_];
            const auto *tex = effectTexIdx_ < effTexData.size() ? &effTexData[effectTexIdx_] : &effTexData.back();
            auto mw = mapWidth_;
            if (skillInfo == nullptr || skillInfo->attackAreaType == 0) {
                auto sx = cursorX_, sy = cursorY_;
                cellInfo_[sy * mw + sx].effectData = tex;
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
                                if (ci.buildingId <= 0) { ci.effectData = tex; }
                            }
                            break;
                        case Map::DirRight:
                            if (sx + i < mapWidth_) {
                                auto &ci = cellInfo_[st + sx + i];
                                if (ci.buildingId <= 0) { ci.effectData = tex; }
                            }
                            break;
                        case Map::DirLeft:
                            if (sx >= i) {
                                auto &ci = cellInfo_[st + sx - i];
                                if (ci.buildingId <= 0) { ci.effectData = tex; }
                            }
                            break;
                        case Map::DirDown:
                            if (sy + i < mapHeight_) {
                                auto &ci = cellInfo_[st + i * mw + sx];
                                if (ci.buildingId <= 0) { ci.effectData = tex; }
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
                            if (ci.buildingId <= 0) { ci.effectData = tex; }
                        }
                        if (sx + i < mapWidth_) {
                            auto &ci = cellInfo_[st + sx + i];
                            if (ci.buildingId <= 0) { ci.effectData = tex; }
                        }
                        if (sx >= i) {
                            auto &ci = cellInfo_[st + sx - i];
                            if (ci.buildingId <= 0) { ci.effectData = tex; }
                        }
                        if (sy + i < mapHeight_) {
                            auto &ci = cellInfo_[st + i * mw + sx];
                            if (ci.buildingId <= 0) { ci.effectData = tex; }
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
                            if (ci.buildingId <= 0) { ci.effectData = tex; }
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
                if (x < 0 || x >= data::WarFieldWidth || y < 0 || y >= data::WarFieldHeight) {
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
                    if (ci.effectData) {
                        Texture::renderRLE(*ci.effectData, colors, pixels2, pitch2, aheight, dx, ty);
                        ci.effectData = nullptr;
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

void Warfield::handleKeyInput(Node::Key key) {
    if (stage_ != MoveSelecting && stage_ != AttackSelecting) {
        if (key == KeyCancel) {
            if (currentActor_ && currentActor_->side == 0) {
                pendingAutoAction_ = nullptr;
                resumeAutoAttack_ = false;
                movingPath_.clear();
                if (stage_ == Moving) { stage_ = Idle; }
            }
            autoControl_ = false;
        }
        return;
    }
    int x, y;
    switch (key) {
    case KeyUp:
        y = cursorY_ - 1;
        if (y < 0 || !cellInfo_[y * mapWidth_ + cursorX_].insideMovingArea) { break; }
        cellInfo_[cursorY_ * mapWidth_ + cursorX_].insideMovingArea = 1;
        cursorY_ = y;
        drawDirty_ = true;
        break;
    case KeyDown:
        y = cursorY_ + 1;
        if (y >= mapHeight_ || !cellInfo_[y * mapWidth_ + cursorX_].insideMovingArea) { break; }
        cellInfo_[cursorY_ * mapWidth_ + cursorX_].insideMovingArea = 1;
        cursorY_ = y;
        drawDirty_ = true;
        break;
    case KeyLeft:
        x = cursorX_ - 1;
        if (x < 0 || !cellInfo_[cursorY_ * mapWidth_ + x].insideMovingArea) { break; }
        cellInfo_[cursorY_ * mapWidth_ + cursorX_].insideMovingArea = 1;
        cursorX_ = x;
        drawDirty_ = true;
        break;
    case KeyRight:
        x = cursorX_ + 1;
        if (x >= mapWidth_ || !cellInfo_[cursorY_ * mapWidth_ + x].insideMovingArea) { break; }
        cellInfo_[cursorY_ * mapWidth_ + cursorX_].insideMovingArea = 1;
        cursorX_ = x;
        drawDirty_ = true;
        break;
    case KeyOK: case KeySpace: {
        x = cursorX_; y = cursorY_;
        switch (stage_) {
        case MoveSelecting: {
            if (x == cameraX_ && y == cameraY_) { stage_ = Idle; break; }
            if (cellInfo_[y * mapWidth_ + x].charInfo) {
                stage_ = Idle;
                break;
            }
            auto ite = selCells_.find(std::make_pair(x, y));
            if (ite != selCells_.end()) {
                stage_ = Moving;
                movingPath_.clear();
                auto *sc = &ite->second;
                while (sc) {
                    movingPath_.emplace_back(std::make_pair(sc->x, sc->y));
                    sc = sc->moveParent;
                }
            } else {
                stage_ = Idle;
            }
            break;
        }
        case AttackSelecting: {
            startActAction();
            break;
        }
        default:
            break;
        }
        unmaskArea();
        drawDirty_ = true;
        return;
    }
    case KeyCancel:
        unmaskArea();
        clearActionState(false);
        drawDirty_ = true;
        playerMenu();
        return;
    default:
        return;
    }
    if (drawDirty_) {
        cellInfo_[cursorY_ * mapWidth_ + cursorX_].insideMovingArea = 2;
    }
}

void Warfield::frameUpdate() {
    switch (stage_) {
    case Idle:
        nextAction();
        break;
    case Moving: {
        if (movingPath_.empty() || !currentActor_) {
            movingPath_.clear();
            if (currentActor_ && battle::shouldContinueAfterMovement(
                    currentActor_->side == 0 && !autoControl_,
                    static_cast<bool>(pendingAutoAction_), resumeAutoAttack_)) {
                stage_ = Idle;
            } else if (currentActor_) {
                endTurn(currentActor_);
            } else {
                stage_ = Idle;
            }
            break;
        }
        int x, y;
        std::tie(x, y) = movingPath_.back();
        if (x == cameraX_ && y == cameraY_) {
            movingPath_.pop_back();
            if (movingPath_.empty()) {
                if (battle::shouldContinueAfterMovement(
                        currentActor_->side == 0 && !autoControl_,
                        static_cast<bool>(pendingAutoAction_), resumeAutoAttack_)) {
                    stage_ = Idle;
                } else {
                    endTurn(currentActor_);
                }
                break;
            }
            std::tie(x, y) = movingPath_.back();
        }
        movingPath_.pop_back();
        auto &ci = cellInfo_[cameraX_ + cameraY_ * mapWidth_];
        auto &newci = cellInfo_[x + y * mapWidth_];
        auto *charInfo = ci.charInfo;
        if (charInfo != currentActor_ || newci.charInfo) {
            movingPath_.clear();
            endTurn(currentActor_);
            break;
        }
        if (x < cameraX_) {
            charInfo->direction = DirLeft;
        } else if (x > cameraX_) {
            charInfo->direction = DirRight;
        } else if (y < cameraY_) {
            charInfo->direction = DirUp;
        } else if (y > cameraY_) {
            charInfo->direction = DirDown;
        }
        --charInfo->steps;
        newci.charInfo = charInfo;
        ci.charInfo = nullptr;
        charInfo->x = x;
        charInfo->y = y;
        cameraX_ = x;
        cameraY_ = y;
        drawDirty_ = true;
        if (movingPath_.empty()) {
            if (battle::shouldContinueAfterMovement(
                    currentActor_->side == 0 && !autoControl_,
                    static_cast<bool>(pendingAutoAction_), resumeAutoAttack_)) {
                stage_ = Idle;
            } else {
                endTurn(currentActor_);
            }
        }
        break;
    }
    case Acting: {
        if (!currentActor_) {
            stage_ = Idle;
            clearActionState(false);
            break;
        }
        fightTexIdx_ = std::min(fightTexIdx_ + 1, fightTexCount_ - 1);
        if (fightFrame_ == 0) {
            const mem::SkillData *skillInfo;
            if (actId_ > 0 && (skillInfo = mem::gSaveData.skillInfo[actId_]) != nullptr) {
                gWindow->playAtkSound(skillInfo->soundId);
            } else {
                gWindow->playAtkSound(0);
            }
        } else if (fightFrame_ == 3) {
            gWindow->playEffectSound(effectId_);
        }
        ++fightFrame_;
        if (++effectTexIdx_ >= int(gEffect[effectId_].size()) + 3) {
            auto *actor = currentActor_;
            auto postFunc = [this, actor]() {
                if (currentActor_ != actor) { return; }
                if (--attackTimesLeft_ > 0) {
                    auto *ch = actor;
                    const auto *skill = mem::gSaveData.skillInfo[actId_];
                    if (!skill) {
                        actIndex_ = actId_ = -1;
                        actLevel_ = 0;
                        actItemSlot_ = -1;
                        skillLevelup_ = false;
                        effectId_ = -1;
                        effectTexIdx_ = -1;
                        fightTexIdx_ = -1;
                        fightTexCount_ = 0;
                        fightFrame_ = 0;
                        attackTimesLeft_ = 0;
                        fightTex_ = nullptr;
                        endTurn(actor);
                        return;
                    }
                    actLevel_ = battle::calcRepeatedSkillLevel(
                        skill->reqMp, actLevel_, ch->info.mp);
                    if (actLevel_ >= 0) {
                        startActAction();
                    } else {
                        actIndex_ = actId_ = -1;
                        actItemSlot_ = -1;
                    }
                } else {
                    actIndex_ = actId_ = -1;
                    actItemSlot_ = -1;
                }
                if (actIndex_ < 0) {
                    skillLevelup_ = false;
                    actLevel_ = 0;
                    effectId_ = -1;
                    effectTexIdx_ = -1;
                    fightTexIdx_ = -1;
                    fightTexCount_ = 0;
                    fightFrame_ = 0;
                    attackTimesLeft_ = 0;
                    fightTex_ = nullptr;
                    endTurn(actor);
                }
            };
            if (skillLevelup_) {
                skillLevelup_ = false;
                stage_ = PoppingUp;
                const auto *skill = mem::gSaveData.skillInfo[actId_];
                auto *ch = actor;
                auto *msgBox = new MessageBox(this, 0, height_ / 3, width_, 60);
                msgBox->popup({fmt::format(GETTEXT(81), GETSKILLNAME(actId_),
                                           ch->info.skillLevel[actIndex_] / 100 + 1)}, MessageBox::PressToCloseThis);
                msgBox->setCloseHandler([this, actor, postFunc]() {
                    if (currentActor_ != actor) { return; }
                    stage_ = Acting;
                    postFunc();
                });
            } else {
                postFunc();
            }
        }
        drawDirty_ = true;
        break;
    }
    default:
        break;
    }
}

void Warfield::nextAction() {
    currentActor_ = nullptr;
    CharInfo *ch = nullptr;
    for (;;) {
        if (charQueue_.empty()) {
            if (round_ > 0) {
                for (auto &ci: chars_) {
                    mem::actRoundEndDrain(&ci.info, ci.x < 0 || ci.y < 0);
                }
                if (checkWarEnd()) { return; }
            }
            ++round_;
            charQueue_ = battle::buildRoundQueue(
                turnOrder_,
                [](const CharInfo *actor) { return actor->info.speed; },
                [](const CharInfo *actor) { return actor->info.hp > 0; });
            for (auto *actor: charQueue_) {
                actor->steps = battle::calculateMovementSteps(
                    actor->info.speed, actor->info.hurt);
                actor->initialSteps = actor->steps;
            }
            if (charQueue_.empty()) {
                checkWarEnd();
                return;
            }
        }
        ch = charQueue_.back();
        if (ch->info.hp <= 0) {
            charQueue_.pop_back();
            continue;
        }
        break;
    }
    currentActor_ = ch;
    battle::prepareActorActionCode(
        ch->actionCode,
        static_cast<bool>(pendingAutoAction_) || resumeAutoAttack_);
    cameraX_ = ch->x;
    cameraY_ = ch->y;
    drawDirty_ = true;
    auto *sv = dynamic_cast<StatusView*>(statusPanel_);
    if (sv) {
        auto windowBorder = core::config.windowBorder();
        sv->show(&ch->info, false, true);
        sv->forceUpdate();
        sv->setPosition(ch->side == 1 ? windowBorder * 4
                                      : (width_ - windowBorder * 4 - sv->width()),
                       height_ * 2 / 5 - sv->height() / 2);
    }
    if (ch->side == 1 || autoControl_) {
        autoAction();
    } else {
        lastMenuIndex_ = 0;
        playerMenu();
    }
}

void Warfield::autoAction() {
    if (pendingAutoAction_) {
        battle::runPendingAction(pendingAutoAction_);
        return;
    }
    auto *ch = currentActor_;
    if (!ch) { stage_ = Idle; return; }
    const auto resumeAutoAttack = resumeAutoAttack_;
    resumeAutoAttack_ = false;
    const auto currentAiStats = [](const CharInfo &actor) {
        return battle::resolveAiRuntimeStats(
            actor.aiEntryStats, actor.aiEquipmentBonusStats, actor.info);
    };
    const auto actorAiStats = currentAiStats(*ch);
    const battle::AiResourceState resourceState{
        ch->info.hp, ch->info.maxHp, ch->info.hurt, ch->info.poisoned,
        ch->info.stamina, ch->info.mp, ch->info.maxMp,
        actorAiStats.medic, actorAiStats.depoison,
    };
    const auto actorIndex = static_cast<int>(ch - chars_.data());
    std::vector<battle::AiAllyState> allies;
    allies.reserve(chars_.size());
    for (const auto &ally: chars_) {
        const auto validPosition = ally.x >= 0 && ally.x < mapWidth_
            && ally.y >= 0 && ally.y < mapHeight_;
        const auto allyAiStats = currentAiStats(ally);
        allies.push_back(battle::AiAllyState{
            ally.side, ally.id >= 0 && validPosition,
            ally.info.hp > 0 && validPosition,
            ally.info.hp, ally.info.maxHp, ally.info.hurt,
            ally.info.poisoned, allyAiStats.medic, allyAiStats.depoison,
            ally.actionCode, allyAiStats.attack,
        });
    }
    const auto allyPower = battle::summarizeAllyPower(ch->side, allies);
    std::int16_t resourceItemId = -1;
    int resourceActId = 0;
    std::optional<int> resourceTargetIndex;
    std::optional<std::pair<int, int>> resourceSupportPosition;
    std::vector<std::pair<int, int>> resourceMovingPath;
    auto findResourceItem = [ch, &resourceItemId](mem::PropType type, int delta) {
        resourceItemId = ch->side == 1
            ? mem::tryUseNpcItem(&ch->info, type, static_cast<std::int16_t>(delta))
            : mem::tryUseBagItem(&ch->info, type, static_cast<std::int16_t>(delta));
        return resourceItemId >= 0;
    };
    using Position = std::pair<int, int>;
    const auto onMap = [this](Position position) {
        return position.first >= 0 && position.first < mapWidth_
            && position.second >= 0 && position.second < mapHeight_;
    };
    const auto terrainDistance = [this](Position from, Position target) {
        return battle::terrainPathDistance(
            mapWidth_, mapHeight_, from, target,
            [this](int x, int y) {
                return cellInfo_[y * mapWidth_ + x].blocked;
            });
    };
    const auto canCastAtCurrentPosition = [this, ch, onMap, terrainDistance](
                                               int targetIndex,
                                               int attackAreaType,
                                               int range) {
        if (targetIndex < 0
            || targetIndex >= static_cast<int>(chars_.size())
            || !onMap({ch->x, ch->y})) {
            return false;
        }
        const auto &target = chars_[targetIndex];
        if (target.info.hp <= 0 || !onMap({target.x, target.y})) {
            return false;
        }
        const Position actorPosition{ch->x, ch->y};
        const Position targetPosition{target.x, target.y};
        return battle::canCastFromPosition(
            attackAreaType, range,
            terrainDistance(actorPosition, targetPosition),
            actorPosition, targetPosition);
    };
    const auto buildCastRangeCells = [this, onMap](
                                          Position targetPosition,
                                          Position actorPosition,
                                          int range) {
        std::map<Position, SelectableCell> castRangeCells;
        if (!onMap(targetPosition) || !onMap(actorPosition)) {
            return castRangeCells;
        }
        battle::getCastRangeArea(
            mapWidth_, mapHeight_, targetPosition, std::max(0, range),
            castRangeCells,
            [this](int x, int y) {
                return cellInfo_[y * mapWidth_ + x].blocked;
            },
            [this, targetPosition, actorPosition](int x, int y) {
                const Position position{x, y};
                return position != targetPosition && position != actorPosition
                    && cellInfo_[y * mapWidth_ + x].charInfo != nullptr;
            });
        return castRangeCells;
    };
    battle::GameRandom resourceRandom;
    auto prepareSupport = [this, ch, actorIndex, actorAiStats, &allies, &resourceRandom,
                           &resourceActId, &resourceTargetIndex,
                           &resourceSupportPosition, &resourceMovingPath,
                           &buildCastRangeCells, &terrainDistance]
                          (battle::AiResourceAction action) {
        std::optional<int> targetIndex;
        int ability = 0;
        if (action == battle::AiResourceAction::MedicSupport) {
            ability = actorAiStats.medic;
            resourceActId = -1;
            targetIndex = battle::chooseMedicSupportTarget(
                actorIndex, ability, allies, resourceRandom);
        } else {
            ability = actorAiStats.depoison;
            resourceActId = -2;
            targetIndex = battle::chooseDepoisonSupportTarget(
                actorIndex, ability, allies, resourceRandom);
        }
        if (!targetIndex) { return battle::AiResourceAction::None; }

        resourceTargetIndex = targetIndex;
        const auto *target = &chars_[*targetIndex];
        std::map<std::pair<int, int>, SelectableCell> movementCells;
        getSelectableArea(ch, movementCells, ch->steps, 0);
        const std::pair<int, int> targetPosition{target->x, target->y};
        const std::pair<int, int> actorPosition{ch->x, ch->y};
        const auto castRangeCells = buildCastRangeCells(
            targetPosition, actorPosition, battle::calcTechniqueRange(ability));
        const auto currentCanCast = battle::canCastFromPosition(
            0, battle::calcTechniqueRange(ability),
            terrainDistance(actorPosition, targetPosition),
            actorPosition, targetPosition);
        const auto choice = battle::chooseCastMovementPosition(
            movementCells, castRangeCells, actorPosition, targetPosition,
            battle::calcTechniqueRange(ability),
            battle::CastMovementMode::Approach, currentCanCast,
            terrainDistance);
        resourceSupportPosition = choice.position;
        if (resourceSupportPosition) {
            auto *cell = &movementCells[*resourceSupportPosition];
            while (cell) {
                resourceMovingPath.emplace_back(cell->x, cell->y);
                cell = cell->moveParent;
            }
        }
        return action;
    };
    const auto resourceAction = resumeAutoAttack
        ? battle::AiResourceAction::None
        : battle::chooseAiResourceAction(
            resourceState, resourceRandom,
             [ch, actorIndex, actorAiStats, &allies, &findResourceItem, &resourceActId,
             &resourceTargetIndex, &prepareSupport](battle::AiResourceAction action) {
                switch (action) {
                case battle::AiResourceAction::RecoverHp:
                    if (battle::canSelfMedic(
                            actorAiStats.medic, ch->info.stamina, ch->info.hurt)) {
                        resourceActId = -1;
                        return action;
                    }
                    if (findResourceItem(mem::PropType::Hp, ch->info.maxHp - ch->info.hp)) {
                        return action;
                    }
                    resourceTargetIndex = battle::chooseMedicProvider(
                        actorIndex, ch->info.hurt, allies);
                    return resourceTargetIndex
                        ? battle::AiResourceAction::RequestMedic
                        : battle::AiResourceAction::None;
                case battle::AiResourceAction::SelfDepoison:
                    if (battle::canSelfDepoison(
                            actorAiStats.depoison, ch->info.stamina, ch->info.poisoned)) {
                        resourceActId = -2;
                        return action;
                    }
                    if (findResourceItem(mem::PropType::Poisoned, ch->info.poisoned)) {
                        return action;
                    }
                    resourceTargetIndex = battle::chooseDepoisonProvider(
                        actorIndex, ch->info.poisoned, allies);
                    return resourceTargetIndex
                        ? battle::AiResourceAction::RequestDepoison
                        : battle::AiResourceAction::None;
                case battle::AiResourceAction::RecoverMp:
                    return findResourceItem(mem::PropType::Mp, ch->info.maxMp - ch->info.mp)
                        ? action : battle::AiResourceAction::None;
                case battle::AiResourceAction::MedicSupport:
                case battle::AiResourceAction::DepoisonSupport:
                    return prepareSupport(action);
                default:
                    return battle::AiResourceAction::None;
                }
            });
    const auto supportWithoutPosition =
        (resourceAction == battle::AiResourceAction::MedicSupport
         || resourceAction == battle::AiResourceAction::DepoisonSupport)
        && !resourceSupportPosition;
    const auto requestSupport =
        resourceAction == battle::AiResourceAction::RequestMedic
        || resourceAction == battle::AiResourceAction::RequestDepoison;
    if (!resumeAutoAttack) {
        ch->actionCode = battle::originalActionCode(
            resourceAction, resourceItemId >= 0);
    }
    if (requestSupport) {
        if (resourceTargetIndex) {
            const auto *provider = &chars_[*resourceTargetIndex];
            std::map<std::pair<int, int>, SelectableCell> movementCells;
            getSelectableArea(ch, movementCells, ch->steps, 0);
            const auto providerPosition = std::make_pair(provider->x, provider->y);
            const auto approachPosition = battle::chooseApproachPosition(
                movementCells, providerPosition,
                [this](std::pair<int, int> from, std::pair<int, int> target) {
                    return battle::shortestPathDistance(
                        mapWidth_, mapHeight_, from, target,
                        [this](int x, int y) {
                            return cellInfo_[y * mapWidth_ + x].blocked;
                        },
                        [this, target](int x, int y) {
                            return std::make_pair(x, y) != target
                                && cellInfo_[y * mapWidth_ + x].charInfo != nullptr;
                        });
                });
            if (approachPosition
                && *approachPosition != std::make_pair<int, int>(ch->x, ch->y)) {
                auto *cell = &movementCells[*approachPosition];
                movingPath_.clear();
                while (cell) {
                    movingPath_.emplace_back(cell->x, cell->y);
                    cell = cell->moveParent;
                }
                resumeAutoAttack_ = battle::shouldResumeAutoAttack(true);
                stage_ = Moving;
                return;
            }
        }
        // No approach movement was scheduled.  The current actor continues
        // immediately, and the continuation flag must not leak to the next
        // queued actor.
        resumeAutoAttack_ = battle::shouldResumeAutoAttack(false);
    }
    if (supportWithoutPosition
        && battle::chooseUnreachableSupportFallback(
               actorAiStats.attack, allyPower.total, allyPower.count)
           == battle::AiSupportFallback::Rest) {
        pendingAutoAction_ = [this, ch]() { doRest(ch); };
        battle::runPendingAction(pendingAutoAction_);
        return;
    }
    if (resourceAction != battle::AiResourceAction::None
        && !supportWithoutPosition && !requestSupport) {
        if (resourceAction == battle::AiResourceAction::Rest) {
            pendingAutoAction_ = [this, ch]() { doRest(ch); };
        } else if (resourceItemId >= 0) {
            pendingAutoAction_ = [this, ch, resourceItemId]() {
                if (currentActor_ != ch) { return; }
                if (ch->info.hp <= 0) {
                    endTurn(ch);
                    return;
                }
                std::map<mem::PropType, std::int16_t> changes;
                const auto usedItem = ch->side == 1
                    ? mem::useNpcItem(&ch->info, resourceItemId, changes)
                    : mem::useItem(&ch->info, resourceItemId, changes);
                if (!usedItem) {
                    doRest(ch);
                    return;
                }
                stage_ = PoppingUp;
                auto *msgBox = ItemView::popupUseResult(this, resourceItemId, changes);
                msgBox->setCloseHandler([this, ch] {
                    if (currentActor_ != ch) { return; }
                    endTurn(ch);
                });
            };
        } else {
            auto *target = resourceTargetIndex
                ? &chars_[*resourceTargetIndex] : ch;
            const auto targetIndex = resourceTargetIndex.value_or(-1);
            const auto resourceRange = (resourceAction
                == battle::AiResourceAction::MedicSupport)
                ? battle::calcTechniqueRange(actorAiStats.medic)
                : battle::calcTechniqueRange(actorAiStats.depoison);
            const auto castCheck = canCastAtCurrentPosition;
            pendingAutoAction_ = [this, ch, target, targetIndex,
                                  resourceActId, resourceAction,
                                  resourceRange, castCheck, allyPower,
                                  actorAiStats]() {
                if (currentActor_ != ch) { return; }
                if (ch->info.hp <= 0 || target->info.hp <= 0
                    || target->x < 0 || target->x >= mapWidth_
                    || target->y < 0 || target->y >= mapHeight_) {
                    endTurn(ch);
                    return;
                }
                if ((resourceAction == battle::AiResourceAction::MedicSupport
                     || resourceAction == battle::AiResourceAction::DepoisonSupport)
                    && !castCheck(targetIndex, 0, resourceRange)) {
                    if (battle::chooseUnreachableSupportFallback(
                            actorAiStats.attack, allyPower.total, allyPower.count)
                        == battle::AiSupportFallback::Rest) {
                        doRest(ch);
                    } else {
                        /* Keep action code 4/5 while the actor falls back to
                         * the ordinary random-skill path on the next frame. */
                        resumeAutoAttack_ = true;
                    }
                    return;
                }
                actIndex_ = -1;
                actId_ = resourceActId;
                actLevel_ = 0;
                attackTimesLeft_ = 1;
                cursorX_ = target->x;
                cursorY_ = target->y;
                startActAction();
            };
        }
        if (resourceItemId >= 0) {
            std::map<std::pair<int, int>, SelectableCell> movementCells;
            getSelectableArea(ch, movementCells, ch->steps, 0);
            std::vector<std::pair<int, int>> enemyPositions;
            for (const auto &candidate: chars_) {
                if (candidate.id >= 0 && candidate.side != ch->side
                    && candidate.x >= 0 && candidate.x < mapWidth_
                    && candidate.y >= 0 && candidate.y < mapHeight_) {
                    enemyPositions.emplace_back(candidate.x, candidate.y);
                }
            }
            const auto retreatPosition = battle::chooseRetreatPosition(
                movementCells, ch->steps, enemyPositions);
            if (retreatPosition
                && *retreatPosition != std::make_pair<int, int>(ch->x, ch->y)) {
                movingPath_.clear();
                auto *cell = &movementCells[*retreatPosition];
                while (cell) {
                    movingPath_.emplace_back(cell->x, cell->y);
                    cell = cell->moveParent;
                }
                stage_ = Moving;
                return;
            }
        }
        if (resourceSupportPosition
            && (*resourceSupportPosition != std::make_pair<int, int>(ch->x, ch->y))) {
            movingPath_ = std::move(resourceMovingPath);
            stage_ = Moving;
            return;
        }
        battle::runPendingAction(pendingAutoAction_);
        return;
    }
    std::vector<battle::AiStrategyCharacter> strategyCharacters;
    strategyCharacters.reserve(chars_.size());
    for (const auto &candidate: chars_) {
        const auto validPosition = candidate.x >= 0 && candidate.x < mapWidth_
            && candidate.y >= 0 && candidate.y < mapHeight_;
        const auto candidateAiStats = currentAiStats(candidate);
        strategyCharacters.push_back(battle::AiStrategyCharacter{
            candidate.side, candidate.id >= 0 && validPosition,
            candidate.info.hp > 0 && validPosition,
            candidate.x, candidate.y, candidate.info.hp, candidate.info.maxHp,
            candidateAiStats.attack, candidateAiStats.medic,
            candidate.info.poisoned, candidateAiStats.depoison,
            candidateAiStats.antipoison, candidateAiStats.poison,
        });
    }
    battle::AiStrategyActor strategyActor;
    strategyActor.side = ch->side;
    strategyActor.hp = ch->info.hp;
    strategyActor.attack = actorAiStats.attack;
    strategyActor.stamina = ch->info.stamina;
    strategyActor.mp = ch->info.mp;
    strategyActor.medic = actorAiStats.medic;
    strategyActor.poison = actorAiStats.poison;
    strategyActor.depoison = actorAiStats.depoison;
    strategyActor.throwing = actorAiStats.throwing;
    strategyActor.integrity = actorAiStats.integrity;
    strategyActor.potential = actorAiStats.potential;

    std::vector<battle::AiSkillOption> strategySkills;
    strategySkills.reserve(data::LearnSkillCount);
    for (int i = 0; i < data::LearnSkillCount; ++i) {
        const auto skillId = ch->info.skillId[i];
        if (skillId <= 0) { continue; }
        const auto *skill = mem::gSaveData.skillInfo[skillId];
        if (!skill) { continue; }
        strategySkills.push_back(battle::AiSkillOption{
            i, skillId, skill->reqMp,
        });
    }

    std::vector<battle::AiThrowingOption> throwingItems;
    if (ch->side != 0) {
        for (int i = 0; i < data::CarryItemCount; ++i) {
            const auto itemId = ch->info.item[i];
            if (itemId < 0 || ch->info.itemCount[i] <= 0) { continue; }
            const auto *item = mem::gSaveData.itemInfo[itemId];
            if (!item) { continue; }
            throwingItems.push_back(battle::AiThrowingOption{
                i, itemId, item->addHp, item->addPoisoned, item->itemType,
            });
        }
    } else {
        int bagIndex = 0;
        for (const auto &[itemId, count]: mem::gBag.orderedItems()) {
            if (itemId < 0 || count <= 0) { continue; }
            const auto *item = mem::gSaveData.itemInfo[itemId];
            if (!item) { continue; }
            throwingItems.push_back(battle::AiThrowingOption{
                bagIndex++, itemId, item->addHp, item->addPoisoned, item->itemType,
            });
        }
    }

    const auto pathDistance = [this, ch, &strategyCharacters](int targetIndex) {
        if (targetIndex < 0
            || targetIndex >= static_cast<int>(strategyCharacters.size())) {
            return -1;
        }
        const auto &target = strategyCharacters[targetIndex];
        if (!target.valid || !target.alive) { return -1; }
        const std::pair<int, int> targetPosition{target.x, target.y};
        return battle::terrainPathDistance(
            mapWidth_, mapHeight_, {ch->x, ch->y}, targetPosition,
            [this](int x, int y) {
                return cellInfo_[y * mapWidth_ + x].blocked;
            });
    };

    auto runAtPosition = [this, ch](
                             const std::map<std::pair<int, int>, SelectableCell> &cells,
                             const std::pair<int, int> &position,
                             std::function<void()> action) {
        pendingAutoAction_ = [this, ch, action = std::move(action)]() mutable {
            if (currentActor_ != ch) { return; }
            if (ch->info.hp <= 0) {
                endTurn(ch);
                return;
            }
            action();
        };
        if (position != std::make_pair<int, int>(ch->x, ch->y)) {
            const auto found = cells.find(position);
            if (found == cells.end()) {
                pendingAutoAction_ = nullptr;
                if (currentActor_ == ch) { endTurn(ch); }
                return false;
            }
            movingPath_.clear();
            auto *cell = &found->second;
            while (cell) {
                movingPath_.emplace_back(cell->x, cell->y);
                cell = cell->moveParent;
            }
            stage_ = Moving;
            return true;
        }
        battle::runPendingAction(pendingAutoAction_);
        return true;
    };

    struct CastPositionPlan {
        std::map<Position, SelectableCell> movementCells;
        std::optional<Position> position;
        bool expectedInRange = false;
    };
    auto chooseCastPosition = [this, ch, buildCastRangeCells,
                               canCastAtCurrentPosition, terrainDistance](
                                  int targetIndex, int range,
                                  int attackAreaType,
                                  battle::CastMovementMode mode) {
        CastPositionPlan plan;
        getSelectableArea(ch, plan.movementCells, ch->steps, 0);
        if (targetIndex < 0
            || targetIndex >= static_cast<int>(chars_.size())) {
            return plan;
        }
        const auto &target = chars_[targetIndex];
        const Position targetPosition{target.x, target.y};
        const Position actorPosition{ch->x, ch->y};
        const auto castRangeCells = buildCastRangeCells(
            targetPosition, actorPosition, range);
        const auto choice = battle::chooseCastMovementPosition(
            plan.movementCells, castRangeCells, actorPosition, targetPosition,
            range, mode,
            canCastAtCurrentPosition(targetIndex, attackAreaType, range),
            terrainDistance);
        plan.position = choice.position;
        plan.expectedInRange = choice.expectedInRange;
        return plan;
    };

    const auto forceSkill = resumeAutoAttack || requestSupport || supportWithoutPosition;
    bool preserveSupportFallback = false;
    if (!forceSkill && resourceAction == battle::AiResourceAction::None
        && battle::shouldRetreatForHealth(resourceState, resourceRandom)) {
        std::map<std::pair<int, int>, SelectableCell> movementCells;
        getSelectableArea(ch, movementCells, ch->steps, 0);
        std::vector<std::pair<int, int>> enemyPositions;
        for (const auto &candidate: chars_) {
            if (candidate.id >= 0 && candidate.side != ch->side
                && candidate.x >= 0 && candidate.x < mapWidth_
                && candidate.y >= 0 && candidate.y < mapHeight_) {
                enemyPositions.emplace_back(candidate.x, candidate.y);
            }
        }
        const auto retreatPosition = battle::chooseRetreatPosition(
            movementCells, ch->steps, enemyPositions);
        if (retreatPosition) {
            runAtPosition(movementCells, *retreatPosition,
                          [this]() { doRest(); });
        } else {
            doRest();
        }
        return;
    }

    auto followup = forceSkill
        ? battle::AiFollowupDecision{battle::AiFollowupAction::Skill, -1, -1}
        : battle::chooseAiFollowupAction(
            actorIndex, strategyActor, strategyCharacters,
            throwingItems, strategySkills, resourceRandom);

    if (followup.action == battle::AiFollowupAction::Rest) {
        ch->actionCode = 7;
        doRest();
        return;
    }

    if (followup.action == battle::AiFollowupAction::MedicSupport
        || followup.action == battle::AiFollowupAction::DepoisonSupport) {
        const auto medic = followup.action == battle::AiFollowupAction::MedicSupport;
        const auto range = battle::calcTechniqueRange(
            medic ? actorAiStats.medic : actorAiStats.depoison);
        ch->actionCode = medic ? 5 : 4;
        auto plan = chooseCastPosition(
            followup.targetIndex, range, 0,
            battle::CastMovementMode::Approach);
        if (plan.position) {
            const auto targetIndex = followup.targetIndex;
            const auto castCheck = canCastAtCurrentPosition;
            auto *target = targetIndex >= 0
                && targetIndex < static_cast<int>(chars_.size())
                ? &chars_[targetIndex] : nullptr;
            runAtPosition(
                plan.movementCells, *plan.position,
                [this, ch, target, targetIndex, medic, range, castCheck,
                 allyPower, actorAiStats]() {
                    if (!target || target->info.hp <= 0
                        || !castCheck(targetIndex, 0, range)) {
                        if (battle::chooseUnreachableSupportFallback(
                                actorAiStats.attack, allyPower.total, allyPower.count)
                            == battle::AiSupportFallback::Rest) {
                            doRest(ch);
                        } else {
                            /* Preserve action code 4/5 and let the next
                             * invocation enter the ordinary skill path. */
                            resumeAutoAttack_ = true;
                        }
                        return;
                    }
                    actIndex_ = -1;
                    actId_ = medic ? -1 : -2;
                    actLevel_ = 0;
                    actItemSlot_ = -1;
                    attackTimesLeft_ = 1;
                    cursorX_ = target->x;
                    cursorY_ = target->y;
                    startActAction();
                });
            return;
        }
        if (battle::chooseUnreachableSupportFallback(
                actorAiStats.attack, allyPower.total, allyPower.count)
            == battle::AiSupportFallback::Rest) {
            doRest();
            return;
        }
        preserveSupportFallback = true;
        followup.action = battle::AiFollowupAction::Skill;
    }

    if (followup.action == battle::AiFollowupAction::Poison) {
        const auto targetIndex = battle::choosePoisonTarget(
            actorIndex, strategyActor, strategyCharacters,
            resourceRandom, pathDistance);
        if (!targetIndex) {
            /* Z.DAT:sub_3540E jumps directly to random-skill selection when
             * no eligible poison target exists. */
            followup.action = battle::AiFollowupAction::Skill;
        } else {
            const auto range = battle::calcTechniqueRange(actorAiStats.poison);
            ch->actionCode = 3;
            auto plan = chooseCastPosition(
                *targetIndex, range, 0,
                ch->steps > 0 ? battle::CastMovementMode::Reposition
                              : battle::CastMovementMode::Approach);
            if (plan.position) {
                const auto selectedTargetIndex = *targetIndex;
                const auto castCheck = canCastAtCurrentPosition;
                auto *target = &chars_[selectedTargetIndex];
                runAtPosition(
                    plan.movementCells, *plan.position,
                    [this, ch, target, selectedTargetIndex, range, castCheck,
                     allyPower, actorAiStats]() {
                        if (!target || target->info.hp <= 0
                            || target->side == ch->side
                            || !castCheck(selectedTargetIndex, 0, range)) {
                            /* sub_3540E uses the same team-power split when
                             * repositioning still cannot reach the target. */
                            if (battle::chooseUnreachableSupportFallback(
                                    actorAiStats.attack, allyPower.total,
                                    allyPower.count)
                                == battle::AiSupportFallback::Rest) {
                                doRest(ch);
                            } else {
                                resumeAutoAttack_ = true;
                            }
                            return;
                        }
                        actIndex_ = -1;
                        actId_ = -3;
                        actLevel_ = 0;
                        actItemSlot_ = -1;
                        attackTimesLeft_ = 1;
                        cursorX_ = target->x;
                        cursorY_ = target->y;
                        startActAction();
                    });
                return;
            }
            if (battle::chooseUnreachableSupportFallback(
                    actorAiStats.attack, allyPower.total, allyPower.count)
                == battle::AiSupportFallback::Rest) {
                doRest();
                return;
            }
            followup.action = battle::AiFollowupAction::Skill;
        }
    }

    if (followup.action == battle::AiFollowupAction::Throw) {
        const auto targetIndex = battle::chooseAiTarget(
            actorIndex, strategyActor, strategyCharacters,
            resourceRandom, pathDistance);
        const auto item = std::find_if(
            throwingItems.begin(), throwingItems.end(),
            [&followup](const battle::AiThrowingOption &candidate) {
                return candidate.selectionIndex == followup.selectionIndex;
            });
        if (targetIndex && item != throwingItems.end()) {
            const auto range = battle::calcTechniqueRange(actorAiStats.throwing);
            ch->actionCode = 10;
            auto plan = chooseCastPosition(
                *targetIndex, range, 0,
                battle::CastMovementMode::Approach);
            if (plan.position) {
                const auto selectedTargetIndex = *targetIndex;
                const auto castCheck = canCastAtCurrentPosition;
                auto *target = &chars_[selectedTargetIndex];
                const auto itemId = item->itemId;
                const auto itemSlot = ch->side != 0 ? item->selectionIndex : -1;
                runAtPosition(
                    plan.movementCells, *plan.position,
                    [this, ch, target, selectedTargetIndex, range, castCheck,
                     itemId, itemSlot]() {
                        if (!target || target->info.hp <= 0
                            || target->side == ch->side
                            || !castCheck(selectedTargetIndex, 0, range)) {
                            /* sub_3582B falls through to random skills after
                             * a failed post-move throw check. */
                            resumeAutoAttack_ = true;
                            return;
                        }
                        actIndex_ = itemId;
                        actId_ = -4;
                        actLevel_ = 0;
                        actItemSlot_ = itemSlot;
                        attackTimesLeft_ = 1;
                        cursorX_ = target->x;
                        cursorY_ = target->y;
                        startActAction();
                    });
                return;
            }
        }
        followup.action = battle::AiFollowupAction::Skill;
    }

    const auto skillSlot = battle::chooseOriginalSkillSlot(
        strategySkills, resourceRandom);
    if (!skillSlot || *skillSlot < 0 || *skillSlot >= data::LearnSkillCount) {
        doRest();
        return;
    }
    const auto skillId = ch->info.skillId[*skillSlot];
    const auto *skill = skillId > 0 ? mem::gSaveData.skillInfo[skillId] : nullptr;
    if (!skill) {
        doRest();
        return;
    }
    const auto storedSkillLevel = ch->info.skillLevel[*skillSlot];
    const auto skillLevels = battle::resolveAiSkillLevels(
        skill->reqMp, storedSkillLevel, ch->info.mp);
    /*
     * Z.DAT:sub_34C47 chooses the candidate position from the stored
     * proficiency level.  MP-based level forcing happens later, when the
     * selected action is executed (sub_37734).  Resolving it here changes
     * target selection for low-MP actors because selRange[level] can differ.
     */
    const auto skillRange = skill->selRange[skillLevels.planning];

    struct SkillPlan {
        std::map<Position, SelectableCell> movementCells;
        std::optional<Position> position;
        bool expectedInRange = false;
    };
    auto planSkillPosition = [this, ch, skill, skillRange,
                              buildCastRangeCells, canCastAtCurrentPosition,
                              terrainDistance](int targetIndex) {
        SkillPlan plan;
        getSelectableArea(ch, plan.movementCells, ch->steps, 0);
        if (targetIndex < 0
            || targetIndex >= static_cast<int>(chars_.size())) {
            return plan;
        }
        const auto &target = chars_[targetIndex];
        const Position targetPosition{target.x, target.y};
        const Position actorPosition{ch->x, ch->y};
        const auto castRangeCells = buildCastRangeCells(
            targetPosition, actorPosition, skillRange);
        const auto mode = (skill->attackAreaType == 1
                           || skill->attackAreaType == 2)
            ? battle::CastMovementMode::Aligned
            : battle::CastMovementMode::Approach;
        const auto choice = battle::chooseCastMovementPosition(
            plan.movementCells, castRangeCells, actorPosition, targetPosition,
            skillRange, mode,
            canCastAtCurrentPosition(
                targetIndex, skill->attackAreaType, skillRange),
            terrainDistance);
        plan.position = choice.position;
        plan.expectedInRange = choice.expectedInRange;
        return plan;
    };

    const auto nearestCurrentTarget = [this, ch, actorIndex, onMap,
                                       terrainDistance]() {
        std::optional<int> selected;
        auto selectedDistance = std::numeric_limits<int>::max();
        for (std::size_t i = 0; i < chars_.size(); ++i) {
            const auto &candidate = chars_[i];
            if (static_cast<int>(i) == actorIndex
                || candidate.id < 0 || candidate.side == ch->side
                || candidate.info.hp <= 0
                || !onMap({candidate.x, candidate.y})) {
                continue;
            }
            const auto distance = terrainDistance(
                {ch->x, ch->y}, {candidate.x, candidate.y});
            if (distance < 0 || distance >= selectedDistance) { continue; }
            selected = static_cast<int>(i);
            selectedDistance = distance;
        }
        return selected;
    };

    const int selectedSkillSlot = *skillSlot;
    const auto castSkillAtCurrentPosition = [this, ch, skill, skillId,
                                              selectedSkillSlot,
                                              storedSkillLevel](int targetIndex) {
        if (targetIndex < 0 || targetIndex >= static_cast<int>(chars_.size())) {
            doRest(ch);
            return;
        }
        const auto &target = chars_[targetIndex];
        const auto directional = skill->attackAreaType == 1;
        if (directional) {
            const auto dx = target.x - ch->x;
            const auto dy = target.y - ch->y;
            if (dy < 0) ch->direction = DirUp;
            else if (dx > 0) ch->direction = DirRight;
            else if (dx < 0) ch->direction = DirLeft;
            else ch->direction = DirDown;
            cursorX_ = ch->x;
            cursorY_ = ch->y;
        } else {
            cursorX_ = target.x;
            cursorY_ = target.y;
        }
        actIndex_ = selectedSkillSlot;
        actId_ = skillId;
        /* Resolve the actual cast level only after movement is complete. */
        actLevel_ = battle::resolveAiSkillLevels(
            skill->reqMp, storedSkillLevel, ch->info.mp).execution;
        actItemSlot_ = -1;
        if (actLevel_ < 0) {
            doRest(ch);
            return;
        }
        attackTimesLeft_ = battle::attackCount(ch->info.doubleAttack);
        startActAction();
    };

    auto targetIndex = battle::chooseAiTarget(
        actorIndex, strategyActor, strategyCharacters,
        resourceRandom, pathDistance);
    SkillPlan skillPlan;
    if (targetIndex) {
        skillPlan = planSkillPosition(*targetIndex);
    }
    ch->actionCode = battle::actionCodeForSkill(
        ch->actionCode, forceSkill || preserveSupportFallback);
    if (skillPlan.position) {
        const auto selectedTargetIndex = *targetIndex;
        const auto castCheck = canCastAtCurrentPosition;
        const auto nearestTarget = nearestCurrentTarget;
        runAtPosition(
            skillPlan.movementCells, *skillPlan.position,
            [this, ch, selectedTargetIndex, skillRange, skill,
             castCheck, nearestTarget, castSkillAtCurrentPosition]() {
                if (castCheck(selectedTargetIndex,
                              skill->attackAreaType, skillRange)) {
                    castSkillAtCurrentPosition(selectedTargetIndex);
                    return;
                }
                const auto fallback = nearestTarget();
                if (fallback && castCheck(
                        *fallback, skill->attackAreaType, skillRange)) {
                    castSkillAtCurrentPosition(*fallback);
                } else {
                    doRest(ch);
                }
            });
        return;
    }

    const auto fallbackTarget = nearestCurrentTarget();
    if (fallbackTarget && canCastAtCurrentPosition(
            *fallbackTarget, skill->attackAreaType, skillRange)) {
        castSkillAtCurrentPosition(*fallbackTarget);
        return;
    }
    doRest();
}

void Warfield::recalcKnowledge() {
    knowledge_[0] = knowledge_[1] = 0;
    for (auto &ci: chars_) {
        if (ci.info.hp > 0 && ci.info.knowledge > data::KnowledgeBarrier) {
            knowledge_[ci.side] += ci.info.knowledge;
        }
    }
}

void Warfield::playerMenu() {
    stage_ = PlayerMenu;
    auto windowBorder = core::config.windowBorder();
    auto *ch = currentActor_;
    if (!ch) { stage_ = Idle; return; }
    auto *menu = new MenuTextList(this, windowBorder * 4, windowBorder * 4, width_ - windowBorder * 8, height_ - windowBorder * 8);
    std::vector<std::wstring> n;
    std::vector<int> menuIndices;
    n.reserve(10);
    menuIndices.reserve(10);
    auto &info = ch->info;
    if (ch->steps > 0 && info.stamina > 5) {
        n.emplace_back(GETTEXT(82)); menuIndices.emplace_back(0);
    }
    if (info.stamina > 10) {
        n.emplace_back(GETTEXT(83)); menuIndices.emplace_back(1);
        if (info.poison >= 20) {
            n.emplace_back(GETTEXT(84)); menuIndices.emplace_back(2);
        }
    }
    if (info.stamina > 50) {
        if (info.depoison >= 20) {
            n.emplace_back(GETTEXT(85));
            menuIndices.emplace_back(3);
        }
        if (info.medic >= 20) {
            n.emplace_back(GETTEXT(86));
            menuIndices.emplace_back(4);
        }
    }
    n.emplace_back(GETTEXT(87)); menuIndices.emplace_back(5);
    if (charQueue_.size() > 1) {
        n.emplace_back(GETTEXT(88)); menuIndices.emplace_back(6);
    }
    n.emplace_back(GETTEXT(89)); menuIndices.emplace_back(7);
    n.emplace_back(GETTEXT(90)); menuIndices.emplace_back(8);
    n.emplace_back(GETTEXT(91)); menuIndices.emplace_back(9);
    menu->popup(n, lastMenuIndex_);
    menu->setHandler([this, menu, menuIndices, ch]() {
        auto index = menu->currIndex();
        if (index < 0 || index >= menuIndices.size()) { return; }
        lastMenuIndex_ = index;
        switch (menuIndices[index]) {
        case 0:
            maskSelectableArea(ch->steps, 0);
            stage_ = MoveSelecting;
            drawDirty_ = true;
            break;
        case 1:
            if (ch->info.skillId[1] > 0) {
                std::vector<std::wstring> items;
                std::vector<int> indices;
                for (int i = 0; i < data::LearnSkillCount; ++i) {
                    auto skillId = ch->info.skillId[i];
                    if (skillId <= 0) { continue; }
                    const auto *skillInfo = mem::gSaveData.skillInfo[skillId];
                    if (!skillInfo) { continue; }
                    auto skillLevel =
                        mem::calcRealSkillLevel(skillInfo->reqMp,
                                                std::clamp<std::int16_t>(ch->info.skillLevel[i] / 100, 0, 9),
                                                ch->info.mp);
                    if (skillLevel < 0) { continue; }
                    indices.emplace_back(i);
                    items.emplace_back(GETSKILLNAME(skillId));
                }
                if (!items.empty()) {
                    auto windowBorder = core::config.windowBorder();
                    auto *submenu = new MenuTextList(menu, menu->x() + menu->width() + windowBorder, windowBorder * 4,
                                                     width_ - menu->x() + menu->width() - windowBorder, height_ - windowBorder * 8);
                    submenu->popup(items);
                    submenu->setHandler([this, menu, submenu, indices]() {
                        if (tryUseSkill(indices[submenu->currIndex()])) {
                            delete menu;
                        } else {
                            delete submenu;
                        }
                    });
                    return;
                }
            } else {
                if (tryUseSkill(0)) {
                    delete menu;
                    return;
                }
            }
            {
                auto *msgBox = new MessageBox(this, 0, height_ / 3, width_, 60);
                msgBox->popup({GETTEXT(115)}, MessageBox::PressToCloseThis);
            }
            return;
        case 2:
            if (tryUseSkill(-3)) {
                delete menu;
            }
            return;
        case 3:
            if (tryUseSkill(-2)) {
                delete menu;
            }
            return;
        case 4:
            if (tryUseSkill(-1)) {
                delete menu;
            }
            return;
        case 5: {
            auto windowBorder = core::config.windowBorder();
            auto *iv = new ItemView(this, windowBorder * 4, windowBorder * 4, gWindow->width() - windowBorder * 4, gWindow->height() - windowBorder * 4);
            iv->setCharInfo(&ch->info);
            iv->show(true, [this, ch](std::int16_t itemId) {
                if (currentActor_ != ch) { return; }
                if (itemId < 0) {
                    endTurn(ch);
                } else {
                    actIndex_ = itemId;
                    actId_ = -4;
                    actLevel_ = 0;
                    actItemSlot_ = -1;
                    attackTimesLeft_ = 1;
                    maskSelectableArea(0, battle::calcTechniqueRange(ch->info.throwing));
                    stage_ = AttackSelecting;
                    drawDirty_ = true;
                }
            });
            iv->setCloseHandler([this, ch]() {
                if (currentActor_ == ch) { playerMenu(); }
            });
            delete menu;
            return;
        }
        case 6:
            if (currentActor_ != ch) { return; }
            if (const auto ite = std::find(charQueue_.begin(), charQueue_.end(), ch);
                ite != charQueue_.end()) {
                charQueue_.erase(ite);
            }
            charQueue_.insert(charQueue_.begin(), ch);
            currentActor_ = nullptr;
            pendingAutoAction_ = nullptr;
            stage_ = Idle;
            break;
        case 7: {
            std::vector<std::int16_t> idlist;
            for (auto &c: chars_) {
                idlist.emplace_back(c.side == 1 ? -c.id : c.id);
            }
            auto *svmenu = new CharListMenu(this, 0, 0, gWindow->width(), gWindow->height());
            svmenu->init({GETTEXT(59)}, idlist, {CharListMenu::LEVEL},
                         [this](std::int16_t charId) {
                             auto *sv = new StatusView(this, 0, 0, 0, 0);
                             bool found = false;
                             for (auto &p: chars_) {
                                 if (p.id == charId && p.side == 0) {
                                     sv->show(&p.info, false);
                                     found = true;
                                     break;
                                 }
                             }
                             if (!found) {
                                 sv->show(charId);
                             }
                             sv->makeCenter(width_, height_, x_, y_);
                         }, nullptr);
            svmenu->makeCenter(width_, height_ * 4 / 5, x_, y_);
            return;
        }
        case 8:
            doRest();
            break;
        case 9:
            autoControl_ = true;
            stage_ = Idle;
            break;
        default:
            return;
        }
        delete menu;
    }, []()->bool {
        return false;
    });
}

void Warfield::maskSelectableArea(int steps, int ranges, bool zoecheck) {
    auto *ch = currentActor_;
    if (!ch) { stage_ = Idle; return; }
    getSelectableArea(ch, selCells_, steps, ranges, zoecheck);
    int w = mapWidth_;
    for (auto &c: selCells_) {
        auto &ci = cellInfo_[c.first.first + c.first.second * w];
        ci.insideMovingArea = true;
    }
    cursorX_ = ch->x;
    cursorY_ = ch->y;
}

void Warfield::unmaskArea() {
    int w = mapWidth_;
    for (auto c: selCells_) {
        cellInfo_[c.first.first + c.first.second * w].insideMovingArea = false;
    }
    selCells_.clear();
}

void Warfield::getSelectableArea(CharInfo *ch, std::map<std::pair<int, int>, SelectableCell> &selCells, int steps, int ranges, bool zoecheck) {
    battle::getSelectableArea(
        mapWidth_, mapHeight_, {ch->x, ch->y}, steps, ranges, selCells,
        [this](int x, int y) { return cellInfo_[y * mapWidth_ + x].blocked; },
        [this](int x, int y) { return cellInfo_[y * mapWidth_ + x].charInfo != nullptr; },
        [this, ch, zoecheck](int x, int y) {
            if (!zoecheck) { return false; }
            const auto *other = cellInfo_[y * mapWidth_ + x].charInfo;
            return other != nullptr && other->side == ch->side;
        });
}

class DirectionSelMessageBox: public MessageBox {
public:
    using MessageBox::MessageBox;

    void setDirectionHandler(const std::function<void(Map::Direction)> &func) {
        directionHandler_ = func;
    }
    void handleKeyInput(Key key) override {
        switch (key) {
        case KeyUp:
            directionHandler_(Map::DirUp);
            delete this;
            break;
        case KeyLeft:
            directionHandler_(Map::DirLeft);
            delete this;
            break;
        case KeyRight:
            directionHandler_(Map::DirRight);
            delete this;
            break;
        case KeyDown:
            directionHandler_(Map::DirDown);
            delete this;
            break;
        case KeyCancel: {
            auto fn = std::move(closeHandler_);
            delete this;
            if (fn) { fn(); }
            break;
        }
        default:
            break;
        }
    }

private:
    std::function<void(Map::Direction)> directionHandler_;
};

bool Warfield::tryUseSkill(int index) {
    auto *ch = currentActor_;
    if (!ch) { return false; }
    clearActionState(false);
    if (index < 0) {
        actIndex_ = -1;
        actId_ = index;
        actLevel_ = 0;
        attackTimesLeft_ = 1;
        int steps;
        switch (index) {
        case -3:
            steps = battle::calcTechniqueRange(ch->info.poison);
            break;
        case -2:
            steps = battle::calcTechniqueRange(ch->info.depoison);
            break;
        case -1:
            steps = battle::calcTechniqueRange(ch->info.medic);
            break;
        default:
            steps = 1;
            break;
        }
        maskSelectableArea(0, steps);
        stage_ = AttackSelecting;
        drawDirty_ = true;
        return true;
    }
    const auto *skill = mem::gSaveData.skillInfo[std::max<std::int16_t>(ch->info.skillId[index], 0)];
    if (!skill) { return false; }
    auto skillLevel = std::clamp<std::int16_t>(ch->info.skillLevel[index] / 100, 0, 9);
    skillLevel = mem::calcRealSkillLevel(skill->reqMp, skillLevel, ch->info.mp);
    if (skillLevel < 0) { return false; }
    actIndex_ = index;
    actId_ = ch->info.skillId[index];
    attackTimesLeft_ = battle::attackCount(ch->info.doubleAttack);
    actLevel_ = skillLevel;
    switch (skill->attackAreaType) {
    case 1: {
        auto msgBox = new DirectionSelMessageBox(this, 0, 0, gWindow->width(), gWindow->height());
        msgBox->popup({GETTEXT(92)});
        msgBox->setCloseHandler([this]() {
            clearActionState(false);
            playerMenu();
        });
        msgBox->setDirectionHandler([this, ch](Map::Direction direction) {
            ch->direction = direction;
            startActAction();
        });
        return true;
    }
    case 2:
        startActAction();
        return true;
    default:
        maskSelectableArea(0, skill->selRange[actLevel_]);
        stage_ = AttackSelecting;
        drawDirty_ = true;
        return true;
    }
}

void Warfield::startActAction() {
    popupNumbers_.clear();
    auto *ch = currentActor_;
    if (!ch) { stage_ = Idle; return; }
    if (actId_ < 0) {
        if (cursorX_ < 0 || cursorX_ >= mapWidth_
            || cursorY_ < 0 || cursorY_ >= mapHeight_) {
            if (ch->side == 0) {
                clearActionState(false);
                playerMenu();
            }
            else { endTurn(ch); }
            return;
        }
        auto *target = cellInfo_[cursorY_ * mapWidth_ + cursorX_].charInfo;
        if (!target) {
            if (ch->side == 0) {
                clearActionState(false);
                playerMenu();
            }
            else { endTurn(ch); }
            return;
        }
        std::int16_t result;
        std::uint8_t r, g, b;
        auto *ttf = renderer_->ttf();
        bool popup;
        switch (actId_) {
        case -3:
            effectId_ = data::PoisonEffectID;
            popup = target && ch->side != target->side;
            result = popup ? mem::actPoison(&ch->info, &target->info, 0) : 0;
            popup = popup && result != 0;
            r = 96; g = 176; b = 64;
            break;
        case -2:
            effectId_ = data::DepoisonEffectID;
            popup = target && ch->side == target->side;
            result = popup ? mem::actDepoison(&ch->info, &target->info, 0) : 0;
            r = 104; g = 192; b = 232;
            break;
        case -1:
            effectId_ = data::MedicEffectID;
            popup = target && ch->side == target->side;
            result = popup ? mem::actMedic(&ch->info, &target->info, 2) : 0;
            r = 236; g = 200; b = 40;
            break;
        default: {
            const auto *itemInfo = mem::gSaveData.itemInfo[actIndex_];
            effectId_ = itemInfo ? itemInfo->throwingEffectId : data::PoisonEffectID;
            popup = target && ch->side != target->side;
            bool dead = false;
            result = popup ? mem::actThrow(&ch->info, &target->info, actIndex_, 0, dead) : 0;
            if (popup) {
                if (ch->side == 0) { mem::gBag.remove(actIndex_, 1); }
                else {
                    mem::consumeNpcItemAt(&ch->info, actItemSlot_, actIndex_);
                }
            }
            popup = popup && result != 0;
            if (dead) {
                recalcKnowledge();
            }
            r = 232; g = 32; b = 44;
            break;
        }
        }
        if (popup) {
            auto txt = fmt::format(L"{:+}", result);
            popupNumbers_.emplace_back(PopupNumber{txt, cursorX_, cursorY_, r, g, b});
        }
        if (actId_ >= -3 && actId_ <= -1) {
            battle::finishUtilityAction(ch->info, ch->exp);
        }
        stage_ = Acting;
        if (cameraX_ != cursorX_ || cameraY_ != cursorY_) {
            ch->direction = calcDirection(cameraX_, cameraY_, cursorX_, cursorY_);
        }
        fightTex_ = ch->info.headId >= 0 && ch->info.headId < fightTexData_.size()
            ? &fightTexData_[ch->info.headId] : nullptr;
        fightTexCount_ = ch->info.frame[0];
        fightTexIdx_ = fightTexCount_ * int(ch->direction);
        fightTexCount_ += fightTexIdx_;
        effectTexIdx_ = -ch->info.frameDelay[0];
        fightFrame_ = -ch->info.frameSoundDelay[0];
        return;
    }
    const auto *skillInfo = mem::gSaveData.skillInfo[actId_];
    if (skillInfo) {
        bool levelup = false;
        effectId_ = skillInfo->effectId;
        auto skillType = skillInfo->skillType;
        stage_ = Acting;
        if ((skillInfo->attackAreaType == 0 || skillInfo->attackAreaType == 3)
            && (cameraX_ != cursorX_ || cameraY_ != cursorY_)) {
            ch->direction = calcDirection(cameraX_, cameraY_, cursorX_, cursorY_);
        }
        fightTex_ = ch->info.headId >= 0 && ch->info.headId < fightTexData_.size()
                    ? &fightTexData_[ch->info.headId] : nullptr;
        fightTexIdx_ = 0;
        for (std::int16_t i = 0; i < skillType; ++i) {
            fightTexIdx_ += 4 * ch->info.frame[i];
        }
        fightTexCount_ = ch->info.frame[skillType];
        fightTexIdx_ += fightTexCount_ * int(ch->direction);
        fightTexCount_ += fightTexIdx_;
        effectTexIdx_ = -ch->info.frameDelay[skillType];
        fightFrame_ = -ch->info.frameSoundDelay[skillType];

        battle::AttackDirection attackDirection = battle::AttackDirection::Up;
        switch (ch->direction) {
        case Map::DirRight: attackDirection = battle::AttackDirection::Right; break;
        case Map::DirDown: attackDirection = battle::AttackDirection::Down; break;
        case Map::DirLeft: attackDirection = battle::AttackDirection::Left; break;
        case Map::DirUp: break;
        }
        const auto attackCells = battle::enumerateAttackCells(
            mapWidth_, mapHeight_, cameraX_, cameraY_, cursorX_, cursorY_,
            skillInfo->attackAreaType, skillInfo->selRange[actLevel_],
            skillInfo->area[actLevel_], attackDirection);
        for (const auto &cell: attackCells) {
            makeDamage(ch, cell.x, cell.y, cell.distance);
        }
        mem::postDamage(&ch->info, actIndex_, actLevel_,
                        attackTimesLeft_ == 1 ? 3 : 0, skillLevelup_);
        if (skillLevelup_) {
            actLevel_ = std::clamp<std::int16_t>(ch->info.skillLevel[actIndex_] / 100, 0, 9);
        }
    } else {
        actIndex_ = actId_ = -1;
        actLevel_ = 0;
        actItemSlot_ = -1;
        skillLevelup_ = false;
        effectId_ = -1;
        effectTexIdx_ = -1;
        fightTexIdx_ = -1;
        fightTexCount_ = 0;
        fightFrame_ = 0;
        attackTimesLeft_ = 0;
        fightTex_ = nullptr;
        endTurn(ch);
    }
}

void Warfield::makeDamage(Warfield::CharInfo *ch, int x, int y, int distance) {
    auto *info = cellInfo_[y * mapWidth_ + x].charInfo;
    if (!info || info->side == ch->side) { return; }
    auto &enemyInfo = info->info;
    std::int16_t dmg, ps, exp;
    bool dead = false;
    bool wasDead = enemyInfo.hp <= 0;
    if (mem::actDamage(&ch->info, &enemyInfo, knowledge_[ch->side], knowledge_[ch->side ^ 1],
                       distance, actIndex_, actLevel_, dmg, ps, exp, dead)) {
        ch->exp += exp;
        if (!wasDead && dead) {
            recalcKnowledge();
        }
        auto *ttf = renderer_->ttf();
        const auto *skillInfo = actId_ > 0 ? mem::gSaveData.skillInfo[actId_] : nullptr;
        if (skillInfo && battle::isDrainSkill(*skillInfo)) {
            auto txt = fmt::format(L"{:+}", -dmg);
            popupNumbers_.emplace_back(PopupNumber{txt, x, y, 112, 12, 112});
        } else {
            auto txt = fmt::format(L"{:+}", -dmg);
            popupNumbers_.emplace_back(PopupNumber{txt, x, y, 232, 32, 44});
        }
    }
}

void Warfield::doRest(CharInfo *expectedActor) {
    auto *ch = currentActor_;
    if (!ch || (expectedActor && expectedActor != ch)) { return; }
    if (ch->info.hp <= 0) {
        endTurn(ch);
        return;
    }
    mem::actRest(&ch->info, battle::hasMoved(ch->initialSteps, ch->steps));
    endTurn(ch);
}

void Warfield::endTurn(CharInfo *expectedActor) {
    auto *ch = currentActor_;
    if (!ch || (expectedActor && expectedActor != ch)) { return; }
    const auto ite = std::find(charQueue_.begin(), charQueue_.end(), ch);
    if (ite != charQueue_.end()) {
        charQueue_.erase(ite);
    }
    currentActor_ = nullptr;
    pendingAutoAction_ = nullptr;
    movingPath_.clear();
    resumeAutoAttack_ = false;
    clearActionState(false);
    for (auto &ci: chars_) {
        if (!battle::shouldClearDeadPosition(ci.info.hp, ci.x, ci.y)) {
            continue;
        }
        if (ci.x < mapWidth_ && ci.y < mapHeight_) {
            auto &cell = cellInfo_[ci.x + ci.y * mapWidth_];
            if (cell.charInfo == &ci) { cell.charInfo = nullptr; }
        }
        ci.x = ci.y = -1;
        drawDirty_ = true;
    }
    if (checkWarEnd()) { return; }
    stage_ = Idle;
}

bool Warfield::checkWarEnd() {
    int aliveCount[2] = {0, 0};
    for (const auto &ci: chars_) {
        if (ci.info.hp > 0) { ++aliveCount[ci.side]; }
    }
    if (aliveCount[1] == 0) {
        won_ = true;
        endWar();
        return true;
    }
    if (aliveCount[0] == 0) {
        won_ = false;
        endWar();
        return true;
    }
    return false;
}

void Warfield::endWar() {
    currentActor_ = nullptr;
    pendingAutoAction_ = nullptr;
    movingPath_.clear();
    resumeAutoAttack_ = false;
    clearActionState(false);
    removeAllChildren();
    fadeNode_ = nullptr;
    fadePostAction_ = nullptr;
    runFadePostAction_ = false;
    std::vector<CharInfo*> alives;
    for (auto &ci: chars_) {
        if (ci.side != 0) { continue; }
        auto *charInfo = mem::gSaveData.charInfo[ci.id];
        if (!charInfo) { continue; }
        charInfo->maxMp = battle::mergeBattleMaxMpGrowth(
            ci.persistentEntryMaxMp, ci.battleEntryMaxMp, ci.info.maxMp);
        charInfo->mp = std::clamp<std::int16_t>(
            ci.info.mp, 0, charInfo->maxMp);
        charInfo->poisoned = ci.info.poisoned;
        charInfo->hurt = ci.info.hurt;
        charInfo->stamina = ci.info.stamina;
        const auto floorHp = std::int16_t(ci.info.maxHp / 5);
        if (ci.info.hp > 0) {
            charInfo->hp = std::max<std::int16_t>(ci.info.hp, floorHp);
        } else {
            charInfo->hp = floorHp;
            charInfo->stamina = std::max<std::int16_t>(charInfo->stamina, 10);
        }
        for (int i = 0; i < data::LearnSkillCount; ++i) {
            if (ci.info.skillId[i] <= 0) { continue; }
            charInfo->skillLevel[i] = ci.info.skillLevel[i];
        }
        if (ci.info.hp > 0) { alives.push_back(&ci); }
    }
    const auto *info = data::gWarfieldData.info(warId_);
    const auto wexp = info != nullptr ? info->exp : 0;
    std::vector<std::pair<int, std::wstring>> messages = {{0, GETTEXT(won_ ? 93 : 94)}};
    /* The battlefield bonus is victory-only; the loss flag gates the later
     * level/training/crafting steps, not this shared bonus. */
    if (won_ && !alives.empty()) {
        for (auto *ch: alives) {
            ch->exp += wexp / static_cast<int>(alives.size());
        }
    }
    for (auto &ci: chars_) {
        if (ci.side != 0) { continue; }
        auto *ch = &ci;
        auto *charInfo = mem::gSaveData.charInfo[ch->id];
        if (!charInfo) { continue; }
        const int exp = ch->exp;
        const int exp2 = ch->exp * 8 / 10;
        charInfo->exp = std::uint16_t(
            std::clamp<int>(int(charInfo->exp) + exp, 0, data::ExpMax));
        charInfo->expForItem = std::uint16_t(
            std::clamp<int>(int(charInfo->expForItem) + exp2, 0, data::ExpMax));
        charInfo->expForMakeItem = std::uint16_t(
            std::clamp<int>(int(charInfo->expForMakeItem) + exp2, 0, data::ExpMax));
        if (!won_ && !getExpOnLose_) { continue; }

        const auto name = GETCHARNAME(ch->id);
        messages.emplace_back(std::make_pair(0, fmt::format(GETTEXT(95), name, ch->exp)));
        bool canLearn = false;
        bool makingItem = false;
        std::int16_t skillId = 0;
        int skillLevel = 0;
        const mem::ItemData *itemInfo = nullptr;
        if (charInfo->learningItem >= 0) {
            itemInfo = mem::gSaveData.itemInfo[charInfo->learningItem];
            if (itemInfo) {
                makingItem = true;
                canLearn = true;
                skillId = itemInfo->skillId;
                if (skillId > 0) {
                    for (int i = 0; i < data::LearnSkillCount; ++i) {
                        if (charInfo->skillId[i] != skillId) { continue; }
                        skillLevel = std::clamp<std::int16_t>(
                            charInfo->skillLevel[i] / 100, 0, data::SkillLevelMaxDiv);
                        if (skillLevel >= data::SkillLevelMaxDiv) { canLearn = false; }
                        break;
                    }
                }
            }
        }

        if (charInfo->level < data::LevelMax) {
            int gained = 0;
            std::uint16_t expReq;
            while (charInfo->level + gained < data::LevelMax
                   && (expReq = mem::getExpForLevelUp(charInfo->level + gained)) > 0
                   && charInfo->exp >= expReq) {
                ++gained;
            }
            if (gained > 0) {
                mem::actLevelup(charInfo, gained);
                messages.emplace_back(std::make_pair(0, fmt::format(GETTEXT(96), name)));
            }
        }

        if (canLearn && itemInfo) {
            const auto expReq = mem::getExpForSkillLearn(
                charInfo->learningItem, skillLevel, charInfo->potential);
            if (expReq > 0 && charInfo->expForItem >= expReq) {
                mem::applyBookChanges(charInfo, itemInfo);
                charInfo->expForItem = 0;
                messages.emplace_back(std::make_pair(0, fmt::format(
                    GETTEXT(97), name, GETITEMNAME(charInfo->learningItem))));
                if (skillId > 0) {
                    bool known = false;
                    for (int i = 0; i < data::LearnSkillCount; ++i) {
                        if (charInfo->skillId[i] != skillId) { continue; }
                        known = true;
                        if (charInfo->skillLevel[i] >= data::SkillLevelMaxDiv * 100 - 1) {
                            continue;
                        }
                        charInfo->skillLevel[i] = std::int16_t(charInfo->skillLevel[i] + 100);
                        messages.emplace_back(std::make_pair(1, fmt::format(
                            GETTEXT(98), GETSKILLNAME(skillId),
                            charInfo->skillLevel[i] / 100 + 1)));
                    }
                    if (!known) {
                        for (int i = 0; i < data::LearnSkillCount; ++i) {
                            if (charInfo->skillId[i] > 0) { continue; }
                            charInfo->skillId[i] = skillId;
                            break;
                        }
                    }
                }
            }
        }

        if (makingItem && itemInfo) {
            const auto craftReq = mem::getExpForMakeItem(
                charInfo->learningItem, charInfo->potential);
            const auto material = mem::gBag[itemInfo->reqMaterial];
            bool affordable[data::MakeItemCount] = {false};
            bool anyAffordable = false;
            for (int i = 0; i < data::MakeItemCount; ++i) {
                if (itemInfo->makeItem[i] < 0
                    || material < itemInfo->makeItemCount[i]) {
                    continue;
                }
                affordable[i] = true;
                anyAffordable = true;
            }
            if (craftReq > 0 && charInfo->expForMakeItem >= craftReq && anyAffordable) {
                int index;
                do {
                    index = int(util::gRandom(data::MakeItemCount));
                } while (!affordable[index]);
                const auto produced = mem::gBag[itemInfo->makeItem[index]] > 0
                    ? std::int16_t(util::gRandom(3) + 1)
                    : std::int16_t(1);
                charInfo->expForMakeItem = 0;
                mem::gBag.add(itemInfo->makeItem[index], produced);
                mem::gBag.remove(itemInfo->reqMaterial, itemInfo->makeItemCount[index]);
                messages.emplace_back(std::make_pair(0, fmt::format(
                    GETTEXT(99), name, GETITEMNAME(itemInfo->makeItem[index]))));
            }
        }
    }
    stage_ = Finished;
    popupFinishMessages(std::move(messages), 0);
    delete statusPanel_;
    statusPanel_ = nullptr;
}

void Warfield::popupFinishMessages(std::vector<std::pair<int, std::wstring>> messages, int index) {
    int y = height_ / 3;
    auto *msgBox = new MessageBox(this, 0, y, width_, 60);
    msgBox->popup({messages[index].second}, MessageBox::PressToCloseThis);
    ++index;
    auto *lastMsgBox = msgBox;
    while (index < messages.size() && messages[index].first > 0) {
        auto *msgBox2 = new MessageBox(msgBox, 0, y + 60 * messages[index].first, width_, 60);
        msgBox2->popup({messages[index].second}, MessageBox::PressToCloseParent);
        lastMsgBox = msgBox2;
        ++index;
    }
    lastMsgBox->setCloseHandler([this, messages = std::move(messages), index]() {
        if (index < messages.size()) {
            popupFinishMessages(messages, index);
        } else {
            const bool won = won_;
            const bool instantDie = !won && deadOnLose_;
            cleanup();
            gWindow->endWar(won, instantDie);
        }
    });
}

}
