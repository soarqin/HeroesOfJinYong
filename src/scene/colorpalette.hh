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

#include <array>
#include <string>
#include <cstdint>

namespace hojy::scene {

class ColorPalette final {
public:
    void load(const std::string &name);
    void create(const std::array<std::uint32_t, 256> &colors);
    [[nodiscard]] constexpr size_t size() const { return palette_.size(); }
    [[nodiscard]] const std::uint32_t *colors() const { return palette_.data(); }

private:
    std::array<std::uint32_t, 256> palette_;
};

extern ColorPalette gNormalPalette, gEndPalette, gMaskPalette;

}
