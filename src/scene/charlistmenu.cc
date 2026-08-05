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

#include "charlistmenu.hh"

#include "messagebox.hh"
#include "core/config.hh"

namespace hojy::scene {

CharListMenu::~CharListMenu() {
    delete msgBox_;
}

std::vector<std::int16_t> CharListMenu::getSelectedCharIds() const {
    std::vector<std::int16_t> res;
    if (checkbox_) {
        const auto count = std::min(selected_.size(), charIdList_.size());
        for (size_t i = 0; i < count; ++i) {
            if (selected_[i]) {
                res.emplace_back(charIdList_[i]);
            }
        }
    } else if (currIndex_ >= 0
               && static_cast<std::size_t>(currIndex_) < charIdList_.size()) {
        res.emplace_back(charIdList_[static_cast<std::size_t>(currIndex_)]);
    }
    return res;
}

void CharListMenu::init(
        CharacterListSnapshot snapshot,
        std::shared_ptr<MenuSelectionSink> selectionSink) {
    layoutAnchorX_ = x_;
    layoutAnchorY_ = y_;
    charIdList_.clear();
    std::vector<std::wstring> names;
    std::vector<std::wstring> values;
    charIdList_.reserve(snapshot.rows.size());
    names.reserve(snapshot.rows.size());
    values.reserve(snapshot.rows.size());
    for (auto &row: snapshot.rows) {
        if (row.characterId < 0) { continue; }
        charIdList_.push_back(row.characterId);
        names.push_back(std::move(row.name));
        values.push_back(std::move(row.valueText));
    }
    if (!snapshot.title.empty()) {
        auto *msgBox = new MessageBox(renderer_, x_, y_, rootWidth() - x_, rootHeight() - y_);
        msgBox_ = msgBox;
        msgBox->popup(snapshot.title, MessageBox::Normal, MessageBox::TopLeft);
    }
    setTitle(std::move(snapshot.columnTitle));
    popup(names, values);
    std::vector<std::int32_t> entryIds;
    entryIds.reserve(charIdList_.size());
    for (const auto id: charIdList_) {
        entryIds.push_back(id);
    }
    setEntryIds(entryIds);
    setSelectionSink(std::move(selectionSink));
}

void CharListMenu::makeCenter(int w, int h, int x, int y) {
    centerWidth_ = w;
    centerHeight_ = h;
    centerX_ = x;
    centerY_ = y;
    centerRequested_ = true;
    requestPresentationRefresh();
}

void CharListMenu::prepareRender() {
    if (renderer_ && renderer_->ttf()) {
        renderer_->ttf()->setAltColor(15, 252, 100, 12);
    }
    if (msgBox_) {
        msgBox_->dispatchPrepareRender();
    }
    NodeWithCache::prepareRender();
    if (!renderCacheReady() || msgBox_ && !static_cast<NodeWithCache *>(msgBox_)->renderCacheReady()) {
        return;
    }

    int baseX = layoutAnchorX_;
    int baseY = layoutAnchorY_;
    if (centerRequested_) {
        baseX = centerX_ + (centerWidth_ - width_) / 2;
        baseY = centerY_ + (centerHeight_ - height_) / 2;
    }
    if (msgBox_) {
        msgBox_->setPosition(baseX, baseY);
        setPosition(baseX, baseY + msgBox_->height() + core::config.windowBorder());
    } else {
        setPosition(baseX, baseY);
    }
}

void CharListMenu::render() const {
    if (msgBox_) { msgBox_->dispatchRender(); }
    NodeWithCache::render();
}

}
