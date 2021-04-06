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

#include "itemview.hh"

#include "window.hh"
#include "charlistmenu.hh"
#include "mem/savedata.hh"
#include "mem/strings.hh"
#include "core/config.hh"
#include <fmt/format.h>

namespace hojy::scene {

enum {
    ItemCellSpacing = 5,
};

void ItemView::show(bool inBattle, const std::function<void(std::int16_t)> &resultFunc) {
    auto windowBorder = core::config.windowBorder();
    inBattle_ = inBattle;
    resultFunc_ = resultFunc;
    for (auto &p: mem::gBag.items()) {
        if (inBattle) {
            const auto *itemInfo = mem::gSaveData.itemInfo[p.first];
            if (!itemInfo || (itemInfo->itemType != 3 && itemInfo->itemType != 4)) {
                continue;
            }
        }
        items_.emplace_back(std::make_pair(p.first, p.second));
    }
    int scale0 = gWindow->width() / 320, scale1 = gWindow->height() / 200;
    scale_ = std::max(1, std::min(scale0, scale1));
    cellWidth_ = gWindow->itemTexWidth() * scale_;
    cellHeight_ = gWindow->itemTexHeight() * scale_;
    cols_ = (width_ + ItemCellSpacing - windowBorder * 2) / (cellWidth_ + ItemCellSpacing);
    rows_ = (height_ + ItemCellSpacing - windowBorder * 2) / (cellHeight_ + ItemCellSpacing);
    width_ = (cellWidth_ + ItemCellSpacing) * cols_ - ItemCellSpacing + windowBorder * 2;
    height_ = (cellHeight_ + ItemCellSpacing) * rows_ - ItemCellSpacing + windowBorder * 2;
}

void ItemView::handleKeyInput(Node::Key key) {
    switch (key) {
    case KeyOK: case KeySpace: {
        auto &ipair = items_[currSel_ + currTop_ * cols_];
        std::int16_t id = ipair.first;
        const auto *itemInfo = mem::gSaveData.itemInfo[id];
        if (!itemInfo) { break; }
        if (inBattle_) {
            switch (itemInfo->itemType) {
            case 3: {
                std::map<mem::PropType, std::int16_t> changes;
                if (mem::useItem(charInfo_, id, changes)) {
                    auto fn = std::move(resultFunc_);
                    auto *parent = parent_;
                    delete this;
                    auto *msgBox = popupUseResult(parent, id, changes);
                    msgBox->setCloseHandler([fn] {
                        if (fn) { fn(-1); }
                    });
                } else {
                    delete this;
                }
                return;
            }
            case 4: {
                auto fn = std::move(resultFunc_);
                delete this;
                if (fn) { fn(id); }
                return;
            }
            default:
                break;
            }
            return;
        }
        auto type = itemInfo->itemType;
        switch (type) {
        case 1:
        case 2: {
            auto *clm = new CharListMenu(this, 0, 0, width_, height_);
            clm->initWithTeamMembers({GETTEXT(type == 1 ? 38 : 39) + GETITEMNAME(id)}, {},
                                     [this, itemInfo, id, type, clm](std::int16_t charId) {
                                         if (type == 2 && itemInfo->user >= 0) {
                                             auto *msgBox = new MessageBox(clm, 0, 0, gWindow->width(), gWindow->height());
                                             msgBox->setYesNoHandler([this, id, charId, clm]() {
                                                 if (mem::skillFull(charId)) {
                                                     auto *msgBox = new MessageBox(parent_,
                                                                                   0,
                                                                                   0,
                                                                                   gWindow->width(),
                                                                                   gWindow->height());
                                                     msgBox->popup({GETTEXT(42)}, MessageBox::PressToCloseThis);
                                                 } else if (!mem::equipItem(charId, id)) {
                                                     auto *msgBox = new MessageBox(parent_,
                                                                                   0,
                                                                                   0,
                                                                                   gWindow->width(),
                                                                                   gWindow->height());
                                                     msgBox->popup({GETTEXT(43)}, MessageBox::PressToCloseThis);
                                                 } else {
                                                     update();
                                                 }
                                                 delete clm;
                                             }, [clm]() {
                                                 delete clm;
                                             });
                                             msgBox->popup({GETTEXT(44), GETTEXT(45)}, MessageBox::YesNo);
                                         } else {
                                             if (!mem::equipItem(charId, id)) {
                                                 auto *msgBox = new MessageBox(this, 0, 0, gWindow->width(), gWindow->height());
                                                 msgBox->popup({GETTEXT(46)}, MessageBox::PressToCloseThis);
                                             } else {
                                                 update();
                                             }
                                             delete clm;
                                         }
                                     });
            clm->makeCenter(width_, height_, x_, y_);
            return;
        }
        case 3: {
            int x = width_ / 3, y = height_ * 2 / 7;
            auto *clm = new CharListMenu(this, x, y, width_ - x, height_ - y);
            clm->initWithTeamMembers({GETTEXT(36) + L' ' + GETITEMNAME(id)}, {},
                                     [this, &ipair, id](std::int16_t charId) {
                                         std::map<mem::PropType, std::int16_t> changes;
                                         if (ipair.second && mem::useItem(mem::gSaveData.charInfo[charId], id, changes)) {
                                             std::vector<std::wstring> messages = {GETTEXT(37) + L' ' + GETITEMNAME(id)};
                                             for (auto &c: changes) {
                                                 messages.emplace_back(fmt::format(L"{} {} {}", mem::propToName(c.first), GETTEXT(c.second ? 34 : 35), c.second));
                                             }
                                             auto *msgBox = new MessageBox(this, 0, 0, gWindow->width(), gWindow->height());
                                             msgBox->popup(messages, MessageBox::PressToCloseParent);
                                         } else {
                                             delete this;
                                         }
                                     });
            return;
        }
        case 4:
            return;
        default:
            break;
        }
        auto func = std::move(resultFunc_);
        gWindow->closePopup();
        if (func) { func(id); }
        return;
    }
    case KeyCancel: {
        auto fn = std::move(closeHandler_);
        delete this;
        if (fn) { fn(); }
        return;
    }
    case KeyUp:
        if (currSel_ < cols_) {
            if (currTop_ == 0) {
                int sz = int(items_.size());
                int totalRows = (sz + cols_ - 1) / cols_;
                if (totalRows <= rows_) {
                    currSel_ = currSel_ + (totalRows - 1) * cols_;
                    if (currSel_ >= sz) currSel_ -= cols_;
                } else {
                    currTop_ = totalRows - rows_;
                    currSel_ = currSel_ + (rows_ - 1) * cols_;
                    if (currSel_ + currTop_ * cols_ >= sz) currSel_ -= cols_;
                }
            } else {
                --currTop_;
            }
        } else {
            currSel_ -= cols_;
        }
        update();
        break;
    case KeyLeft:
        if (currSel_ == 0) {
            if (currTop_ == 0) {
                int sz = int(items_.size());
                int totalRows = (sz + cols_ - 1) / cols_;
                if (totalRows > rows_) {
                    currTop_ = totalRows - rows_;
                }
                currSel_ = sz - 1 - currTop_ * cols_;
            } else {
                --currTop_;
                currSel_ = cols_ - 1;
            }
        } else {
            --currSel_;
        }
        update();
        break;
    case KeyRight: {
        int sz = int(items_.size());
        if (++currSel_ + currTop_ * cols_ >= sz) {
            currSel_ = 0;
            currTop_ = 0;
        } else {
            if (currSel_ >= rows_ * cols_) {
                ++currTop_;
                currSel_ -= cols_;
            }
        }
        update();
        break;
    }
    case KeyDown: {
        currSel_ += cols_;
        int sz = int(items_.size());
        if (currSel_ + currTop_ * cols_ >= sz) {
            currSel_ %= cols_;
            currTop_ = 0;
        } else {
            if (currSel_ >= rows_ * cols_) {
                ++currTop_;
                currSel_ -= cols_;
            }
        }
        update();
        break;
    }
    default:
        break;
    }
}

MessageBox *ItemView::popupUseResult(Node *parent, std::int16_t id, const std::map<mem::PropType, std::int16_t> &changes) {
    std::vector<std::wstring> messages = {GETTEXT(37) + L' ' + GETITEMNAME(id)};
    for (auto &c: changes) {
        messages.emplace_back(fmt::format(L"{} {} {}", mem::propToName(c.first), GETTEXT(c.second ? 34 : 35), c.second));
    }
    auto *msgBox = new MessageBox(parent, 0, 0, gWindow->width(), gWindow->height());
    msgBox->popup(messages, MessageBox::PressToCloseThis);
    return msgBox;
}

void ItemView::makeCache() {
    cacheBegin();
    auto windowBorder = core::config.windowBorder();
    renderer_->clear(0, 0, 0, 0);
    renderer_->fillRoundedRect(0, 0, width_, height_, windowBorder, 64, 64, 64, 208);
    renderer_->drawRoundedRect(0, 0, width_, height_, windowBorder, 224, 224, 224, 255);
    int x, y = windowBorder;
    int idx = currTop_ * cols_;
    auto totalSz = int(items_.size());
    if (idx >= totalSz) {
        idx = 0;
        currTop_ = 0;
    }
    auto *ttf = renderer_->ttf();
    int smallFontSize = std::max(8, (ttf->fontSize() * 2 / 3 + 1) & ~1);
    const auto &mgr = gWindow->mapTextureMgr();
    ttf->setColor(236, 236, 236);
    auto *gmap = gWindow->globalMap();
    for (int j = rows_; j && idx < totalSz; --j) {
        x = windowBorder;
        for (int i = cols_; i && idx < totalSz; --i, ++idx) {
            gWindow->renderItemTexture(items_[idx].first, x, y, cellWidth_, cellHeight_);
            auto countStr = std::to_wstring(items_[idx].second);
            int countw = ttf->stringWidth(countStr, smallFontSize);
            ttf->render(countStr, x + cellWidth_ - countw - 4 * scale_, y + cellHeight_ - smallFontSize - 4 * scale_, true, smallFontSize);
            x += cellWidth_ + ItemCellSpacing;
        }
        y += cellHeight_ + ItemCellSpacing;
    }
    int sx = windowBorder + (currSel_ % cols_) * (cellWidth_ + ItemCellSpacing);
    int sy = windowBorder + (currSel_ / cols_) * (cellHeight_ + ItemCellSpacing);
    renderer_->drawRoundedRect(sx - 1, sy - 1, cellWidth_ + 2, cellHeight_ + 2, 2, 252, 252, 252, 255);

    idx = currTop_ * cols_ + currSel_;
    auto itemId = items_[idx].first;
    const auto *itemInfo = mem::gSaveData.itemInfo[itemId];
    if (itemInfo) {
        /* show description */
        std::wstring display;
        if (items_[idx].second > 1) {
            display = fmt::format(L"{} x{}", GETITEMNAME(itemId), items_[idx].second);
        } else {
            display = fmt::format(L"{}", GETITEMNAME(itemId), items_[idx].second);
        }
        std::wstring desc;
        if (items_[idx].first == data::ItemIDCompass) {
            auto *map = gWindow->globalMap();
            if (core::config.shipLogicEnabled()) {
                desc = fmt::format(GETTEXT(40), map->currX(), map->currY(),
                                   mem::gSaveData.baseInfo->shipX, mem::gSaveData.baseInfo->shipY);
            } else {
                desc = fmt::format(GETTEXT(116), map->currX(), map->currY());
            }
        } else {
            desc = GETITEMDESC(itemId);
        }
        auto lineheight = ttf->fontSize() + TextLineSpacing;
        int dx = 0, dy;
        int addLine = 0, reqLine = 0;
        std::wstring addStr[2], reqStr[2];
        switch (itemInfo->itemType) {
        case 1:
        case 2:
            if (itemInfo->charOnly >= 0) {
                reqStr[reqStr[0].size() < 24 ? 0 : 1] += fmt::format(L" {}", GETCHARNAME(itemInfo->charOnly));
            }
            if (itemInfo->reqMpType == 0 || itemInfo->reqMpType == 1) {
                reqStr[reqStr[0].size() < 24 ? 0 : 1] += fmt::format(L" {}={}", GETTEXT(5), GETTEXT(119 + itemInfo->reqMpType));
            }
#define CheckReq(n, textid) \
            if (itemInfo->n != 0) { \
                reqStr[reqStr[0].size() < 24 ? 0 : 1] += fmt::format(L" {}{}{}", GETTEXT(textid), GETTEXT(121 + (itemInfo->n < 0 ? 1 : 0)), std::abs(itemInfo->n)); \
            }
            CheckReq(reqMp, 26)
            CheckReq(reqAttack, 101)
            CheckReq(reqSpeed, 9)
            CheckReq(reqPoison, 104)
            CheckReq(reqMedic, 103)
            CheckReq(reqDepoison, 105)
            CheckReq(reqFist, 106)
            CheckReq(reqSword, 107)
            CheckReq(reqBlade, 108)
            CheckReq(reqSpecial, 123)
            CheckReq(reqThrowing, 109)
            CheckReq(reqPotential, 29)
#undef CheckReq
            if (!reqStr[1].empty()) {
                reqLine = 3;
            } else if (!reqStr[0].empty()) {
                reqLine = 2;
            }
            /* fallthrough */
        case 3: case 4:
            if (itemInfo->skillId > 0) {
                addStr[addStr[0].size() < 24 ? 0 : 1] += fmt::format(L"{}{}", GETTEXT(130), GETSKILLNAME(itemInfo->skillId));
            }
            if (itemInfo->addDoubleAttack) {
                addStr[addStr[0].size() < 24 ? 0 : 1] += fmt::format(L"{}{}", GETTEXT(130), GETTEXT(22));
            }
            if (itemInfo->changeMpType > 0) {
                addStr[addStr[0].size() < 24 ? 0 : 1] += fmt::format(L"{}{}{}", GETTEXT(5), GETTEXT(128), GETTEXT(itemInfo->changeMpType == 1 ? 120 : 129));
            }
#define CheckAdd(n, textid) \
            if (itemInfo->n != 0) { \
                addStr[addStr[0].size() < 24 ? 0 : 1] += fmt::format(L" {}{:+}", GETTEXT(textid), itemInfo->n); \
            }
            CheckAdd(addHp, 25)
            CheckAdd(addMaxHp, 25)
            CheckAdd(addPoisoned, 3)
            CheckAdd(addStamina, 4)
            CheckAdd(addMp, 26)
            CheckAdd(addMaxMp, 26)
            CheckAdd(addAttack, 101)
            CheckAdd(addSpeed, 9)
            CheckAdd(addDefence, 102)
            CheckAdd(addMedic, 103)
            CheckAdd(addPoison, 104)
            CheckAdd(addDepoison, 105)
            CheckAdd(addAntipoison, 14)
            CheckAdd(addFist, 106)
            CheckAdd(addSword, 107)
            CheckAdd(addBlade, 108)
            CheckAdd(addSpecial, 123)
            CheckAdd(addThrowing, 109)
            CheckAdd(addKnowledge, 20)
            CheckAdd(addIntegrity, 21)
            CheckAdd(addPoisonAmp, 23)
#undef CheckAdd
            if (!addStr[1].empty()) {
                addLine = 3;
            } else if (!addStr[0].empty()) {
                addLine = 2;
            }
            break;
        default:
            break;
        }
        if (currSel_ / cols_ * 2 < rows_) {
            /* draw on bottom side */
            dy = height_ - lineheight * (addLine + reqLine + 2) - windowBorder * 2 + TextLineSpacing;
        } else {
            /* draw on top side */
            dy = 0;
        }
        int dw = width_, dh = windowBorder * 2 + lineheight * (addLine + reqLine + 2) - TextLineSpacing;
        renderer_->fillRoundedRect(dx, dy, dw, dh, windowBorder, 64, 64, 64, 208);
        renderer_->drawRoundedRect(dx, dy, dw, dh, windowBorder, 224, 224, 224, 255);
        dx += windowBorder;
        dy += windowBorder;
        dw -= windowBorder * 2;
        ttf->setColor(236, 200, 40);
        if (itemInfo->user < 0 || mem::gSaveData.charInfo[itemInfo->user] == nullptr) {
            ttf->render(display, dx + (dw - ttf->stringWidth(display)) / 2, dy, true);
        } else {
            ttf->render(display + L"  (" + GETCHARNAME(itemInfo->user) + L')', dx + (dw - ttf->stringWidth(display)) / 2, dy, true);
        }
        dy += lineheight;
        ttf->setColor(252, 148, 16);
        ttf->render(desc, dx + (dw - ttf->stringWidth(desc)) / 2, dy, true);
        if (reqLine) {
            dy += lineheight;
            ttf->setColor(236, 236, 236);
            const auto &txt = GETTEXT(116 + itemInfo->itemType);
            ttf->render(txt, dx + (dw - ttf->stringWidth(txt)) / 2, dy, true);
            ttf->setColor(236, 200, 40);
            for (int i = 0; i < reqLine - 1; ++i) {
                dy += lineheight;
                ttf->render(reqStr[i], dx + (dw - ttf->stringWidth(reqStr[i])) / 2, dy, true);
            }
        }
        if (addLine) {
            dy += lineheight;
            ttf->setColor(236, 236, 236);
            const auto &txt = GETTEXT(123 + itemInfo->itemType);
            ttf->render(txt, dx + (dw - ttf->stringWidth(txt)) / 2, dy, true);
            ttf->setColor(236, 200, 40);
            for (int i = 0; i < addLine - 1; ++i) {
                dy += lineheight;
                ttf->render(addStr[i], dx + (dw - ttf->stringWidth(addStr[i])) / 2, dy, true);
            }
        }
    }
    cacheEnd();
}

}
