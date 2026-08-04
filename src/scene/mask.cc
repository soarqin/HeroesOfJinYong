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

#include <algorithm>

namespace hojy::scene {

namespace {

std::uint64_t microsPerAlpha(int interval) {
    const auto speed = core::config.fadeSpeed();
    if (speed <= 0.f) { return 1; }
    const auto value = static_cast<std::uint64_t>(float(4000 * interval) / 3.f / speed);
    return std::max<std::uint64_t>(1, value);
}

}

Mask::Mask(Node *parent, Mask::Type type, int interval):
    Node(parent, parent->x(), parent->y(), parent->width(), parent->height()),
    timeline_(gWindow->currTime(), microsPerAlpha(interval), type == FadeIn) {
}

void Mask::update() {
    timeline_.advance(gWindow->currTime());
    if (timeline_.completed() && !completionSignalled_) {
        completionSignalled_ = true;
        if (parent_) { parent_->fadeEnd(); }
    }
}

void Mask::render() {
    renderer_->fillRect(x_, y_, width_, height_, 0, 0, 0, timeline_.alpha());
}

}
