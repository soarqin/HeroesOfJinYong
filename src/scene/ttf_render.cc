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

#include "ttf.hh"

#include "renderer.hh"
#include "texture.hh"

namespace hojy::scene {

void TTF::setColor(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    altR_[0] = r; altG_[0] = g; altB_[0] = b;
}

void TTF::setAltColor(int index, std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    if (index > 0 && index <= 16) {
        --index;
        altR_[index] = r;
        altG_[index] = g;
        altB_[index] = b;
    }
}

void TTF::render(std::wstring_view str, int x, int y, bool shadow, int fontSize) {
    renderPrepared(str, x, y, shadow, fontSize);
}

void TTF::renderPrepared(std::wstring_view str, int x, int y, bool shadow, int fontSize) {
    if (fontSize < 0) { fontSize = fontSize_; }
    int colorIndex = 0;
    bool hasPrevious = false;
    std::uint32_t previous = 0;
    for (const auto raw: str) {
        const auto ch = static_cast<std::uint32_t>(raw);
        if (ch > 0 && ch < 17) { colorIndex = ch - 1; continue; }
        if (ch < 32) {
            hasPrevious = false;
            continue;
        }
        const auto key = (std::uint64_t(fontSize) << 32) | std::uint64_t(ch);
        const auto ite = fontCache_.find(key);
        if (ite == fontCache_.end() || ite->second.advW == 0) {
            hasPrevious = false;
            continue;
        }
        const auto &fd = ite->second;
        if (hasPrevious) {
            x += preparedKerningAdvance(previous, ch, fontSize);
        }
        if (fd.w == 0 || fd.h == 0 || fd.rpidx >= textures_.size()
            || !textures_[fd.rpidx] || !textures_[fd.rpidx]->valid()) {
            x += effectiveAdvance(fd);
            previous = ch;
            hasPrevious = true;
            continue;
        }
        auto *tex = textures_[fd.rpidx];
        if (shadow) {
            tex->setBlendColor(0, 0, 0, 255);
            renderer_->renderTexture(tex, x + fd.ix0 + 2, y + fd.iy0 + 2,
                                     fd.rpx, fd.rpy, fd.w, fd.h, true);
        }
        tex->setBlendColor(altR_[colorIndex], altG_[colorIndex], altB_[colorIndex], 255);
        renderer_->renderTexture(tex, x + fd.ix0, y + fd.iy0,
                                 fd.rpx, fd.rpy, fd.w, fd.h, true);
        x += effectiveAdvance(fd);
        previous = ch;
        hasPrevious = true;
    }
}

void TTF::renderPrepared(std::wstring_view str, int x, int y, bool shadow, int fontSize,
                         std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    if (fontSize < 0) { fontSize = fontSize_; }
    bool hasPrevious = false;
    std::uint32_t previous = 0;
    for (const auto raw: str) {
        const auto ch = static_cast<std::uint32_t>(raw);
        if (ch > 0 && ch < 17) { continue; }
        if (ch < 32) {
            hasPrevious = false;
            continue;
        }
        const auto key = (std::uint64_t(fontSize) << 32) | std::uint64_t(ch);
        const auto ite = fontCache_.find(key);
        if (ite == fontCache_.end() || ite->second.advW == 0) {
            hasPrevious = false;
            continue;
        }
        const auto &fd = ite->second;
        if (hasPrevious) {
            x += preparedKerningAdvance(previous, ch, fontSize);
        }
        if (fd.w == 0 || fd.h == 0 || fd.rpidx >= textures_.size()
            || !textures_[fd.rpidx] || !textures_[fd.rpidx]->valid()) {
            x += effectiveAdvance(fd);
            previous = ch;
            hasPrevious = true;
            continue;
        }
        auto *tex = textures_[fd.rpidx];
        if (shadow) {
            tex->setBlendColor(0, 0, 0, 255);
            renderer_->renderTexture(tex, x + fd.ix0 + 2, y + fd.iy0 + 2,
                                     fd.rpx, fd.rpy, fd.w, fd.h, true);
        }
        tex->setBlendColor(r, g, b, 255);
        renderer_->renderTexture(tex, x + fd.ix0, y + fd.iy0,
                                 fd.rpx, fd.rpy, fd.w, fd.h, true);
        x += effectiveAdvance(fd);
        previous = ch;
        hasPrevious = true;
    }
}

}
