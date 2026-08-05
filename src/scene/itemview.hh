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

#pragma once

#include "nodewithcache.hh"
#include "item_selection_controller.hh"

#include <vector>
#include <functional>
#include <cstdint>
#include <memory>

namespace hojy::scene {

class ItemView: public NodeWithCache, private ItemSelectionHost {
public:
    using NodeWithCache::NodeWithCache;
    using ItemEntry = ItemViewEntrySnapshot;

    ~ItemView() override;

    void show(std::vector<ItemEntry> items,
              std::unique_ptr<ItemSelectionController> selectionController);
    void setViewportSize(int width, int height) noexcept {
        viewportWidth_ = width;
        viewportHeight_ = height;
    }
    void setItems(std::vector<ItemEntry> items);
    void update() override;
    void applyInputLogic() override;
    void consumeKeyIntent(Key key) override;
    void prepareRender() override;

protected:
    void closeItemSelection() override;
    void replaceItemSelection(std::vector<ItemViewEntrySnapshot> items) override;
    void showCharacterSelection(CharacterSelectionRequest request) override;
    void showItemMessage(ItemMessageRequest request) override;
    void useQuestItem(std::int16_t itemId) override;

    bool prepareTextResources() override;
    void makeCache() override;
    void normalizeSelection();

protected:
    std::vector<ItemViewEntrySnapshot> items_;
    int cols_ = 0, rows_ = 0;
    int scale_ = 1, cellWidth_ = 0, cellHeight_ = 0;
    int currTop_ = 0, currSel_ = 0;
    std::unique_ptr<ItemSelectionController> selectionController_;
    std::shared_ptr<ItemSelectionLifetime> selectionLifetime_;
    const Texture *itemAtlas_ = nullptr;
    int itemTexW_ = 0, itemTexH_ = 0;
    int layoutBoundsWidth_ = 0, layoutBoundsHeight_ = 0;
    int viewportWidth_ = 0, viewportHeight_ = 0;
    bool presentationGeometryReady_ = false;
    Key pendingInput_ = KeyNone;
};

}
