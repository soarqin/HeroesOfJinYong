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

#include "texture.hh"

#include <SDL.h>

namespace hojy::scene {

TextureMgr mapTextureMgr;

Texture *Texture::createAsTarget(Renderer *renderer, int w, int h) {
    auto *tex = new Texture;
    auto *ren = static_cast<SDL_Renderer*>(renderer->renderer_);
    auto *texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    tex->data_ = texture;
    tex->width_ = w;
    tex->height_ = h;
    return tex;
}

bool Texture::loadFromRLE(Renderer *renderer, const std::vector<std::uint8_t> &data, void *palette) {
    size_t left = data.size();
    if (left < 8) {
        return false;
    }
    const auto *buf = data.data();
    struct Header {
        std::int16_t w, h, x, y;
    };
    const auto *hdr = reinterpret_cast<const Header*>(buf);
    if (hdr->w == 0 && hdr->h == 0) {
        return false;
    }
    buf += 8;
    left -= 8;
    std::vector<std::uint8_t> bitmap;
    bitmap.resize(hdr->w * hdr->h);
    std::int32_t y = 0, w = hdr->w, h = hdr->h;
    while (left && y < h) {
        auto size = std::uint32_t(*buf++);
        if (--left < size) {
            break;
        }
        auto *ptr = bitmap.data() + w * (y++);
        left -= size;
        while (size) {
            auto cnt = *buf++;
            --size;
            if (!size) {
                break;
            }
            memset(ptr, 0, cnt);
            ptr += cnt;
            cnt = *buf++;
            --size;
            if (size < cnt) {
                break;
            }
            memcpy(ptr, buf, cnt);
            buf += cnt;
            ptr += cnt;
            size -= cnt;
        }
    }
    auto *surface = SDL_CreateRGBSurfaceWithFormatFrom(bitmap.data(), w, h, 8, w, SDL_PIXELFORMAT_INDEX8);
    SDL_SetSurfacePalette(surface, static_cast<SDL_Palette*>(palette));
    data_ = SDL_CreateTextureFromSurface(static_cast<SDL_Renderer*>(renderer->renderer_), surface);
    SDL_FreeSurface(surface);
    SDL_SetTextureBlendMode(static_cast<SDL_Texture*>(data_), SDL_BLENDMODE_BLEND);
    width_ = hdr->w;
    height_ = hdr->h;
    originX_ = hdr->x;
    originY_ = hdr->y;
    return true;
}

void Texture::enableBlendMode(bool r) {
    SDL_SetTextureBlendMode(static_cast<SDL_Texture*>(data_), r ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
}

void TextureMgr::setPalette(const std::uint32_t *colors, std::size_t size) {
    auto *palette = SDL_AllocPalette(size);
    SDL_SetPaletteColors(palette, reinterpret_cast<const SDL_Color*>(colors), 0, size);
    palette_ = palette;
}

bool TextureMgr::loadFromRLE(std::int32_t id, const std::vector<std::uint8_t> &data) {
    Texture tex;
    if (!tex.loadFromRLE(renderer_, data, palette_)) {
        return false;
    }
    textures_.emplace(id, tex);
    return true;
}

const Texture &TextureMgr::operator[](std::int32_t id) const {
    auto ite = textures_.find(id);
    if (ite == textures_.end()) {
        static const Texture empty;
        return empty;
    }
    return ite->second;
}

}
