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
#include "mem/savedata.hh"
#include "mem/action.hh"
#include "util/conv.hh"
#include <fmt/format.h>
#include <map>

namespace hojy::scene {

Map::Direction calcDirection(int fx, int fy, int tx, int ty) {
    int dx = tx - fx, dy = ty - fy;
    if (std::abs(dx) > std::abs(dy)) {
        if (dx < 0) { return Map::DirLeft; }
        return Map::DirRight;
    }
    if (dy < 0) { return Map::DirUp; }
    return Map::DirDown;
}

WarField::WarField(Renderer *renderer, int x, int y, int width, int height, float scale):
    Map(renderer, x, y, width, height, scale) {
    fightTextures_.resize(FightTextureListCount);
    for (size_t i = 0; i < FightTextureListCount; ++i) {
        auto &f = fightTextures_[i];
        f.setRenderer(renderer_);
        f.setPalette(gNormalPalette);
        data::GrpData::DataSet dset;
        if (data::GrpData::loadData(fmt::format("FIGHT{:03}.IDX", i), fmt::format("FIGHT{:03}.GRP", i), dset)) {
            f.loadFromRLE(dset);
        }
    }
}

WarField::~WarField() {
    delete maskTex_;
}

void WarField::cleanup() {
    chars_.clear();
    charQueue_.clear();
    stage_ = Idle;
    knowledge_[0] = knowledge_[1] = 0;
    cursorX_ = 0;
    cursorY_ = 0;
    autoControl_ = false;
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
    fightTexMgr_ = nullptr;
    popupNumbers_.clear();
}

bool WarField::load(std::int16_t warId) {
    cleanup();

    warId_ = warId;
    const auto *info = data::gWarFieldData.info(warId);
    auto warMapId = info->warFieldId;
    const auto &layers = data::gWarFieldData.layers(warMapId)->layers;
    if (warMapLoaded_.find(warMapId) == warMapLoaded_.end()) {
        mapWidth_ = data::WarFieldWidth;
        mapHeight_ = data::WarFieldHeight;
        data::GrpData::DataSet dset;
        if (!data::GrpData::loadData(fmt::format("WDX{:03}", warMapId), fmt::format("WMP{:03}", warMapId), dset)) {
            return false;
        }
        if (!textureMgr_.mergeFromRLE(dset)) {
            return false;
        }
        warMapLoaded_.insert(warMapId);
        if (!maskTex_) {
            maskTex_ = new Texture;
            maskTex_->loadFromRLE(renderer_, dset[0], gMaskPalette);
            maskTex_->enableBlendMode(true);
        }
    }
    {
        auto *tex = textureMgr_[0];
        cellWidth_ = tex->width();
        cellHeight_ = tex->height();
        offsetX_ = tex->originX();
        offsetY_ = tex->originY();
    }
    int cellDiffX = cellWidth_ / 2;
    int cellDiffY = cellHeight_ / 2;
    texWidth_ = (mapWidth_ + mapHeight_) * cellDiffX;
    texHeight_ = (mapWidth_ + mapHeight_) * cellDiffY;

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
            ci.isWater = texId >= 179 && texId <= 181 || texId == 261 || texId == 511 || texId >= 662 && texId <= 665 || texId == 674;
            ci.earth = textureMgr_[texId];
            texId = layers[1][pos] >> 1;
            if (texId) {
                ci.building = textureMgr_[texId];
            }
        }
        x -= cellDiffX; y += cellDiffY;
    }

    subMapId_ = warMapId;
    resetFrame();

    return true;
}

bool WarField::getDefaultChars(std::set<std::int16_t> &chars) const {
    const auto *info = data::gWarFieldData.info(warId_);
    if (info->forceMembers[0] >= 0) { return false; }
    for (auto &id: info->defaultMembers) {
        if (id >= 0) { chars.insert(id); }
    }
    return true;
}

void WarField::putChars(const std::vector<std::int16_t> &chars) {
    const auto *info = data::gWarFieldData.info(warId_);
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
    for (auto &ci: chars_) {
        auto &cell = cellInfo_[ci.y * mapWidth_ + ci.x];
        mem::addUpPropFromEquipToChar(&ci.info);
        if (ci.side == 1) {
            ci.info.hp = ci.info.maxHp;
            ci.info.mp = ci.info.maxMp;
            ci.info.stamina = data::StaminaMax;
        }
        if (ci.info.knowledge >= data::KnowledgeBarrier) {
            knowledge_[ci.side] += ci.info.knowledge;
        }
        cell.charInfo = &ci;
    }
    frameUpdate();
    if (info->music >= 0) {
        gWindow->playMusic(info->music);
    }
}

void WarField::render() {
    Map::render();

    if (drawDirty_) {
        drawDirty_ = false;
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int curX = cameraX_, curY = cameraY_;
        int nx = int(auxWidth_) / 2 + cellWidth_ * 2;
        int ny = int(auxHeight_) / 2 + cellHeight_ * 2;
        int wcount = nx * 2 / cellWidth_;
        int hcount = (ny * 2 + 4 * cellHeight_) / cellDiffY;
        int cx, cy, tx, ty;
        int delta = -mapWidth_ + 1;

        renderer_->setTargetTexture(drawingTerrainTex_);
        renderer_->setClipRect(0, 0, 2048, 2048);
        renderer_->clear(0, 0, 0, 0);

        cx = (nx / cellDiffX + ny / cellDiffY) / 2;
        cy = (ny / cellDiffY - nx / cellDiffX) / 2;
        tx = int(auxWidth_) / 2 - (cx - cy) * cellDiffX;
        ty = int(auxHeight_) / 2 + cellDiffY - (cx + cy) * cellDiffY;
        cx = curX - cx; cy = curY - cy;
        bool selecting = stage_ == MoveSelecting || stage_ == AttackSelecting || stage_ == UseItemSelecting;
        bool acting = stage_ == Acting;
        auto *ch = charQueue_.back();
        if (acting && effectTexIdx_ >= 0) {
            const auto *skillInfo = actId_ >= 0 ? mem::gSaveData.skillInfo[actId_] : nullptr;
            const auto &effTexMgr = (*gEffect[effectId_]);
            const auto *tex = effTexMgr[effectTexIdx_ < effTexMgr.size() ? effectTexIdx_ : effTexMgr.size() - 1];
            auto mw = mapWidth_;
            if (skillInfo == nullptr || skillInfo->attackAreaType == 0) {
                auto sx = cursorX_, sy = cursorY_;
                cellInfo_[sy * mw + sx].effect = tex;
            } else {
                switch (skillInfo->attackAreaType) {
                case 1: {
                    auto sx = cameraX_, sy = cameraY_, st = sy * mw;
                    int r = skillInfo->selRange[actLevel_];
                    for (int i = r; i; --i) {
                        switch (charQueue_.back()->direction) {
                        case Map::DirUp:
                            if (sy >= i) cellInfo_[st - i * mw + sx].effect = tex;
                            break;
                        case Map::DirRight:
                            if (sx + i < mapWidth_) cellInfo_[st + sx + i].effect = tex;
                            break;
                        case Map::DirLeft:
                            if (sx >= i) cellInfo_[st + sx - i].effect = tex;
                            break;
                        case Map::DirDown:
                            if (sy + i < mapHeight_) cellInfo_[st + i * mw + sx].effect = tex;
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
                        if (sy >= i) cellInfo_[st - i * mw + sx].effect = tex;
                        if (sx + i < mapWidth_) cellInfo_[st + sx + i].effect = tex;
                        if (sx >= i) cellInfo_[st + sx - i].effect = tex;
                        if (sy + i < mapHeight_) cellInfo_[st + i * mw + sx].effect = tex;
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
                            cellInfo_[ry * mw + rx].effect = tex;
                        }
                    }
                    break;
                }
                default:
                    break;
                }
            }
        }
        for (int j = hcount; j; --j) {
            int x = cx, y = cy;
            int dx = tx;
            int offset = y * mapWidth_ + x;
            for (int i = wcount; i; --i, dx += cellWidth_, offset += delta, ++x, --y) {
                if (x < 0 || x >= data::WarFieldWidth || y < 0 || y >= data::WarFieldHeight) {
                    continue;
                }
                auto &ci = cellInfo_[offset];
                renderer_->renderTexture(ci.earth, dx, ty);
                if (ci.insideMovingArea == 2) {
                    maskTex_->setBlendColor(236, 236, 236, 160);
                    renderer_->renderTexture(maskTex_, dx, ty);
                } else if (ci.charInfo) {
                    maskTex_->setBlendColor(192, 192, 192, 160);
                    renderer_->renderTexture(maskTex_, dx, ty);
                } else if (selecting && !ci.insideMovingArea) {
                    maskTex_->setBlendColor(32, 32, 32, 204);
                    renderer_->renderTexture(maskTex_, dx, ty);
                }
                if (ci.building) {
                    renderer_->renderTexture(ci.building, dx, ty);
                }
                if (ci.charInfo) {
                    if (acting && ci.charInfo == ch) {
                        renderer_->renderTexture((*fightTexMgr_)[fightTexIdx_], dx, ty);
                    } else {
                        renderer_->renderTexture(textureMgr_[2553 + 4 * ci.charInfo->texId + int(ci.charInfo->direction)], dx, ty);
                    }
                }
                if (ci.effect) {
                    renderer_->renderTexture(ci.effect, dx, ty);
                    ci.effect = nullptr;
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
        if (acting && effectTexIdx_ >= 3) {
            int ax = int(auxWidth_) / 2, ay = int(auxHeight_) / 2 + cellDiffY;
            auto *ttf = renderer_->ttf();
            auto fsize = int(8.f * scale_);
            for (auto &n: popupNumbers_) {
                int deltax = n.x - cameraX_, deltay = n.y - cameraY_;
                ttf->setColor(n.r, n.g, n.b);
                ttf->render(n.str,
                            ax + (deltax - deltay) * cellDiffX + n.offsetX,
                            ay + (deltax + deltay) * cellDiffY - cellDiffY * 2 - fsize - fightFrame_ * 2,
                            true, fsize);
            }
        }
        renderer_->setTargetTexture(nullptr);
        renderer_->unsetClipRect();
    }

    renderer_->clear(0, 0, 0, 0);
    renderer_->renderTexture(drawingTerrainTex_, x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
}

void WarField::handleKeyInput(Node::Key key) {
    if (stage_ != MoveSelecting && stage_ != AttackSelecting && stage_ != UseItemSelecting) {
        if (key == KeyCancel) {
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
            if (x == cameraX_ && y == cameraY_) { break; }
            if (cellInfo_[y * mapWidth_ + x].charInfo) {
                break;
            }
            auto ite = selCells_.find(std::make_pair(x, y));
            if (ite != selCells_.end()) {
                stage_ = Moving;
                auto *sc = &ite->second;
                while (sc) {
                    movingPath_.emplace_back(std::make_pair(sc->x, sc->y));
                    sc = sc->parent;
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
        if (stage_ != AttackSelecting) {
            unmaskArea();
            drawDirty_ = true;
            playerMenu();
        }
        return;
    default:
        return;
    }
    if (drawDirty_) {
        cellInfo_[cursorY_ * mapWidth_ + cursorX_].insideMovingArea = 2;
    }
}

void WarField::frameUpdate() {
    switch (stage_) {
    case Idle:
        nextAction();
        break;
    case Moving: {
        if (movingPath_.empty()) { stage_ = Idle; break; }
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
        fightTexIdx_ = std::min(fightTexIdx_ + 1, fightTexCount_ - 1);
        if (fightFrame_ == 0) {
            const mem::SkillData *skillInfo;
            if (actId_ >= 0 && (skillInfo = mem::gSaveData.skillInfo[actId_]) != nullptr) {
                gWindow->playAtkSound(skillInfo->soundId);
            } else {
                gWindow->playAtkSound(0);
            }
        } else if (fightFrame_ == 3) {
            gWindow->playEffectSound(effectId_);
        }
        ++fightFrame_;
        if (++effectTexIdx_ >= int(gEffect[effectId_]->size()) + 3) {
            if (--attackTimesLeft_ <= 0) {
                actIndex_ = -1;
                actId_ = -1;
                actLevel_ = 0;
                effectId_ = -1;
                effectTexIdx_ = -1;
                fightTexIdx_ = -1;
                fightTexCount_ = 0;
                fightFrame_ = 0;
                attackTimesLeft_ = 0;
                fightTexMgr_ = nullptr;
                endTurn();
            } else {
                startActAction();
            }
        }
        drawDirty_ = true;
        break;
    }
    default:
        break;
    }
}

void WarField::nextAction() {
    if (charQueue_.empty()) {
        charQueue_.reserve(chars_.size());
        for (auto &c: chars_) {
            if (c.info.hp > 0) {
                charQueue_.emplace_back(&c);
                c.steps = c.info.speed / 15;
            }
        }
        std::stable_sort(charQueue_.begin(), charQueue_.end(), [](const CharInfo *c0, const CharInfo *c1) {
            return c0->info.speed < c1->info.speed;
        });
    }
    CharInfo *ch;
    while ((ch = charQueue_.back())->info.hp <= 0) {
        charQueue_.pop_back();
    }
    cameraX_ = ch->x;
    cameraY_ = ch->y;
    drawDirty_ = true;
    if (ch->side == 1 || autoControl_) {
        autoAction();
    } else {
        playerMenu();
    }
}

void WarField::autoAction() {
    charQueue_.pop_back();
    /* TODO: implement this */
}

void WarField::playerMenu() {
    stage_ = PlayerMenu;
    auto *ch = charQueue_.back();
    auto *menu = new MenuTextList(this, 40, 40, width_ - 80, height_ - 80);
    std::vector<std::wstring> n;
    std::vector<int> menuIndices;
    n.reserve(10);
    menuIndices.reserve(10);
    auto &info = ch->info;
    if (ch->steps && info.stamina >= 5) {
        n.emplace_back(L"移動"); menuIndices.emplace_back(0);
    }
    if (info.stamina >= 10) {
        n.emplace_back(L"攻擊"); menuIndices.emplace_back(1);
    }
    if (info.poison && info.stamina >= 20) {
        n.emplace_back(L"用毒"); menuIndices.emplace_back(2);
    }
    if (info.stamina >= 50) {
        if (info.depoison) {
            n.emplace_back(L"解毒");
            menuIndices.emplace_back(3);
        }
        if (info.medic) {
            n.emplace_back(L"醫療");
            menuIndices.emplace_back(4);
        }
    }
    n.emplace_back(L"物品"); menuIndices.emplace_back(5);
    if (charQueue_.size() > 1) {
        n.emplace_back(L"等待"); menuIndices.emplace_back(6);
    }
    n.emplace_back(L"狀態"); menuIndices.emplace_back(7);
    n.emplace_back(L"休息"); menuIndices.emplace_back(8);
    n.emplace_back(L"自動"); menuIndices.emplace_back(9);
    menu->popup(n);
    menu->setHandler([this, menu, menuIndices, ch]() {
        auto index = menu->currIndex();
        if (index < 0 || index >= menuIndices.size()) { return; }
        switch (menuIndices[index]) {
        case 0:
            maskSelectableArea(ch->steps);
            stage_ = MoveSelecting;
            drawDirty_ = true;
            break;
        case 1:
            if (ch->info.skillId[1] > 0) {
                auto *submenu = new MenuTextList(menu, menu->x() + menu->width() + 10, 40, width_ - menu->x() + menu->width() - 10, height_ - 80);
                std::vector<std::wstring> items;
                for (auto skillId: ch->info.skillId) {
                    if (skillId <= 0) {
                        break;
                    }
                    const auto *skillInfo = mem::gSaveData.skillInfo[skillId];
                    if (!skillInfo) {
                        break;
                    }
                    items.emplace_back(util::big5Conv.toUnicode(skillInfo->name));
                }
                submenu->popup(items);
                submenu->setHandler([this, menu, submenu]() {
                    if (tryUseSkill(submenu->currIndex())) {
                        delete menu;
                    } else {
                        delete submenu;
                    }
                });
            } else {
                if (tryUseSkill(0)) {
                    delete menu;
                }
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
            auto *iv = new ItemView(this, 40, 40, gWindow->width() - 40, gWindow->height() - 40);
            iv->setUser(ch->id);
            iv->show(true, [this](std::int16_t itemId) {
                if (itemId < 0) {
                    endTurn();
                }
            });
            return;
        }
        case 6:
            charQueue_.pop_back();
            charQueue_.insert(charQueue_.begin(), ch);
            stage_ = Idle;
            break;
        case 7: {
            auto *svmenu = new CharListMenu(this, 0, 0, gWindow->width(), gWindow->height());
            svmenu->initWithTeamMembers({L"要查閱誰的狀態"}, {CharListMenu::LEVEL},
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
                                            sv->makeCenter(gWindow->width(), gWindow->height());
                                        }, nullptr);
            svmenu->makeCenter(gWindow->width(), gWindow->height() * 4 / 5);
            return;
        }
        case 8:
            endTurn();
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

void WarField::maskSelectableArea(int steps, bool zoecheck) {
    auto *ch = charQueue_.back();
    auto myside = ch->side;
    int w = mapWidth_, h = mapHeight_;
    std::vector<SelectableCell*> sortedSelectable;

    selCells_.clear();
    auto &start = selCells_[std::make_pair(ch->x, ch->y)];
    start.x = ch->x;
    start.y = ch->y;
    start.moves = 0;
    start.parent = nullptr;
    sortedSelectable.push_back(&start);

    while (!sortedSelectable.empty()) {
        std::pop_heap(sortedSelectable.begin(), sortedSelectable.end(), CompareSelCells());
        auto *mc = sortedSelectable.back();
        sortedSelectable.erase(sortedSelectable.end() - 1);
        bool blocked = false;
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
            if (zoecheck) {
                auto &ci = cellInfo_[ty * w + tx];
                if (ci.charInfo && ci.charInfo->side == myside) {
                    blocked = true;
                    break;
                }
            }
            nx[ncnt] = tx;
            ny[ncnt] = ty;
            ++ncnt;
        }
        if (blocked) { continue; }
        for (int i = 0; i < ncnt; ++i) {
            int tx = nx[i], ty = ny[i];
            auto &ci = cellInfo_[ty * w + tx];
            if (ci.isWater || ci.building || (ci.charInfo && ci.charInfo->side == myside)) { continue; }
            auto currMove = mc->moves + 1;
            auto ite = selCells_.find(std::make_pair(tx, ty));
            if (ite == selCells_.end()) {
                if (currMove > steps) {
                    continue;
                }
                auto &mcell = selCells_[std::make_pair(tx, ty)];
                mcell.x = tx;
                mcell.y = ty;
                mcell.moves = currMove;
                mcell.parent = mc;
                sortedSelectable.push_back(&mcell);
                std::push_heap(sortedSelectable.begin(), sortedSelectable.end(), CompareSelCells());
            }
        }
    }
    for (auto c: selCells_) {
        cellInfo_[c.first.first + c.first.second * w].insideMovingArea = true;
    }
    cursorX_ = ch->x;
    cursorY_ = ch->y;
}

void WarField::unmaskArea() {
    int w = mapWidth_;
    for (auto c: selCells_) {
        cellInfo_[c.first.first + c.first.second * w].insideMovingArea = false;
    }
    selCells_.clear();
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
        case KeyCancel:
            delete this;
            break;
        default:
            break;
        }
    }

private:
    std::function<void(Map::Direction)> directionHandler_;
};

bool WarField::tryUseSkill(int index) {
    auto *ch = charQueue_.back();
    if (index < 0) {
        actIndex_ = -1;
        actId_ = index;
        actLevel_ = 0;
        attackTimesLeft_ = 1;
        int steps;
        switch (index) {
        case -3:
            steps = ch->info.poison / 15;
            break;
        case -2:
            steps = ch->info.depoison / 15;
            break;
        case -1:
            steps = ch->info.medic / 15;
            break;
        default:
            steps = 1;
            break;
        }
        maskSelectableArea(steps);
        stage_ = AttackSelecting;
        drawDirty_ = true;
        return true;
    }
    const auto *skill = mem::gSaveData.skillInfo[std::max<std::int16_t>(ch->info.skillId[index], 0)];
    if (!skill) {
        return false;
    }
    actIndex_ = index;
    actId_ = ch->info.skillId[index];
    attackTimesLeft_ = ch->info.doubleAttack ? 2 : 1;
    actLevel_ = std::clamp<std::int16_t>(ch->info.skillLevel[index] / 100, 0, 9);
    auto mpUse = skill->reqMp * (actLevel_ / 2 + 1);
    if (mpUse > ch->info.mp) {
        actLevel_ = ch->info.mp / skill->reqMp * 2;
    }
    switch (skill->attackAreaType) {
    case 1: {
        auto msgBox = new DirectionSelMessageBox(this, 0, 0, gWindow->width(), gWindow->height());
        msgBox->popup({L"選擇攻擊方向"});
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
        maskSelectableArea(skill->selRange[actLevel_]);
        stage_ = AttackSelecting;
        drawDirty_ = true;
        return true;
    }
}

void WarField::startActAction() {
    popupNumbers_.clear();
    if (actId_ < 0) {
        auto *ch = charQueue_.back();
        auto *target = cellInfo_[cursorY_ * mapWidth_ + cursorX_].charInfo;
        std::int16_t result;
        std::uint8_t r, g, b;
        auto *ttf = renderer_->ttf();
        bool popup;
        switch (actId_) {
        case -2:
            effectId_ = data::DepoisonEffectID;
            popup = target && ch->side == target->side;
            result = popup ? mem::actDepoison(&ch->info, &target->info, 2) : 0;
            r = 104; g = 192; b = 232;
            break;
        case -1:
            effectId_ = data::MedicEffectID;
            popup = target && ch->side == target->side;
            result = popup ? mem::actMedic(&ch->info, &target->info, 4) : 0;
            r = 236; g = 200; b = 40;
            break;
        default:
            effectId_ = data::PoisonEffectID;
            popup = target && ch->side != target->side;
            result = popup ? mem::actPoison(&ch->info, &target->info, 3) : 0;
            popup = popup && result != 0;
            r = 96; g = 176; b = 64;
            break;
        }
        if (popup) {
            auto txt = fmt::format(L"{:+}", result);
            popupNumbers_.emplace_back(PopupNumber{txt, cursorX_, cursorY_,
                                                   -ttf->stringWidth(txt, int(8.f * scale_)) / 2,
                                                   r, g, b});
        }
        stage_ = Acting;
        if (cameraX_ != cursorX_ || cameraY_ != cursorY_) {
            ch->direction = calcDirection(cameraX_, cameraY_, cursorX_, cursorY_);
        }
        fightTexMgr_ = &fightTextures_[ch->info.headId];
        fightTexCount_ = ch->info.frame[0];
        fightTexIdx_ = fightTexCount_ * int(ch->direction);
        fightTexCount_ += fightTexIdx_;
        effectTexIdx_ = -ch->info.frameDelay[0];
        fightFrame_ = -ch->info.frameSoundDelay[0];
        return;
    }
    const auto *skillInfo = mem::gSaveData.skillInfo[actId_];
    if (skillInfo) {
        effectId_ = skillInfo->effectId;
        auto skillType = skillInfo->skillType;
        stage_ = Acting;
        auto *ch = charQueue_.back();
        if ((skillInfo->attackAreaType == 0 || skillInfo->attackAreaType == 3)
            && (cameraX_ != cursorX_ || cameraY_ != cursorY_)) {
            ch->direction = calcDirection(cameraX_, cameraY_, cursorX_, cursorY_);
        }
        fightTexMgr_ = &fightTextures_[ch->info.headId];
        fightTexIdx_ = 0;
        for (std::int16_t i = 0; i < skillType; ++i) {
            fightTexIdx_ += 4 * ch->info.frame[i];
        }
        fightTexCount_ = ch->info.frame[skillType];
        fightTexIdx_ += fightTexCount_ * int(ch->direction);
        fightTexCount_ += fightTexIdx_;
        effectTexIdx_ = -ch->info.frameDelay[skillType];
        fightFrame_ = -ch->info.frameSoundDelay[skillType];

        switch (skillInfo->attackAreaType) {
        case 1: {
            auto sx = cameraX_, sy = cameraY_;
            int r = skillInfo->selRange[actLevel_];
            for (int i = r; i; --i) {
                switch (ch->direction) {
                case Map::DirUp:
                    if (sy >= i) { makeDamage(ch, sx, sy - i); }
                    break;
                case Map::DirRight:
                    if (sx + i < mapWidth_) { makeDamage(ch, sx + i, sy); }
                    break;
                case Map::DirLeft:
                    if (sx >= i) { makeDamage(ch, sx - i, sy); }
                    break;
                case Map::DirDown:
                    if (sy + i < mapHeight_) { makeDamage(ch, sx, sy + i); }
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
            for (int i = r; i; --i) {
                if (sy >= i) { makeDamage(ch, sx, sy - i); }
                if (sx + i < mapWidth_) { makeDamage(ch, sx + i, sy); }
                if (sx >= i) { makeDamage(ch, sx - i, sy); }
                if (sy + i < mapHeight_) { makeDamage(ch, sx, sy + i); }
            }
            break;
        }
        case 3: {
            auto sx = cursorX_, sy = cursorY_;
            int r = skillInfo->area[actLevel_];
            for (int j = -r; j <= r; ++j) {
                auto ry = sy + j;
                if (ry < 0 || ry >= mapHeight_) { continue; }
                for (int i = -r; i <= r; ++i) {
                    auto rx = sx + i;
                    if (rx < 0 || rx >= mapWidth_) { continue; }
                    makeDamage(ch, rx, ry);
                }
            }
            break;
        }
        default:
            makeDamage(ch, cursorX_, cursorY_);
            break;
        }
    } else {
        endTurn();
    }
}

void WarField::makeDamage(WarField::CharInfo *ch, int x, int y) {
    auto *info = cellInfo_[y * mapWidth_ + x].charInfo;
    if (!info || info->side == ch->side) { return; }
    std::int16_t dmg, ps;
    bool dead, levelup;
    if (mem::actDamage(&ch->info, &info->info, knowledge_[0], knowledge_[1],
                   std::abs(x - cameraX_) + std::abs(y - cameraY_), actIndex_, actLevel_,
                   attackTimesLeft_ == 1 ? 3 : 0, dmg, ps, dead, levelup)) {
        auto txt = fmt::format(L"{:+}", -dmg);
        auto *ttf = renderer_->ttf();
        popupNumbers_.emplace_back(PopupNumber {txt, x, y,
                                                -ttf->stringWidth(txt, int(8.f * scale_)) / 2,
                                                232, 32, 44});
    }
}

void WarField::endTurn() {
    auto *ch = charQueue_.back();
    mem::actPoisonDamage(&ch->info);
    charQueue_.pop_back();
    int aliveCount[2] = {0, 0};
    for (auto &ci: chars_) {
        if (ci.info.hp > 0) {
            ++aliveCount[ci.side];
        } else if (ci.x > 0) {
            cellInfo_[ci.x + ci.y * mapWidth_].charInfo = nullptr;
            ci.x = ci.y = -1;
            drawDirty_ = true;
        }
    }
    if (aliveCount[1] == 0) {
        endWar(true);
        return;
    }
    if (aliveCount[0] == 0) {
        endWar(false);
        return;
    }
    stage_ = Idle;
}

void WarField::endWar(bool won) {
    for (auto &ci: chars_) {
        auto *charInfo = mem::gSaveData.charInfo[ci.id];
        if (!charInfo) { continue; }
        charInfo->hp = std::max<std::int16_t>(1, ci.info.hp);
        charInfo->mp = ci.info.mp;
        charInfo->poisoned = ci.info.poisoned;
        charInfo->hurt = ci.info.hurt;
        charInfo->stamina = ci.info.stamina;
    }
    cleanup();
    gWindow->endWar(won);
}

}
