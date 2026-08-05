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
#include "status_snapshot.hh"

#include <functional>

namespace hojy::scene {

class StatusView: public NodeWithCache {
public:
    using NodeWithCache::NodeWithCache;

    void show(CharacterStatusSnapshot snapshot);
    void setHeadTextureProvider(std::function<const Texture *(std::int16_t)> provider) {
        headTextureProvider_ = std::move(provider);
    }
    void setBattleAnchor(bool left, int width, int height, int border) noexcept;

    void applyInputLogic() override;
    void consumeKeyIntent(Key key) override;
    void prepareRender() override;

protected:
    bool prepareTextResources() override;
    void ensureLayout() override;
    void makeCache() override;

protected:
    CharacterStatusSnapshot data_ {};
    bool simpleMode_ = false;
    bool battleAnchorEnabled_ = false;
    bool battleAnchorLeft_ = true;
    int battleAreaWidth_ = 0;
    int battleAreaHeight_ = 0;
    int battleAnchorBorder_ = 0;
    std::function<const Texture *(std::int16_t)> headTextureProvider_;
    Key pendingInput_ = KeyNone;
};

}
