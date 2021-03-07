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
#include "texture.hh"

namespace hojy::scene {

class MenuYesNo;

class Title final: public NodeWithCache {
public:
    using NodeWithCache::NodeWithCache;
    ~Title() override;

    void init();
    void handleKeyInput(Key key) override;
    void handleTextInput(const std::wstring &str) override;

private:
    void makeCache() override;
    void doRandomBaseInfo();
    void drawProperty(const std::wstring &name, std::int16_t value, std::int16_t maxValue, int x, int y, int h, int mpType = -1);

private:
    TextureMgr titleTextureMgr_;
    Texture *big_ = nullptr;
    MenuYesNo *menu_ = nullptr;
    int mode_ = 0;
    size_t currSel_ = 0;
    std::wstring mainCharName_;
};

}
