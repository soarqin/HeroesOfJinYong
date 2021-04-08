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

#include "rectpacker.hh"

#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#include <external/stb_rect_pack.h>

namespace hojy::scene {

struct RectPackData {
    explicit RectPackData(int nodeCount): nodes(new stbrp_node[nodeCount]) {
    }
    ~RectPackData() {
        delete[] nodes;
    }
    stbrp_context context = {};
    stbrp_node *nodes;
};

RectPacker::RectPacker(int width, int height): width_(width), height_(height) {
}

RectPacker::~RectPacker() {
    for (auto *rpd: rectpackData_) {
        delete rpd;
    }
    rectpackData_.clear();
}

int RectPacker::pack(std::uint16_t w, std::uint16_t h, std::int16_t &x, std::int16_t &y) {
    if (rectpackData_.empty()) {
        newRectPack();
    }
    auto sz = int(rectpackData_.size());
    stbrp_rect rc = {0, w, h};
    int rpidx = -1;
    for (int i = 0; i < sz; ++i) {
        auto &rpd = rectpackData_[i];
        if (stbrp_pack_rects(&rpd->context, &rc, 1)) {
            rpidx = i;
            break;
        }
    }
    if (rpidx < 0) {
        /* No space to hold the bitmap,
         * create a new bitmap */
        newRectPack();
        auto &rpd = rectpackData_.back();
        if (stbrp_pack_rects(&rpd->context, &rc, 1)) {
            rpidx = int(rectpackData_.size()) - 1;
        }
    }
    x = rc.x;
    y = rc.y;
    return rpidx;
}

void RectPacker::newRectPack() {
    rectpackData_.resize(rectpackData_.size() + 1);
    auto *&rpd = rectpackData_.back();
    rpd = new RectPackData(width_);
    stbrp_init_target(&rpd->context, width_, height_, rpd->nodes, width_);
}

}
