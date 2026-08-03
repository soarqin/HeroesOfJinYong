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

#include "colorpalette.hh"
#include "menu.hh"
#include "charlistmenu.hh"
#include "statusview.hh"
#include "itemview.hh"
#include "window.hh"
#include "effect.hh"
#include "data/grpdata.hh"
#include "data/warfielddata.hh"
#include "battle/ai.hh"
#include "battle/game_random.hh"
#include "battle/turn_order.hh"
#include "mem/savedata.hh"
#include "mem/strings.hh"
#include "core/config.hh"
#include "util/random.hh"
#include <fmt/xchar.h>
#include <map>
#include <algorithm>
#include <climits>
#include <utility>

namespace hojy::scene {

namespace {

/* Shared adapter so the pure battle ai uses the game random source. */
battle::RandomSource &aiRandom() {
    static battle::GameRandom random;
    return random;
}

}

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

void Warfield::cleanup() {
    removeAllChildren();
    fadeNode_ = nullptr;
    fadePostAction_ = nullptr;
    runFadePostAction_ = false;
    pendingAutoAction_ = nullptr;
    currentActor_ = nullptr;
    charQueue_.clear();
    turnOrder_.clear();
    chars_.clear();
    round_ = 0;
    stage_ = Idle;
    knowledge_[0] = knowledge_[1] = 0;
    cursorX_ = 0;
    cursorY_ = 0;
    autoControl_ = false;
    won_ = false;
    skillLevelup_ = false;
    selCells_.clear();
    movingPath_.clear();
    actIndex_ = -1;
    actId_ = -1;
    actLevel_ = 0;
    effectId_ = -1;
    effectTexIdx_ = -1;
    fightTexIdx_ = -1;
    fightTexCount_ = 0;
    fightFrame_ = 0;
    attackTimesLeft_ = 0;
    fightTex_ = nullptr;
    popupNumbers_.clear();
}

bool Warfield::load(std::int16_t warId) {
    cleanup();

    warId_ = warId;
    const auto *info = data::gWarfieldData.info(warId);
    auto warMapId = info->warFieldId;
    const auto &layers = data::gWarfieldData.layers(warMapId)->layers;
    if (warMapLoaded_.find(warMapId) == warMapLoaded_.end()) {
        mapWidth_ = data::WarFieldWidth;
        mapHeight_ = data::WarFieldHeight;
        if (data::GrpData::loadData("WDX", "WMP", texData_)) {
            for (std::int16_t i = 0; i < 1000; ++i) {
                warMapLoaded_.insert(i);
            }
        } else {
            if (!data::GrpData::loadData(fmt::format("WDX{:03}", warMapId), fmt::format("WMP{:03}", warMapId), texData_)) {
                return false;
            }
            warMapLoaded_.insert(warMapId);
        }
    }
    {
        const auto *arr = reinterpret_cast<const uint16_t*>(texData_[0].data());
        cellWidth_ = arr[0];
        cellHeight_ = arr[1];
        offsetX_ = arr[2];
        offsetY_ = arr[3];
    }
    int cellDiffX = cellWidth_ / 2;
    int cellDiffY = cellHeight_ / 2;
    auto size = mapWidth_ * mapHeight_;
    cellInfo_.clear();
    cellInfo_.resize(size);

    int x = (mapHeight_ - 1) * cellDiffX + offsetX_;
    int y = offsetY_;
    int pos = 0;
    for (int j = mapHeight_; j; --j) {
        int tx = x, ty = y;
        for (int i = mapWidth_; i; --i, ++pos, tx += cellDiffX, ty += cellDiffY) {
            auto &ci = cellInfo_[pos];
            auto texId = layers[0][pos] >> 1;
            ci.earthId = texId;
            ci.buildingId = layers[1][pos] >> 1;
            ci.blocked = ci.buildingId > 0 || texId >= 179 && texId <= 181 || texId == 261 || texId == 511 || texId >= 662 && texId <= 665 || texId == 674;
        }
        x -= cellDiffX; y += cellDiffY;
    }

    subMapId_ = warMapId;
    resetFrame();
    if (!statusPanel_) {
        statusPanel_ = new StatusView(renderer_, x_, y_, width_, height_);
    }
    return true;
}

bool Warfield::getDefaultChars(std::set<std::int16_t> &chars) const {
    const auto *info = data::gWarfieldData.info(warId_);
    if (info->forceMembers[0] >= 0) { return false; }
    for (auto &id: info->defaultMembers) {
        if (id >= 0) { chars.insert(id); }
    }
    return true;
}

void Warfield::putChars(const std::vector<std::int16_t> &chars) {
    const auto *info = data::gWarfieldData.info(warId_);
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
        auto &cell = cellInfo_[ci.y * mapWidth_ + ci.x];
        /* NOTE: remove duplicate chars */
        if (cell.charInfo != nullptr) {
            ite = chars_.erase(ite);
            continue;
        }
        mem::addUpPropFromEquipToChar(&ci.info);
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
        if (acting && ch && effectTexIdx_ >= 0) {
            const auto *skillInfo = actId_ > 0 ? mem::gSaveData.skillInfo[actId_] : nullptr;
            const auto &effTexData = gEffect[effectId_];
            if (!effTexData.empty()) {
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
                Texture::renderRLE(texData_[ci.earthId], colors, pixels, pitch, aheight, dx, ty);
                if (!movingOrActing) {
                    static std::uint32_t maskColors[256] = {0};
                    if (ci.insideMovingArea == 2) {
                        maskColors[254] = 0xA0A0A0A0u;
                        Texture::renderRLEBlending(texData_[0], maskColors, pixels, pitch, aheight, dx, ty);
                    } else if (ci.charInfo) {
                        maskColors[254] = 0x80A0A0A0u;
                        Texture::renderRLEBlending(texData_[0], maskColors, pixels, pitch, aheight, dx, ty);
                    } else if (selecting && !ci.insideMovingArea) {
                        maskColors[254] = 0xD0A0A0A0u;
                        Texture::renderRLEBlending(texData_[0], maskColors, pixels, pitch, aheight, dx, ty);
                    }
                }
                if (ci.buildingId > 0) {
                    Texture::renderRLE(texData_[ci.buildingId], colors, pixels2, pitch2, aheight, dx, ty);
                } else {
                    if (ci.charInfo) {
                        if (acting && ci.charInfo == ch && fightTex_ && fightTexIdx_ >= 0 && fightTexIdx_ < fightTex_->size()) {
                            Texture::renderRLE((*fightTex_)[fightTexIdx_], colors, pixels2, pitch2, aheight, dx, ty);
                        } else {
                            Texture::renderRLE(texData_[2553 + 4 * ci.charInfo->texId
                                + int(ci.charInfo->direction)], colors, pixels2, pitch2, aheight, dx, ty);
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
        statusPanel_->render();
    }
}

void Warfield::handleKeyInput(Node::Key key) {
    if ((stage_ == MoveSelecting || stage_ == AttackSelecting) && !currentActor_) {
        stage_ = Idle;
        return;
    }
    if (stage_ != MoveSelecting && stage_ != AttackSelecting) {
        if (key == KeyCancel) {
            if (currentActor_ && currentActor_->side == 0) {
                pendingAutoAction_ = nullptr;
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
        if (movingPath_.empty() || !currentActor_) { stage_ = Idle; break; }
        int x, y;
        std::tie(x, y) = movingPath_.back();
        if (x == cameraX_ && y == cameraY_) {
            movingPath_.pop_back();
            std::tie(x, y) = movingPath_.back();
        }
        movingPath_.pop_back();
        auto &ci = cellInfo_[cameraX_ + cameraY_ * mapWidth_];
        auto &newci = cellInfo_[x + y * mapWidth_];
        auto *charInfo = ci.charInfo;
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
        break;
    }
    case Acting: {
        if (!currentActor_) { stage_ = Idle; break; }
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
                    const auto *skill = mem::gSaveData.skillInfo[actId_];
                    if (skill) {
                        /*
                         * Z.DAT:0x38555 repeats unconditionally and the level
                         * resolution inside the damage routine always lands on a
                         * usable level, so a drained actor still strikes at its
                         * cheapest level instead of losing the second hit.
                         */
                        actLevel_ = std::max<std::int16_t>(
                            0, mem::calcRealSkillLevel(skill->reqMp, actLevel_, actor->info.mp));
                    }
                    startActAction();
                } else {
                    actIndex_ = actId_ = -1;
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
            if (skillLevelup_ && actor) {
                skillLevelup_ = false;
                stage_ = PoppingUp;
                const auto *skill = mem::gSaveData.skillInfo[actId_];
                auto *msgBox = new MessageBox(this, 0, height_ / 3, width_, 60);
                msgBox->popup({fmt::format(GETTEXT(81), GETSKILLNAME(actId_),
                                           actor->info.skillLevel[actIndex_] / 100 + 1)}, MessageBox::PressToCloseThis);
                msgBox->setCloseHandler([this, postFunc]() {
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
                /*
                 * `NUM-ROUND-DRAIN` Z.DAT:0x3C563: the original drains hurt and
                 * poison from every participant once the round is over, not at
                 * the start of each single turn.
                 */
                for (auto &ci: chars_) {
                    mem::actRoundEndDrain(&ci.info, ci.x < 0 || ci.y < 0);
                }
                if (checkWarEnd()) { return; }
            }
            ++round_;
            charQueue_ = battle::buildRoundQueue(turnOrder_,
                [](const CharInfo *actor) { return actor->info.speed; },
                [](const CharInfo *actor) { return actor->info.hp > 0; });
            for (auto *actor: charQueue_) {
                actor->steps = battle::calculateMovementSteps(actor->info.speed, actor->info.hurt);
                actor->initialSteps = actor->steps;
            }
#ifndef NDEBUG
            fmt::print(stdout, "Battle round {} order:", round_);
            for (auto ite = charQueue_.rbegin(); ite != charQueue_.rend(); ++ite) {
                fmt::print(stdout, " {}(speed={},hurt={},steps={})",
                           (*ite)->id, (*ite)->info.speed, (*ite)->info.hurt, (*ite)->steps);
            }
            fmt::print(stdout, "\n");
            fflush(stdout);
#endif
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
    cameraX_ = ch->x;
    cameraY_ = ch->y;
    drawDirty_ = true;
    auto *sv = dynamic_cast<StatusView*>(statusPanel_);
    auto windowBorder = core::config.windowBorder();
    sv->show(&ch->info, false, true);
    sv->forceUpdate();
    sv->setPosition(ch->side == 1 ? windowBorder * 4 : (width_ - windowBorder * 4 - sv->width()), height_ * 2 / 5 - sv->height() / 2);
    if (ch->side == 1 || autoControl_) {
        autoAction();
    } else {
        lastMenuIndex_ = 0;
        playerMenu();
    }
}

void Warfield::runPendingAutoAction() {
    auto action = std::move(pendingAutoAction_);
    if (action) { action(); }
}

void Warfield::autoUseItem(CharInfo *ch, std::int16_t itemId) {
    std::map<mem::PropType, std::int16_t> changes;
    bool usedItem = ch->side == 1 ? mem::useNpcItem(&ch->info, itemId, changes)
                                  : mem::useItem(&ch->info, itemId, changes);
    if (!usedItem) {
        doRest(ch);
        return;
    }
    stage_ = PoppingUp;
    auto *msgBox = ItemView::popupUseResult(this, itemId, changes);
    msgBox->setCloseHandler([this, ch] {
        endTurn(ch);
    });
}

std::vector<std::int16_t> Warfield::rangeGrid(int fromX, int fromY) const {
    /*
     * `AREA-BUILD-RANGE` Z.DAT:0x36E7F: a flood fill that walks around buildings
     * and blocked terrain but ignores who stands where. The cost is uniform, so
     * one fill from the target also answers every candidate cell.
     */
    std::vector<std::int16_t> grid(std::size_t(mapWidth_) * mapHeight_, -1);
    if (fromX < 0 || fromY < 0 || fromX >= mapWidth_ || fromY >= mapHeight_) { return grid; }
    std::vector<std::pair<int, int>> current{{fromX, fromY}}, next;
    grid[std::size_t(fromY) * mapWidth_ + fromX] = 0;
    for (std::int16_t step = 1; !current.empty(); ++step) {
        next.clear();
        for (auto &cell: current) {
            static const int dx[4] = {0, 1, -1, 0};
            static const int dy[4] = {-1, 0, 0, 1};
            for (int i = 0; i < 4; ++i) {
                int tx = cell.first + dx[i], ty = cell.second + dy[i];
                if (tx < 0 || ty < 0 || tx >= mapWidth_ || ty >= mapHeight_) { continue; }
                auto index = std::size_t(ty) * mapWidth_ + tx;
                if (grid[index] >= 0 || cellInfo_[index].blocked) { continue; }
                grid[index] = step;
                next.emplace_back(tx, ty);
            }
        }
        current.swap(next);
    }
    return grid;
}

void Warfield::startMovingTo(std::map<std::pair<int, int>, SelectableCell> &cells, int x, int y) {
    auto ite = cells.find(std::make_pair(x, y));
    if (ite == cells.end()) { return; }
    stage_ = Moving;
    movingPath_.clear();
    for (auto *sc = &ite->second; sc; sc = sc->moveParent) {
        movingPath_.emplace_back(std::make_pair(sc->x, sc->y));
    }
}

bool Warfield::moveAwayFromEnemies(CharInfo *ch) {
    /*
     * `AI-RETREAT` Z.DAT:0x34AEC: only cells that consume the whole movement
     * allowance qualify, and among those the one with the largest total distance
     * to the opposing side wins.
     */
    std::map<std::pair<int, int>, SelectableCell> cells;
    getSelectableArea(ch, cells, ch->steps, 0);
    int best = 0, bx = ch->x, by = ch->y;
    for (auto &cell: cells) {
        if (cell.second.moves != ch->steps) { continue; }
        int total = 0;
        for (auto &other: chars_) {
            if (other.side == ch->side || other.x < 0 || other.y < 0) { continue; }
            total += std::abs(cell.first.first - other.x) + std::abs(cell.first.second - other.y);
        }
        if (total > best) { best = total; bx = cell.first.first; by = cell.first.second; }
    }
    if (bx == ch->x && by == ch->y) { return false; }
    startMovingTo(cells, bx, by);
    return true;
}

bool Warfield::approachAndAct(CharInfo *ch, const CharInfo *target, int range, bool aligned,
                              std::function<void()> act) {
    if (!target || target->x < 0 || target->y < 0 || range < 0) { return false; }
    const auto grid = rangeGrid(target->x, target->y);
    auto distanceAt = [this, &grid](int x, int y) {
        return int(grid[std::size_t(y) * mapWidth_ + x]);
    };
    const int tx = target->x, ty = target->y;
    auto usable = [&](int x, int y) {
        int distance = distanceAt(x, y);
        if (distance < 0 || distance > range) { return false; }
        return !aligned || x == tx || y == ty;
    };
    if (usable(ch->x, ch->y)) {
        pendingAutoAction_ = std::move(act);
        runPendingAutoAction();
        return true;
    }
    if (ch->steps <= 0) { return false; }
    std::map<std::pair<int, int>, SelectableCell> cells;
    getSelectableArea(ch, cells, ch->steps, 0);
    /*
     * Z.DAT:0x36601 walks the standoff distance from the skill range down to 1
     * and takes the cell closest to the current position, so the actor keeps as
     * much distance as the skill allows.
     */
    for (int wanted = range; wanted >= 1; --wanted) {
        int best = -1, bx = -1, by = -1;
        for (auto &cell: cells) {
            if (cell.second.moves < 0) { continue; }
            int x = cell.first.first, y = cell.first.second;
            if (distanceAt(x, y) != wanted) { continue; }
            if (aligned && x != tx && y != ty) { continue; }
            int cost = std::abs(x - ch->x) + std::abs(y - ch->y);
            if (best < 0 || cost < best) { best = cost; bx = x; by = y; }
        }
        if (best < 0) { continue; }
        pendingAutoAction_ = std::move(act);
        startMovingTo(cells, bx, by);
        return true;
    }
    return false;
}

void Warfield::aimAndAct(CharInfo *ch, int x, int y) {
    const auto *skill = actId_ > 0 ? mem::gSaveData.skillInfo[actId_] : nullptr;
    if (skill && skill->attackAreaType == 1) {
        ch->direction = calcDirection(ch->x, ch->y, x, y);
    }
    cursorX_ = x;
    cursorY_ = y;
    startActAction();
}

bool Warfield::autoSupport(CharInfo *ch, CharInfo *target, std::int16_t actId) {
    if (!target) { return false; }
    int range;
    switch (actId) {
    case -3: range = ch->info.poison / 15 + 1; break;
    case -2: range = ch->info.depoison / 15 + 1; break;
    default: range = ch->info.medic / 15 + 1; break;
    }
    const int tx = target->x, ty = target->y;
    return approachAndAct(ch, target, range, false, [this, ch, actId, tx, ty]() {
        actIndex_ = -1;
        actId_ = actId;
        actLevel_ = 0;
        attackTimesLeft_ = 1;
        aimAndAct(ch, tx, ty);
    });
}

bool Warfield::autoThrow(CharInfo *ch, CharInfo *target, std::int16_t itemId) {
    if (!target || itemId < 0) { return false; }
    const int tx = target->x, ty = target->y;
    return approachAndAct(ch, target, ch->info.throwing / 15 + 1, false,
                          [this, ch, itemId, tx, ty]() {
        actIndex_ = itemId;
        actId_ = -4;
        actLevel_ = 0;
        attackTimesLeft_ = 1;
        aimAndAct(ch, tx, ty);
    });
}

bool Warfield::autoAttack(CharInfo *ch, CharInfo *preferred) {
    /* `AI-ATTACK` Z.DAT:0x34C47 picks one of the learnt skills at random. */
    std::int16_t slots[data::LearnSkillCount];
    int count = 0;
    for (int i = 0; i < data::LearnSkillCount; ++i) {
        if (ch->info.skillId[i] > 0) { slots[count++] = std::int16_t(i); }
    }
    if (!count) { return false; }
    auto slot = slots[util::gRandom(count)];
    const auto *skill = mem::gSaveData.skillInfo[ch->info.skillId[slot]];
    if (!skill) { return false; }
    auto level = std::clamp<std::int16_t>(ch->info.skillLevel[slot] / 100, 0, data::SkillLevelMaxDiv);
    level = std::max<std::int16_t>(0, mem::calcRealSkillLevel(skill->reqMp, level, ch->info.mp));
    const int range = skill->selRange[level];
    const bool aligned = skill->attackAreaType == 1 || skill->attackAreaType == 2;
    auto engage = [&](CharInfo *target)->bool {
        if (!target) { return false; }
        const int tx = target->x, ty = target->y;
        return approachAndAct(ch, target, range, aligned,
                              [this, ch, slot, level, tx, ty]() {
            actIndex_ = slot;
            actId_ = ch->info.skillId[slot];
            actLevel_ = level;
            attackTimesLeft_ = ch->info.doubleAttack == 1 ? 2 : 1;
            aimAndAct(ch, tx, ty);
        });
    };
    if (engage(preferred)) { return true; }
    /* Z.DAT:0x34F55 falls back to the nearest enemy once moving did not help. */
    CharInfo *nearest = nullptr;
    int bestDistance = -1;
    const auto grid = rangeGrid(ch->x, ch->y);
    for (auto &other: chars_) {
        if (other.side == ch->side || other.info.hp <= 0 || other.x < 0 || other.y < 0) { continue; }
        int distance = grid[std::size_t(other.y) * mapWidth_ + other.x];
        if (distance < 0) { continue; }
        if (bestDistance < 0 || distance < bestDistance) { bestDistance = distance; nearest = &other; }
    }
    return nearest != preferred && engage(nearest);
}

bool Warfield::moveTowards(CharInfo *ch, const CharInfo *target) {
    if (!target || target->x < 0 || target->y < 0 || ch->steps <= 0) { return false; }
    std::map<std::pair<int, int>, SelectableCell> cells;
    getSelectableArea(ch, cells, ch->steps, 0);
    const auto grid = rangeGrid(target->x, target->y);
    int best = -1, bx = ch->x, by = ch->y;
    for (auto &cell: cells) {
        if (cell.second.moves < 0) { continue; }
        int distance = grid[std::size_t(cell.first.second) * mapWidth_ + cell.first.first];
        if (distance < 0) { continue; }
        if (best < 0 || distance < best) {
            best = distance;
            bx = cell.first.first;
            by = cell.first.second;
        }
    }
    if (bx == ch->x && by == ch->y) { return false; }
    startMovingTo(cells, bx, by);
    return true;
}

battle::AiContext Warfield::buildAiContext(CharInfo *ch) const {
    battle::AiContext context;
    const auto grid = rangeGrid(ch->x, ch->y);
    for (auto &ci: chars_) {
        battle::AiParticipant participant;
        participant.side = ci.side;
        participant.active = ci.info.hp > 0 && ci.x >= 0 && ci.y >= 0;
        participant.request = ci.request;
        participant.distance = participant.active
            ? grid[std::size_t(ci.y) * mapWidth_ + ci.x] : -1;
        auto &stats = participant.stats;
        const auto &info = ci.info;
        stats.hp = info.hp; stats.maxHp = info.maxHp;
        stats.mp = info.mp; stats.maxMp = info.maxMp;
        stats.stamina = info.stamina; stats.hurt = info.hurt; stats.poisoned = info.poisoned;
        stats.attack = info.attack; stats.medic = info.medic; stats.poison = info.poison;
        stats.depoison = info.depoison; stats.antipoison = info.antipoison;
        stats.throwing = info.throwing;
        stats.integrity = info.integrity; stats.potential = info.potential;
        for (int i = 0; i < data::LearnSkillCount; ++i) {
            if (info.skillId[i] <= 0) { continue; }
            const auto *skill = mem::gSaveData.skillInfo[info.skillId[i]];
            if (!skill) { continue; }
            if (stats.minSkillReqMp < 0 || skill->reqMp < stats.minSkillReqMp) {
                stats.minSkillReqMp = skill->reqMp;
            }
        }
        if (&ci == ch) { context.self = int(context.participants.size()); }
        context.participants.emplace_back(participant);
    }
    auto addItem = [&context](std::int16_t itemId) {
        const auto *itemInfo = mem::gSaveData.itemInfo[itemId];
        if (!itemInfo || itemInfo->itemType == 1 || itemInfo->itemType == 2) { return; }
        context.items.emplace_back(battle::AiItem{itemId, itemInfo->addHp, itemInfo->addMp,
                                                 itemInfo->addPoisoned});
    };
    if (ch->side == 1) {
        for (int i = 0; i < data::CarryItemCount; ++i) {
            if (ch->info.item[i] >= 0 && ch->info.itemCount[i] > 0) { addItem(ch->info.item[i]); }
        }
    } else {
        for (auto &entry: mem::gBag.items()) {
            if (entry.first >= 0 && entry.second > 0) { addItem(entry.first); }
        }
    }
    return context;
}

void Warfield::autoAction() {
    if (pendingAutoAction_) {
        runPendingAutoAction();
        return;
    }
    auto *ch = currentActor_;
    if (!ch) { stage_ = Idle; return; }
    /* Z.DAT:0x329D0 drops the pending request when the character's own turn starts. */
    ch->request = battle::AiRequest::None;

    auto context = buildAiContext(ch);
    auto decision = battle::decideAiAction(context, aiRandom());
    ch->request = decision.request;
    auto *target = decision.target >= 0 && decision.target < int(chars_.size())
        ? &chars_[decision.target] : nullptr;
    auto rest = [this, ch]() { doRest(ch); };

    switch (decision.action) {
    case battle::AiAction::Rest:
        pendingAutoAction_ = rest;
        runPendingAutoAction();
        return;
    case battle::AiAction::Flee:
        pendingAutoAction_ = rest;
        if (!moveAwayFromEnemies(ch)) { runPendingAutoAction(); }
        return;
    case battle::AiAction::UseItem: {
        auto itemId = std::int16_t(decision.itemSlot);
        pendingAutoAction_ = [this, ch, itemId]() { autoUseItem(ch, itemId); };
        if (!moveAwayFromEnemies(ch)) { runPendingAutoAction(); }
        return;
    }
    case battle::AiAction::Medic:
        if (autoSupport(ch, target, -1)) { return; }
        break;
    case battle::AiAction::Depoison:
        if (autoSupport(ch, target, -2)) { return; }
        break;
    case battle::AiAction::Poison: {
        int index = battle::pickAiPoisonTarget(context, aiRandom());
        auto *victim = index >= 0 && index < int(chars_.size()) ? &chars_[index] : nullptr;
        if (autoSupport(ch, victim, -3)) { return; }
        break;
    }
    case battle::AiAction::Throw: {
        int index = battle::pickAiTarget(context, aiRandom());
        auto *victim = index >= 0 && index < int(chars_.size()) ? &chars_[index] : nullptr;
        if (autoThrow(ch, victim, std::int16_t(decision.itemSlot))) { return; }
        break;
    }
    case battle::AiAction::Attack:
        break;
    }

    /*
     * Z.DAT:0x36366: when the support action could not be carried out, the actor
     * attacks if it is worth more than the average of its own side and rests
     * otherwise.
     */
    int index = battle::pickAiTarget(context, aiRandom());
    auto *victim = index >= 0 && index < int(chars_.size()) ? &chars_[index] : nullptr;
    int ownTotal = 0, ownCount = 0;
    for (auto &ci: chars_) {
        if (ci.side != ch->side) { continue; }
        ownTotal += ci.info.attack + ci.info.hp;
        ++ownCount;
    }
    const bool worthAttacking = decision.action == battle::AiAction::Attack
        || ownCount == 0 || ch->info.attack * 2 > ownTotal * 2 / ownCount;
    if (worthAttacking && autoAttack(ch, victim)) { return; }
    pendingAutoAction_ = rest;
    if (!moveTowards(ch, victim)) { runPendingAutoAction(); }
}

void Warfield::recalcKnowledge() {
    knowledge_[0] = knowledge_[1] = 0;
    for (auto &ci: chars_) {
        /* Z.DAT:0x3919E compares with `jle`, so the barrier is exclusive. */
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
    /*
     * Menu gates from Z.DAT:0x32EA4-0x32FE7. All stamina comparisons use `jle`,
     * so the thresholds are exclusive, and the three support skills need a
     * proficiency of at least 20.
     */
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
                    attackTimesLeft_ = 1;
                    /* Z.DAT:0x3A33F adds one to the derived range. */
                    maskSelectableArea(0, ch->info.throwing / 15 + 1);
                    stage_ = AttackSelecting;
                    drawDirty_ = true;
                }
            });
            iv->setCloseHandler([this]() { playerMenu(); });
            delete menu;
            return;
        }
        case 6: {
            auto ite = std::find(charQueue_.begin(), charQueue_.end(), ch);
            if (ite != charQueue_.end()) {
                charQueue_.erase(ite);
                charQueue_.insert(charQueue_.begin(), ch);
            }
            currentActor_ = nullptr;
            pendingAutoAction_ = nullptr;
            stage_ = Idle;
            break;
        }
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
            doRest(ch);
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
    struct CompareSelCells {
        bool operator()(const SelectableCell *a, const SelectableCell *b) {
            return a->moves > b->moves;
        }
    };
    auto myside = ch->side;
    int w = mapWidth_, h = mapHeight_;
    std::vector<SelectableCell*> sortedMovable;

    selCells.clear();
    auto &start = selCells[std::make_pair(ch->x, ch->y)];
    start.x = ch->x;
    start.y = ch->y;
    start.moves = 0;
    start.ranges = 0;
    start.moveParent = nullptr;
    start.rangeParent = nullptr;
    if (steps > 0) {
        sortedMovable.push_back(&start);
    }
    while (!sortedMovable.empty()) {
        std::pop_heap(sortedMovable.begin(), sortedMovable.end(), CompareSelCells());
        auto *mc = sortedMovable.back();
        sortedMovable.erase(sortedMovable.end() - 1);
        bool zoeblocked = false;
        int nx[4], ny[4], ncnt = 0;
        for (int i = 0; i < 4; ++i) {
            int tx, ty;
            switch (i) {
            case 0:
                if (mc->y <= 0) { continue; }
                tx = mc->x;
                ty = mc->y - 1;
                break;
            case 1:
                if (mc->x + 1 >= w) { continue; }
                tx = mc->x + 1;
                ty = mc->y;
                break;
            case 2:
                if (mc->x <= 0) { continue; }
                tx = mc->x - 1;
                ty = mc->y;
                break;
            default:
                if (mc->y + 1 >= h) { continue; }
                tx = mc->x;
                ty = mc->y + 1;
                break;
            }
            if (zoecheck) {
                auto &ci = cellInfo_[ty * w + tx];
                if (ci.charInfo && ci.charInfo->side == myside) {
                    zoeblocked = true;
                    break;
                }
            }
            nx[ncnt] = tx;
            ny[ncnt] = ty;
            ++ncnt;
        }
        if (zoeblocked) { continue; }
        for (int i = 0; i < ncnt; ++i) {
            int tx = nx[i], ty = ny[i];
            auto &ci = cellInfo_[ty * w + tx];
            if (ci.charInfo || ci.blocked) {
                continue;
            }
            auto currMove = mc->moves + 1;
            auto ite = selCells.find(std::make_pair(tx, ty));
            if (ite == selCells.end()) {
                auto &mcell = selCells[std::make_pair(tx, ty)];
                mcell.x = tx;
                mcell.y = ty;
                mcell.moves = currMove;
                mcell.moveParent = mc;
                if (currMove < steps) {
                    sortedMovable.push_back(&mcell);
                    std::push_heap(sortedMovable.begin(), sortedMovable.end(), CompareSelCells());
                }
            }
        }
    }
    if (ranges) {
        struct CompareRangeCells {
            bool operator()(const SelectableCell *a, const SelectableCell *b) {
                return a->ranges > b->ranges;
            }
        };
        std::vector<SelectableCell*> sortedAttackable;
        sortedAttackable.reserve(selCells.size());
        for (auto &p: selCells) {
            sortedAttackable.push_back(&p.second);
        }
        std::make_heap(sortedAttackable.begin(), sortedAttackable.end(), CompareRangeCells());
        while (!sortedAttackable.empty()) {
            std::pop_heap(sortedAttackable.begin(), sortedAttackable.end(), CompareRangeCells());
            auto *mc = sortedAttackable.back();
            sortedAttackable.erase(sortedAttackable.end() - 1);
            int nx[4], ny[4], ncnt = 0;
            for (int i = 0; i < 4; ++i) {
                int tx, ty;
                switch (i) {
                case 0:
                    if (mc->x <= 0) { continue; }
                    tx = mc->x - 1;
                    ty = mc->y;
                    break;
                case 1:
                    if (mc->y <= 0) { continue; }
                    tx = mc->x;
                    ty = mc->y - 1;
                    break;
                case 2:
                    if (mc->x + 1 >= w) { continue; }
                    tx = mc->x + 1;
                    ty = mc->y;
                    break;
                default:
                    if (mc->y + 1 >= h) { continue; }
                    tx = mc->x;
                    ty = mc->y + 1;
                    break;
                }
                nx[ncnt] = tx;
                ny[ncnt] = ty;
                ++ncnt;
            }
            for (int i = 0; i < ncnt; ++i) {
                int tx = nx[i], ty = ny[i];
                auto &ci = cellInfo_[ty * w + tx];
                if (ci.blocked) {
                    continue;
                }
                auto currRange = mc->ranges + 1;
                auto ite = selCells.find(std::make_pair(tx, ty));
                if (ite == selCells.end()) {
                    auto &mcell = selCells[std::make_pair(tx, ty)];
                    mcell.x = tx;
                    mcell.y = ty;
                    mcell.moves = -1;
                    mcell.ranges = currRange;
                    mcell.rangeParent = mc;
                    if (currRange < ranges) {
                        sortedAttackable.push_back(&mcell);
                        std::push_heap(sortedAttackable.begin(), sortedAttackable.end(), CompareRangeCells());
                    }
                }
            }
        }
    }
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
    if (index < 0) {
        actIndex_ = -1;
        actId_ = index;
        actLevel_ = 0;
        attackTimesLeft_ = 1;
        /*
         * Z.DAT:0x397A7, 0x39B50 and 0x39EB9 all derive the range as
         * `proficiency / 15 + 1`.
         */
        int steps;
        switch (index) {
        case -3:
            steps = ch->info.poison / 15 + 1;
            break;
        case -2:
            steps = ch->info.depoison / 15 + 1;
            break;
        case -1:
            steps = ch->info.medic / 15 + 1;
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
    attackTimesLeft_ = ch->info.doubleAttack ? 2 : 1;
    actLevel_ = skillLevel;
    switch (skill->attackAreaType) {
    case 1: {
        auto msgBox = new DirectionSelMessageBox(this, 0, 0, gWindow->width(), gWindow->height());
        msgBox->popup({GETTEXT(92)});
        msgBox->setCloseHandler([this]() {
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
        auto *target = cellInfo_[cursorY_ * mapWidth_ + cursorX_].charInfo;
        if (!target) {
            playerMenu();
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
            /* actMedic charges 2 itself (Z.DAT:0x3A28E), the shared tail 2 more. */
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
                mem::gBag.remove(actIndex_, 1);
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
        /*
         * Z.DAT:0x399FE-0x39A33 is the shared tail of poison, depoison and
         * medic: exactly one experience point and two stamina, whatever the
         * result was. Throwing awards nothing (Z.DAT:0x3A83B).
         */
        if (actId_ >= -3 && actId_ <= -1) {
            ++ch->exp;
            ch->info.stamina = std::clamp<std::int16_t>(ch->info.stamina - 2, 0, data::StaminaMax);
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

        /*
         * The original walks a line or cross outwards (Z.DAT:0x389D1 and
         * Z.DAT:0x37E32 both count up from 1), so the random draws happen in
         * near-to-far order.
         */
        switch (skillInfo->attackAreaType) {
        case 1: {
            auto sx = cameraX_, sy = cameraY_;
            int r = skillInfo->selRange[actLevel_];
            for (int i = 1; i <= r; ++i) {
                switch (ch->direction) {
                case Map::DirUp:
                    if (sy >= i) { makeDamage(ch, sx, sy - i, i); }
                    break;
                case Map::DirRight:
                    if (sx + i < mapWidth_) { makeDamage(ch, sx + i, sy, i); }
                    break;
                case Map::DirLeft:
                    if (sx >= i) { makeDamage(ch, sx - i, sy, i); }
                    break;
                case Map::DirDown:
                    if (sy + i < mapHeight_) { makeDamage(ch, sx, sy + i, i); }
                    break;
                default:
                    break;
                }
            }
            break;
        }
        case 2: {
            auto sx = cameraX_, sy = cameraY_;
            int r = skillInfo->selRange[actLevel_];
            /* Z.DAT:0x37E43, 0x37FC5, 0x380C8, 0x381CD: up, down, left, right. */
            for (int i = 1; i <= r; ++i) {
                if (sy >= i) { makeDamage(ch, sx, sy - i, i); }
                if (sy + i < mapHeight_) { makeDamage(ch, sx, sy + i, i); }
                if (sx >= i) { makeDamage(ch, sx - i, sy, i); }
                if (sx + i < mapWidth_) { makeDamage(ch, sx + i, sy, i); }
            }
            break;
        }
        case 3: {
            auto sx = cursorX_, sy = cursorY_;
            int r = skillInfo->area[actLevel_];
            /*
             * Z.DAT:0x37AAD measures the distance from the actor to each hit
             * cell, not from the actor to the area centre plus the offset.
             * The original also scans columns first.
             */
            for (int i = -r; i <= r; ++i) {
                auto rx = sx + i;
                if (rx < 0 || rx >= mapWidth_) { continue; }
                for (int j = -r; j <= r; ++j) {
                    auto ry = sy + j;
                    if (ry < 0 || ry >= mapHeight_) { continue; }
                    makeDamage(ch, rx, ry, std::abs(rx - cameraX_) + std::abs(ry - cameraY_));
                }
            }
            break;
        }
        default: {
            int x = cursorX_, y = cursorY_;
            makeDamage(ch, x, y, std::abs(x - cameraX_) + std::abs(y - cameraY_));
            break;
        }
        }
        mem::postDamage(&ch->info, actIndex_, actLevel_, attackTimesLeft_ == 1 ? 3 : 0, skillLevelup_);
        if (skillLevelup_) {
            actLevel_ = std::clamp<std::int16_t>(ch->info.skillLevel[actIndex_] / 100, 0, 9);
        }
    } else {
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
    /*
     * The original recomputes both knowledge sums inside the damage routine
     * relative to the attacker (Z.DAT:0x3919E), so the attacker's own side
     * always feeds the attack term.
     */
    if (mem::actDamage(&ch->info, &enemyInfo, knowledge_[ch->side], knowledge_[ch->side ^ 1],
                       distance, actIndex_, actLevel_, dmg, ps, exp, dead)) {
        ch->exp += exp;
        if (!wasDead && dead) {
            recalcKnowledge();
        }
        const auto *skillInfo = mem::gSaveData.skillInfo[actId_];
        if (skillInfo && skillInfo->damageType > 0) {
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
    /*
     * Z.DAT:0x3A8CF compares the remaining steps with `speed / 10` instead of
     * the value the round handed out (`Z.DAT:0x328A7`), which makes the
     * stationary bonus unreachable for almost every speed. The comparison
     * itself is a defect, so this build tests the actual movement.
     */
    mem::actRest(&ch->info, ch->steps != ch->initialSteps);
    endTurn(ch);
}

void Warfield::endTurn(CharInfo *expectedActor) {
    auto *ch = currentActor_;
    if (!ch || (expectedActor && expectedActor != ch)) { return; }
    auto ite = std::find(charQueue_.begin(), charQueue_.end(), ch);
    if (ite != charQueue_.end()) {
        charQueue_.erase(ite);
    }
    currentActor_ = nullptr;
    pendingAutoAction_ = nullptr;
    for (auto &ci: chars_) {
        if (ci.info.hp <= 0 && ci.x >= 0 && ci.y >= 0) {
            if (ci.x < mapWidth_ && ci.y < mapHeight_) {
                auto &cell = cellInfo_[ci.x + ci.y * mapWidth_];
                if (cell.charInfo == &ci) { cell.charInfo = nullptr; }
            }
            ci.x = ci.y = -1;
            drawDirty_ = true;
        }
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
    removeAllChildren();
    fadeNode_ = nullptr;
    fadePostAction_ = nullptr;
    runFadePostAction_ = false;
    std::vector<CharInfo*> alives;
    for (auto &ci: chars_) {
        if (ci.side != 0) { continue; }
        auto *charInfo = mem::gSaveData.charInfo[ci.id];
        if (!charInfo) { continue; }
        /*
         * Z.DAT:0x3B46A: survivors never leave the field below a fifth of their
         * maximum, and the fallen get back up at that value with at least ten
         * stamina.
         */
        charInfo->mp = ci.info.mp;
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
    auto wexp = info != nullptr ? info->exp : 0;
    std::vector<std::pair<int, std::wstring>> messages = { {0, GETTEXT(won_ ? 93 : 94) } };
    /*
     * Z.DAT:0x3B405 only shares the battlefield bonus among the survivors of a
     * won fight; the experience earned per hit is credited either way.
     */
    if (won_ || getExpOnLose_) {
        for (auto *ch: alives) {
            ch->exp += wexp / int(alives.size());
        }
    }
    {
        for (auto &ci: chars_) {
            if (ci.side != 0) { continue; }
            auto *ch = &ci;
            auto *charInfo = mem::gSaveData.charInfo[ch->id];
            if (!charInfo) { continue; }
            /*
             * Z.DAT:0x3B509 credits the earned experience for everyone before it
             * checks the outcome: the whole amount feeds the character level and
             * four fifths feeds both the skill book and the crafting progress,
             * instead of being split between them.
             */
            const int exp = ch->exp;
            const int exp2 = ch->exp * 8 / 10;
            charInfo->exp = std::clamp<int>(int(charInfo->exp) + exp, 0, data::ExpMax);
            charInfo->expForItem = std::uint16_t(
                std::clamp<int>(int(charInfo->expForItem) + exp2, 0, data::ExpMax));
            charInfo->expForMakeItem = std::uint16_t(
                std::clamp<int>(int(charInfo->expForMakeItem) + exp2, 0, data::ExpMax));
            /* Z.DAT:0x3B5A1 gates the level up, training and crafting steps. */
            if (!won_ && !getExpOnLose_) { continue; }
            auto name = GETCHARNAME(ch->id);
            messages.emplace_back(std::make_pair(0, fmt::format(GETTEXT(95), name, ch->exp)));
            bool canLearn = false, makingItem = false;
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
                            skillLevel = std::clamp<std::int16_t>(charInfo->skillLevel[i] / 100,
                                                                  0, data::SkillLevelMaxDiv);
                            /* Z.DAT:0x3BB41: a maxed skill blocks the whole step. */
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
            /*
             * `WAR-TRAIN` Z.DAT:0x3BA85 advances the book once per battle, keeps
             * no leftover progress and grants no random maximum-mp bonus. The
             * skill level rises by exactly 100 while the stored value stays below
             * 899, and an unknown skill simply lands in the first free slot.
             */
            if (canLearn && itemInfo) {
                auto expReq = mem::getExpForSkillLearn(charInfo->learningItem, skillLevel,
                                                       charInfo->potential);
                if (expReq > 0 && charInfo->expForItem >= expReq) {
                    mem::applyBookChanges(charInfo, itemInfo);
                    charInfo->expForItem = 0;
                    messages.emplace_back(std::make_pair(0, fmt::format(GETTEXT(97), name,
                                                                        GETITEMNAME(charInfo->learningItem))));
                    if (skillId > 0) {
                        bool known = false;
                        for (int i = 0; i < data::LearnSkillCount; ++i) {
                            if (charInfo->skillId[i] != skillId) { continue; }
                            known = true;
                            if (charInfo->skillLevel[i] >= data::SkillLevelMaxDiv * 100 - 1) { continue; }
                            charInfo->skillLevel[i] = std::int16_t(charInfo->skillLevel[i] + 100);
                            messages.emplace_back(std::make_pair(1, fmt::format(GETTEXT(98),
                                GETSKILLNAME(skillId), charInfo->skillLevel[i] / 100 + 1)));
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
            /*
             * `WAR-CRAFT` Z.DAT:0x3C2AC. `makeItemCount` is the amount of
             * material a recipe consumes, not the amount produced: the output is
             * one unit for a new bag entry and `rnd(3) + 1` when the bag already
             * holds that item. Any of the five recipe slots may be drawn, not
             * only the leading ones.
             */
            if (makingItem) {
                const auto craftReq = mem::getExpForMakeItem(charInfo->learningItem, charInfo->potential);
                const auto material = mem::gBag[itemInfo->reqMaterial];
                bool affordable[data::MakeItemCount] = {false};
                bool anyAffordable = false;
                for (int i = 0; material > 0 && i < data::MakeItemCount; ++i) {
                    if (itemInfo->makeItem[i] < 0 || material < itemInfo->makeItemCount[i]) { continue; }
                    affordable[i] = true;
                    anyAffordable = true;
                }
                if (craftReq > 0 && charInfo->expForMakeItem >= craftReq && anyAffordable) {
                    int index;
                    do {
                        index = int(util::gRandom(data::MakeItemCount));
                    } while (!affordable[index]);
                    const auto produced = mem::gBag[itemInfo->makeItem[index]] > 0
                        ? std::int16_t(util::gRandom(3) + 1) : std::int16_t(1);
                    charInfo->expForMakeItem = 0;
                    mem::gBag.add(itemInfo->makeItem[index], produced);
                    mem::gBag.remove(itemInfo->reqMaterial, itemInfo->makeItemCount[index]);
                    messages.emplace_back(std::make_pair(0, fmt::format(GETTEXT(99),
                                                                        name, GETITEMNAME(itemInfo->makeItem[index]))));
                }
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
