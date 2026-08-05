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
#include <cstdint>

namespace hojy::scene {

class EndScreen: public NodeWithCache {
public:
    using NodeWithCache::NodeWithCache;

    [[nodiscard]] bool init();
    void update() override;
    void render() const override;
    void applyInputLogic() override;
    void consumeKeyIntent(Key key) override;

private:
    void makeCache() override;
    [[nodiscard]] int stage3FrameTotal() const;

private:
    TextureMgr wordTexMgr_, imgTexMgr_;
    std::int16_t w_ = 0, h_ = 0, tw_ = 0, th_ = 0;
    int stage_ = 0, frame_ = 0, frameTotal_ = 0;
    Key pendingInput_ = KeyNone;
};

}
