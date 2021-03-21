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

#include <SDL.h>

namespace hojy::scene {

ColorPalette gNormalPalette, gEndPalette, gMaskPalette;

ColorPalette::~ColorPalette() {
    if (paletteObj_) {
        SDL_FreePalette(static_cast<SDL_Palette *>(paletteObj_));
        paletteObj_ = nullptr;
    }
}

void ColorPalette::load(const std::string &name) {
    auto ifs = util::File::open(core::config.dataFilePath(name + ".COL"));
    std::uint8_t c[4] = {0, 0, 0, 0xFF};
    for (size_t i = 0; i < 256; ++i) {
        ifs.read(c, 3);
        for (int j = 0; j < 3; ++j) {
            c[j] = std::uint8_t(std::uint32_t(c[j]) * 4);
        }
        palette_[i] = *reinterpret_cast<std::uint32_t*>(c);
    }
    palette_[0] = 0;
    createObj();
}

void ColorPalette::create(const std::array<std::uint32_t, 256> &colors) {
    palette_ = colors;
    createObj();
}

void ColorPalette::createObj() {
    auto sz = palette_.size();
    auto *paletteObj = SDL_AllocPalette(sz);
    SDL_SetPaletteColors(paletteObj, reinterpret_cast<const SDL_Color*>(palette_.data()), 0, sz);
    paletteObj_ = paletteObj;
}

}
