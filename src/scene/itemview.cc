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
#include "mem/action.hh"
#include "util/conv.hh"
#include <fmt/format.h>

namespace hojy::scene {

enum {
    ItemCellSpacing = 5,
};

void ItemView::show(bool inBattle, const std::function<void(std::int16_t)> &resultFunc) {
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
    const auto &mgr = gWindow->mapTextureMgr();
    const auto *tex = mgr[3501];
    int scale0 = gWindow->width() / 320, scale1 = gWindow->height() / 240;
    scale_ = std::max(1, std::min(scale0, scale1));
    cellWidth_ = tex->width() * scale_;
    cellHeight_ = tex->height() * scale_;
    cols_ = (width_ + ItemCellSpacing - SubWindowBorder * 2) / (cellWidth_ + ItemCellSpacing);
    rows_ = (height_ + ItemCellSpacing - SubWindowBorder * 2) / (cellHeight_ + ItemCellSpacing);
    width_ = (cellWidth_ + ItemCellSpacing) * cols_ - ItemCellSpacing + SubWindowBorder * 2;
    height_ = (cellHeight_ + ItemCellSpacing) * rows_ - ItemCellSpacing + SubWindowBorder * 2;
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
                if (mem::useItem(charId_, id, changes)) {
                    std::vector<std::wstring> messages = {L"使用 " + util::big5Conv.toUnicode(itemInfo->name)};
                    for (auto &c: changes) {
                        if (c.second) {
                            messages.emplace_back(fmt::format(L"{} 提昇 {}", mem::propToName(c.first), c.second));
                        } else {
                            messages.emplace_back(fmt::format(L"{} 減少 {}", mem::propToName(c.first), c.second));
                        }
                    }
                    auto *msgBox = new MessageBox(parent_, 0, 0, gWindow->width(), gWindow->height());
                    msgBox->popup(messages, MessageBox::PressToCloseThis);
                    auto fn = std::move(resultFunc_);
                    delete this;
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
            clm->initWithTeamMembers({type == 1 ? L"誰要配備 " : L"誰要修練 " + util::big5Conv.toUnicode(itemInfo->name)}, {},
                                     [this, itemInfo, id, type, clm](std::int16_t charId) {
                                         if (type == 2 && itemInfo->user >= 0) {
                                             auto *msgBox = new MessageBox(clm, 0, 0, gWindow->width(), gWindow->height());
                                             msgBox->setYesNoHandler([this, id, charId, clm]() {
                                                 if (!mem::equipItem(charId, id)) {
                                                     auto *msgBox = new MessageBox(parent_,
                                                                                   0,
                                                                                   0,
                                                                                   gWindow->width(),
                                                                                   gWindow->height());
                                                     msgBox->popup({L"此人不適合修練此物品"}, MessageBox::PressToCloseThis);
                                                 } else {
                                                     update();
                                                 }
                                                 delete clm;
                                             }, [clm]() {
                                                 delete clm;
                                             });
                                             msgBox->popup({L"此物品現在已經有人修練了", L"是否要換人修練？"}, MessageBox::YesNo);
                                         } else {
                                             if (!mem::equipItem(charId, id)) {
                                                 auto *msgBox = new MessageBox(this, 0, 0, gWindow->width(), gWindow->height());
                                                 msgBox->popup({L"此人不適合配備此物品"}, MessageBox::PressToCloseThis);
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
            clm->initWithTeamMembers({L"誰要使用 " + util::big5Conv.toUnicode(itemInfo->name)}, {},
                                     [this, &ipair, itemInfo, id, type, clm](std::int16_t charId) {
                                         std::map<mem::PropType, std::int16_t> changes;
                                         if (ipair.second && mem::useItem(charId, id, changes)) {
                                             std::vector<std::wstring> messages = {L"使用 " + util::big5Conv.toUnicode(itemInfo->name)};
                                             for (auto &c: changes) {
                                                 if (c.second) {
                                                     messages.emplace_back(fmt::format(L"{} 提升 {}", mem::propToName(c.first), c.second));
                                                 } else {
                                                     messages.emplace_back(fmt::format(L"{} 減少 {}", mem::propToName(c.first), c.second));
                                                 }
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

void ItemView::makeCache() {
    cacheBegin();
    renderer_->clear(0, 0, 0, 0);
    renderer_->fillRoundedRect(0, 0, width_, height_, RoundedRectRad, 64, 64, 64, 208);
    renderer_->drawRoundedRect(0, 0, width_, height_, RoundedRectRad, 224, 224, 224, 255);
    int x, y = SubWindowBorder;
    int idx = currTop_ * cols_;
    auto totalSz = int(items_.size());
    if (idx >= totalSz) {
        idx = 0;
        currTop_ = 0;
    }
    auto *ttf = renderer_->ttf();
    int smallFontSize = (ttf->fontSize() * 2 / 3 + 1) & ~1;
    const auto &mgr = gWindow->mapTextureMgr();
    ttf->setColor(236, 236, 236);
    for (int j = rows_; j && idx < totalSz; --j) {
        x = SubWindowBorder;
        for (int i = cols_; i && idx < totalSz; --i, ++idx) {
            auto *tex = mgr[items_[idx].first + data::ItemTexIdStart];
            renderer_->renderTexture(tex, x, y, cellWidth_, cellHeight_, 0, 0, cellWidth_ / 2, cellHeight_ / 2, true);
            auto countStr = std::to_wstring(items_[idx].second);
            int countw = ttf->stringWidth(countStr, smallFontSize);
            ttf->render(countStr, x + cellWidth_ - countw - 4 * scale_, y + cellHeight_ - smallFontSize - 4 * scale_, true, smallFontSize);
            x += cellWidth_ + ItemCellSpacing;
        }
        y += cellHeight_ + ItemCellSpacing;
    }

    idx = currTop_ * cols_ + currSel_;
    const auto *itemInfo = mem::gSaveData.itemInfo[items_[idx].first];
    if (itemInfo) {
        /* show description */
        std::wstring display;
        if (items_[idx].second > 1) {
            display = fmt::format(L"{} x{}", util::big5Conv.toUnicode(itemInfo->name), items_[idx].second);
        } else {
            display = fmt::format(L"{}", util::big5Conv.toUnicode(itemInfo->name), items_[idx].second);
        }
        std::wstring desc;
        if (items_[idx].first == data::ItemIDCompass) {
            auto *map = gWindow->globalMap();
            desc = fmt::format(L"人 ({},{})  船 ({},{})", map->currX(), map->currY(), mem::gSaveData.baseInfo->shipX, mem::gSaveData.baseInfo->shipY);
        } else {
            desc = util::big5Conv.toUnicode(itemInfo->desc);
        }
        auto lineheight = ttf->fontSize() + TextLineSpacing;
        int dx, dy;
        dx = SubWindowBorder;
        if (currSel_ / cols_ * 2 < rows_) {
            /* draw on bottom side */
            dy = height_ - SubWindowBorder * 3 - lineheight * 2 + TextLineSpacing;
        } else {
            /* draw on top side */
            dy = SubWindowBorder;
        }
        int dw = width_ - SubWindowBorder * 2, dh = SubWindowBorder * 2 + lineheight * 2 - TextLineSpacing;
        renderer_->fillRoundedRect(dx, dy, dw, dh, RoundedRectRad, 64, 64, 64, 208);
        renderer_->drawRoundedRect(dx, dy, dw, dh, RoundedRectRad, 224, 224, 224, 255);
        dx += SubWindowBorder;
        dy += SubWindowBorder;
        dw -= SubWindowBorder * 2;
        ttf->setColor(236, 200, 40);
        if (itemInfo->user < 0 || mem::gSaveData.charInfo[itemInfo->user] == nullptr) {
            ttf->render(display, dx + (dw - ttf->stringWidth(display)) / 2, dy, true);
        } else {
            ttf->render(display + L"  (" + util::big5Conv.toUnicode(mem::gSaveData.charInfo[itemInfo->user]->name) + L')', dx + (dw - ttf->stringWidth(display)) / 2, dy, true);
        }
        dy += lineheight;
        ttf->setColor(252, 148, 16);
        ttf->render(desc, dx + (dw - ttf->stringWidth(desc)) / 2, dy, true);
    }
    int sx = SubWindowBorder + (currSel_ % cols_) * (cellWidth_ + ItemCellSpacing);
    int sy = SubWindowBorder + (currSel_ / cols_) * (cellHeight_ + ItemCellSpacing);
    renderer_->drawRect(sx - 1, sy - 1, cellWidth_ + 2, cellHeight_ + 2, 252, 252, 252, 255);
    cacheEnd();
}

}
