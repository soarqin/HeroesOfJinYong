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

#include "colorpalette.hh"

#include "core/config.hh"
#include "util/file.hh"

namespace hojy::scene {

ColorPalette gNormalPalette, gEndPalette, gMaskPalette;

bool ColorPalette::load(const std::string &name) {
    auto ifs = util::File::open(core::config.dataFilePath(name + ".COL"));
    if (!ifs) { return false; }
    std::array<std::uint32_t, 256> loaded{};
    for (size_t i = 0; i < 256; ++i) {
        std::uint8_t c[3]{};
        if (ifs.read(c, sizeof(c)) != sizeof(c)) {
            return false;
        }
        const auto red = std::uint32_t(c[2]) * 4U;
        const auto green = std::uint32_t(c[1]) * 4U;
        const auto blue = std::uint32_t(c[0]) * 4U;
        // Preserve the original byte order after its explicit BGR swap.
        loaded[i] = 0xFF000000U | (blue << 16U) | (green << 8U) | red;
    }
    loaded[0] = 0;
    palette_ = loaded;
    return true;
}

void ColorPalette::create(const std::array<std::uint32_t, 256> &colors) {
    palette_ = colors;
}

}
