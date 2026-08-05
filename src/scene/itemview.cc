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

#include <algorithm>
#include "core/config.hh"

namespace hojy::scene {

enum {
    ItemCellSpacing = 5,
};

ItemView::~ItemView() {
    if (selectionLifetime_) {
        selectionLifetime_->alive = false;
    }
}

void ItemView::show(std::vector<ItemEntry> items,
                    std::unique_ptr<ItemSelectionController> selectionController) {
    if (selectionLifetime_) {
        selectionLifetime_->alive = false;
    }
    auto windowBorder = core::config.windowBorder();
    items_ = std::move(items);
    selectionController_ = std::move(selectionController);
    selectionLifetime_ = std::make_shared<ItemSelectionLifetime>();
    if (selectionController_) {
        selectionController_->bindLifetime(selectionLifetime_);
        selectionController_->bindItems(items_);
    }
    currTop_ = 0;
    currSel_ = 0;
    if (layoutBoundsWidth_ <= 0) { layoutBoundsWidth_ = width_; }
    if (layoutBoundsHeight_ <= 0) { layoutBoundsHeight_ = height_; }
    presentationGeometryReady_ = false;
    requestPresentationRefresh();
}

void ItemView::setItems(std::vector<ItemEntry> items) {
    items_ = std::move(items);
    if (selectionController_) { selectionController_->bindItems(items_); }
    normalizeSelection();
    requestPresentationRefresh();
}

void ItemView::closeItemSelection() {
    if (selectionLifetime_) {
        selectionLifetime_->alive = false;
    }
    requestPresentationCleanup();
}

void ItemView::replaceItemSelection(std::vector<ItemViewEntrySnapshot> items) {
    setItems(std::move(items));
}

void ItemView::showCharacterSelection(CharacterSelectionRequest request) {
    postCommand([request = std::move(request)](SceneCommandContext &context) mutable {
        context.showCharacterSelection(std::move(request));
    });
}

void ItemView::showItemMessage(ItemMessageRequest request) {
    postCommand([request = std::move(request)](SceneCommandContext &context) mutable {
        context.showItemMessage(std::move(request));
    });
}

void ItemView::useQuestItem(std::int16_t itemId) {
    postCommand([itemId](SceneCommandContext &context) {
        context.useQuestItem(itemId);
    });
}

void ItemView::update() {
    normalizeSelection();
    NodeWithCache::update();
}

void ItemView::normalizeSelection() {
    if (cols_ <= 0 || rows_ <= 0 || items_.empty()) {
        currTop_ = 0;
        currSel_ = 0;
        return;
    }
    const int totalRows = (int(items_.size()) + cols_ - 1) / cols_;
    const int maxTop = std::max(0, totalRows - rows_);
    currTop_ = std::clamp(currTop_, 0, maxTop);
    const int first = currTop_ * cols_;
    const int visible = std::min(rows_ * cols_, int(items_.size()) - first);
    currSel_ = std::clamp(currSel_, 0, std::max(0, visible - 1));
}

void ItemView::consumeKeyIntent(Node::Key key) {
    pendingInput_ = key;
}

void ItemView::applyInputLogic() {
    if (!presentationGeometryReady_) { return; }
    const auto key = pendingInput_;
    pendingInput_ = KeyNone;
    normalizeSelection();
    if (items_.empty() || cols_ <= 0 || rows_ <= 0) { return; }
    switch (key) {
    case KeyOK: case KeySpace: {
        const auto id = items_[currSel_ + currTop_ * cols_].itemId;
        if (selectionController_) {
            selectionController_->select(*this, id);
        }
        return;
    }
    case KeyCancel: {
        if (selectionController_) {
            selectionController_->cancel(*this);
        } else {
            requestPresentationCleanup();
        }
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
        normalizeSelection();
        requestPresentationRefresh();
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
        normalizeSelection();
        requestPresentationRefresh();
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
        normalizeSelection();
        requestPresentationRefresh();
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
        normalizeSelection();
        requestPresentationRefresh();
        break;
    }
    default:
        break;
    }
}

void ItemView::prepareRender() {
    const auto *atlas = renderer_ ? renderer_->itemAtlas() : nullptr;
    const auto atlasWidth = renderer_ ? renderer_->itemTexWidth() : 0;
    const auto atlasHeight = renderer_ ? renderer_->itemTexHeight() : 0;
    const bool atlasChanged = atlas != itemAtlas_
        || atlasWidth != itemTexW_ || atlasHeight != itemTexH_;
    itemAtlas_ = atlas;
    itemTexW_ = atlasWidth;
    itemTexH_ = atlasHeight;

    const auto windowBorder = core::config.windowBorder();
    const int scale0 = (viewportWidth_ > 0 ? viewportWidth_ : rootWidth()) / 320;
    const int scale1 = (viewportHeight_ > 0 ? viewportHeight_ : rootHeight()) / 200;
    const int nextScale = std::max(1, std::min(scale0, scale1));
    const int nextCellWidth = itemTexW_ * nextScale;
    const int nextCellHeight = itemTexH_ * nextScale;
    const int nextCols = nextCellWidth > 0
        ? (layoutBoundsWidth_ + ItemCellSpacing - windowBorder * 2)
            / (nextCellWidth + ItemCellSpacing)
        : 0;
    const int nextRows = nextCellHeight > 0
        ? (layoutBoundsHeight_ + ItemCellSpacing - windowBorder * 2)
            / (nextCellHeight + ItemCellSpacing)
        : 0;
    const bool geometryChanged = !presentationGeometryReady_
        || atlasChanged || nextScale != scale_ || nextCellWidth != cellWidth_
        || nextCellHeight != cellHeight_ || nextCols != cols_ || nextRows != rows_;
    scale_ = nextScale;
    cellWidth_ = nextCellWidth;
    cellHeight_ = nextCellHeight;
    cols_ = std::max(0, nextCols);
    rows_ = std::max(0, nextRows);
    if (cellWidth_ > 0 && cellHeight_ > 0 && cols_ > 0 && rows_ > 0) {
        width_ = (cellWidth_ + ItemCellSpacing) * cols_
            - ItemCellSpacing + windowBorder * 2;
        height_ = (cellHeight_ + ItemCellSpacing) * rows_
            - ItemCellSpacing + windowBorder * 2;
        presentationGeometryReady_ = true;
    } else {
        presentationGeometryReady_ = false;
    }
    if (geometryChanged) { requestPresentationRefresh(); }
    NodeWithCache::prepareRender();
}

bool ItemView::prepareTextResources() {
    auto *ttf = renderer_->ttf();
    // Item descriptions contain optional text from the data files.  Match
    // the original lazy glyph rendering: an unsupported glyph is skipped,
    // but it must not make the entire item panel disappear.
    for (const auto &entry: items_) {
        (void)ttf->prepareText(std::to_wstring(entry.count));
        (void)ttf->prepareText(entry.displayText);
        (void)ttf->prepareText(entry.description);
        (void)ttf->prepareText(entry.requirementTitle);
        (void)ttf->prepareText(entry.effectTitle);
        for (const auto &line: entry.requirementLines) {
            (void)ttf->prepareText(line);
        }
        for (const auto &line: entry.effectLines) {
            (void)ttf->prepareText(line);
        }
    }
    return true;
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
    if (totalSz == 0 || cols_ <= 0 || rows_ <= 0) {
        cacheEnd();
        return;
    }
    auto *ttf = renderer_->ttf();
    int smallFontSize = std::max(8, (renderer_->fontSize() * 2 / 3 + 1) & ~1);
    ttf->setColor(236, 236, 236);
    for (int j = rows_; j && idx < totalSz; --j) {
        x = windowBorder;
        for (int i = cols_; i && idx < totalSz; --i, ++idx) {
            if (itemAtlas_ && itemTexW_ > 0 && itemTexH_ > 0) {
                const int columns = std::max(1, 1024 / itemTexW_);
                renderer_->renderTexture(itemAtlas_, x, y, cellWidth_, cellHeight_,
                                         itemTexW_ * (items_[idx].itemId % columns),
                                         itemTexH_ * (items_[idx].itemId / columns),
                                         itemTexW_, itemTexH_, true);
            }
            auto countStr = std::to_wstring(items_[idx].count);
            int countw = ttf->preparedStringWidth(countStr, smallFontSize);
            ttf->renderPrepared(countStr, x + cellWidth_ - countw - 4 * scale_, y + cellHeight_ - smallFontSize - 4 * scale_, true, smallFontSize);
            x += cellWidth_ + ItemCellSpacing;
        }
        y += cellHeight_ + ItemCellSpacing;
    }
    int sx = windowBorder + (currSel_ % cols_) * (cellWidth_ + ItemCellSpacing);
    int sy = windowBorder + (currSel_ / cols_) * (cellHeight_ + ItemCellSpacing);
    renderer_->drawRoundedRect(sx - 1, sy - 1, cellWidth_ + 2, cellHeight_ + 2, 2, 252, 252, 252, 255);

    idx = currTop_ * cols_ + currSel_;
    if (idx < 0 || idx >= totalSz) {
        cacheEnd();
        return;
    }
    const auto &entry = items_[idx];
    {
        auto lineheight = renderer_->fontSize() + TextLineSpacing;
        int dx = 0, dy;
        const auto reqLine = entry.requirementTitle.empty()
            ? 0 : static_cast<int>(entry.requirementLines.size()) + 1;
        const auto addLine = entry.effectTitle.empty()
            ? 0 : static_cast<int>(entry.effectLines.size()) + 1;
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
        ttf->renderPrepared(
            entry.displayText,
            dx + (dw - ttf->preparedStringWidth(entry.displayText)) / 2,
            dy, true);
        dy += lineheight;
        ttf->setColor(252, 148, 16);
        ttf->renderPrepared(
            entry.description,
            dx + (dw - ttf->preparedStringWidth(entry.description)) / 2,
            dy, true);
        if (reqLine) {
            dy += lineheight;
            ttf->setColor(236, 236, 236);
            ttf->renderPrepared(
                entry.requirementTitle,
                dx + (dw - ttf->preparedStringWidth(entry.requirementTitle)) / 2,
                dy, true);
            ttf->setColor(236, 200, 40);
            for (const auto &line: entry.requirementLines) {
                dy += lineheight;
                ttf->renderPrepared(
                    line, dx + (dw - ttf->preparedStringWidth(line)) / 2,
                    dy, true);
            }
        }
        if (addLine) {
            dy += lineheight;
            ttf->setColor(236, 236, 236);
            ttf->renderPrepared(
                entry.effectTitle,
                dx + (dw - ttf->preparedStringWidth(entry.effectTitle)) / 2,
                dy, true);
            ttf->setColor(236, 200, 40);
            for (const auto &line: entry.effectLines) {
                dy += lineheight;
                ttf->renderPrepared(
                    line, dx + (dw - ttf->preparedStringWidth(line)) / 2,
                    dy, true);
            }
        }
    }
    cacheEnd();
}

}
