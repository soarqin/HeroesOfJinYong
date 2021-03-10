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

#include "mask.hh"

#include "window.hh"
#include "core/config.hh"

namespace hojy::scene {

Mask::Mask(Node *parent, Mask::Type type, int interval):
    Node(parent, parent->x(), parent->y(), parent->width(), parent->height()),
    type_(type), interval_(interval), start_(gWindow->currTime()) {
}

void Mask::render() {
    auto alpha = (gWindow->currTime() - start_) / std::chrono::microseconds(int(float(1333 * interval_) / core::config.fadeSpeed()));
    if (alpha > 255) {
        alpha = 255;
        parent_->fadeEnd();
    }
    if (type_ == FadeIn) alpha = 255 - alpha;
    renderer_->fillRect(x_, y_, width_, height_, 0, 0, 0, alpha);
}

}
