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
#include "data/grpdata.hh"
#include "data/warfielddata.hh"
#include "mem/savedata.hh"
#include <fmt/format.h>
#include <map>

namespace hojy::scene {

WarField::WarField(Renderer *renderer, int x, int y, int width, int height, float scale):
    Map(renderer, x, y, width, height, scale) {
    fightTextures_.resize(FightTextureListCount);
    for (size_t i = 0; i < FightTextureListCount; ++i) {
        auto &f = fightTextures_[i];
        f.setRenderer(renderer_);
        f.setPalette(gNormalPalette);
        data::GrpData::DataSet dset;
        if (data::GrpData::loadData(fmt::format("FIGHT{:03}IDX", i), fmt::format("FIGHT{:03}.GRP", i), dset)) {
            f.loadFromRLE(dset);
        }
    }
}

WarField::~WarField() {
    delete maskTex_;
}

bool WarField::load(std::int16_t warId) {
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
            const auto *charInfo = mem::gSaveData.charInfo[id];
            if (!charInfo) { continue; }
            chars_.emplace_back(CharInfo {0, false, id, charInfo->headId, info->memberX[i], info->memberY[i], DirLeft,
                                          charInfo->speed, charInfo->hp, charInfo->mp, data::StaminaMax, 0});
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
            const auto *charInfo = mem::gSaveData.charInfo[id];
            if (!charInfo) { continue; }
            auto ite = charMap.find(id);
            size_t index;
            if (ite != charMap.end()) {
                index = ite->second;
            } else {
                index = *indices.begin();
                indices.erase(indices.begin());
            }
            chars_.emplace_back(CharInfo{0, false, id, charInfo->headId, info->memberX[index], info->memberY[index], DirLeft,
                                         charInfo->speed, charInfo->hp, charInfo->mp, charInfo->stamina, 0});
        }
    }
    for (size_t i = 0; i < data::WarFieldEnemyCount; ++i) {
        auto id = info->enemy[i];
        if (id < 0) { continue; }
        const auto *charInfo = mem::gSaveData.charInfo[id];
        if (!charInfo) { continue; }
        chars_.emplace_back(CharInfo {1, true, id, charInfo->headId, info->enemyX[i], info->enemyY[i], DirRight,
                                      charInfo->speed, charInfo->maxHp, charInfo->maxMp, data::StaminaMax, 0});
    }
    for (auto &ci: chars_) {
        auto &cell = cellInfo_[ci.y * mapWidth_ + ci.x];
        cell.charInfo = &ci;
        cell.charTex = textureMgr_[2553 + 4 * ci.texId + int(ci.direction)];
    }
    frameUpdate();
}

void WarField::render() {
    Map::render();

    if (drawDirty_) {
        drawDirty_ = false;
        int cellDiffX = cellWidth_ / 2;
        int cellDiffY = cellHeight_ / 2;
        int curX = cameraX_, curY = cameraY_;
        int nx = int(auxWidth_) / 2 + int(cellWidth_ * scale_);
        int ny = int(auxHeight_) / 2 + int(cellHeight_ * scale_);
        int wcount = nx * 2 / cellWidth_;
        int hcount = (ny * 2 + int(float(2 * cellHeight_) * scale_)) / cellDiffY;
        int cx, cy, tx, ty;
        int delta = -mapWidth_ + 1;

        renderer_->setTargetTexture(drawingTerrainTex_);
        renderer_->setClipRect(0, 0, 2048, 2048);
        renderer_->clear(0, 0, 0, 0);

        cx = (nx / cellDiffX + ny / cellDiffY) / 2;
        cy = (ny / cellDiffY - nx / cellDiffX) / 2;
        tx = int(auxWidth_) / 2 - (cx - cy) * cellDiffX;
        ty = int(auxHeight_) / 2 - (cx + cy) * cellDiffY;
        cx = curX - cx; cy = curY - cy;
        bool selecting = stage_ == Selecting;
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
                } else if (ci.charTex) {
                    maskTex_->setBlendColor(192, 192, 192, 160);
                    renderer_->renderTexture(maskTex_, dx, ty);
                } else if (selecting && !ci.insideMovingArea) {
                    maskTex_->setBlendColor(32, 32, 32, 204);
                    renderer_->renderTexture(maskTex_, dx, ty);
                }
                if (ci.building) {
                    renderer_->renderTexture(ci.building, dx, ty);
                }
                if (ci.charTex) {
                    renderer_->renderTexture(ci.charTex, dx, ty);
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
        renderer_->setTargetTexture(nullptr);
        renderer_->unsetClipRect();
    }

    renderer_->clear(0, 0, 0, 0);
    renderer_->renderTexture(drawingTerrainTex_, x_, y_, width_, height_, 0, 0, auxWidth_, auxHeight_);
}

void WarField::handleKeyInput(Node::Key key) {
    if (stage_ != Selecting) { return; }
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
        }
        unmaskArea();
        drawDirty_ = true;
        break;
    }
    case KeyCancel:
        unmaskArea();
        drawDirty_ = true;
        playerMenu();
        return;
    default:
        break;
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
        newci.charTex = textureMgr_[2553 + 4 * charInfo->texId + int(charInfo->direction)];
        ci.charInfo = nullptr;
        ci.charTex = nullptr;
        charInfo->x = x;
        charInfo->y = y;
        cameraX_ = x;
        cameraY_ = y;
        drawDirty_ = true;
        break;
    }
    case Acting:
    default:
        break;
    }
}

void WarField::nextAction() {
    if (charQueue_.empty()) {
        charQueue_.reserve(chars_.size());
        for (auto &c: chars_) {
            charQueue_.emplace_back(&c);
            c.steps = c.speed / 15;
        }
        std::sort(charQueue_.begin(), charQueue_.end(), [](const CharInfo *c0, const CharInfo *c1) {
            return c0->speed < c1->speed;
        });
    }
    auto *ch = charQueue_.back();
    cameraX_ = ch->x;
    cameraY_ = ch->y;
    drawDirty_ = true;
    if (ch->side == 1 || ch->autoControl) {
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
    if (ch->steps) { n.emplace_back(L"移動"); menuIndices.emplace_back(0); }
    n.emplace_back(L"攻擊"); menuIndices.emplace_back(1);
    auto *charInfo = mem::gSaveData.charInfo[ch->id];
    if (charInfo) {
        if (charInfo->medic) {
            n.emplace_back(L"醫療"); menuIndices.emplace_back(2);
        }
        if (charInfo->poison) {
            n.emplace_back(L"用毒"); menuIndices.emplace_back(3);
        }
        if (charInfo->depoison) {
            n.emplace_back(L"解毒"); menuIndices.emplace_back(4);
        }
    }
    n.emplace_back(L"物品"); menuIndices.emplace_back(5);
    n.emplace_back(L"等待"); menuIndices.emplace_back(6);
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
            stage_ = Selecting;
            drawDirty_ = true;
            delete menu;
            return;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            break;
        case 8:
            charQueue_.pop_back();
            stage_ = Idle;
            delete menu;
            return;
        case 9:
        default:
            break;
        }
    }, []()->bool {
        return false;
    });
}

struct CompareSelCells {
    bool operator()(const WarField::SelectableCell *a, const WarField::SelectableCell *b) {
        return a->moves > b->moves;
    }
};

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

}
