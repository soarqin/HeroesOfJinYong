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

#include "renderer.hh"
#include "colorpalette.hh"

#include <SDL.h>

namespace hojy::scene {

int upToPowerOf2(int n) {
    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    return n + 1;
}

Texture *Texture::createAsTarget(Renderer *renderer, int w, int h) {
#ifdef ALLOW_ODD_WIDTH
#else
    w = upToPowerOf2(w);
    h = upToPowerOf2(h);
#endif
    auto *tex = new Texture;
    auto *ren = static_cast<SDL_Renderer*>(renderer->renderer_);
    auto *texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_TARGET, w, h);
    tex->data_ = texture;
    tex->width_ = w;
    tex->height_ = h;
    return tex;
}

Texture *Texture::create(Renderer *renderer, std::int16_t w, std::int16_t h) {
    auto *tex = new Texture;
    auto *ren = static_cast<SDL_Renderer*>(renderer->renderer_);
    auto *texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    tex->data_ = texture;
    tex->width_ = w;
    tex->height_ = h;
    return tex;
}

Texture::~Texture() {
    if (data_) {
        SDL_DestroyTexture(static_cast<SDL_Texture *>(data_));
    }
}

Texture::Texture(Texture &&other) noexcept: data_(other.data_), width_(other.width_), height_(other.height_), originX_(other.originX_), originY_(other.originY_) {
    other.data_ = nullptr;
}

Texture &Texture::operator=(Texture &&other) noexcept {
    data_ = other.data_;
    width_ = other.width_;
    height_ = other.height_;
    originX_ = other.originX_;
    originY_ = other.originY_;
    other.data_ = nullptr;
    return *this;
}

void Texture::enableBlendMode(bool r) {
    SDL_SetTextureBlendMode(static_cast<SDL_Texture*>(data_), r ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
}

void Texture::setBlendColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    auto *tex = static_cast<SDL_Texture*>(data_);
    SDL_SetTextureColorMod(tex, r, g, b);
    SDL_SetTextureAlphaMod(tex, a);
}

Texture *Texture::loadFromRLE(Renderer *renderer, const std::string &data, const ColorPalette &palette) {
    size_t left = data.size();
    if (left < 8) {
        return nullptr;
    }
    const auto *obuf = reinterpret_cast<const std::uint8_t*>(data.data());
    struct Header {
        std::int16_t w, h, x, y;
    };
    const auto *hdr = reinterpret_cast<const Header*>(obuf);
    if (hdr->w == 0 || hdr->h == 0) {
        return nullptr;
    }
    obuf += 8;
    left -= 8;
    std::vector<std::uint8_t> bitmap;
    bitmap.resize(hdr->w * hdr->h);
    std::int32_t y = 0, w = hdr->w, h = hdr->h;
    while (left && y < h) {
        auto size = std::uint32_t(*obuf++);
        if (--left < size) {
            break;
        }
        const auto *buf = obuf;
        left -= size;
        obuf += size;
        auto *ptr = bitmap.data() + w * (y++);
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
    if (!surface) {
        return nullptr;
    }
    SDL_SetSurfacePalette(surface, static_cast<SDL_Palette*>(palette.obj()));
    auto *tex = SDL_CreateTextureFromSurface(static_cast<SDL_Renderer*>(renderer->renderer_), surface);
    SDL_FreeSurface(surface);
    if (!tex) {
        return nullptr;
    }
    auto *texture = new(std::nothrow) Texture;
    if (!texture) {
        SDL_DestroyTexture(tex);
        return nullptr;
    }
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    texture->data_ = tex;
    texture->width_ = hdr->w;
    texture->height_ = hdr->h;
    texture->originX_ = hdr->x;
    texture->originY_ = hdr->y;
    return texture;
}

Texture *Texture::loadFromRAW(Renderer *renderer, const std::string &data, int width, int height, const ColorPalette &palette) {
    auto *surface = SDL_CreateRGBSurfaceWithFormatFrom(const_cast<char*>(data.data()), width, height, 8, width, SDL_PIXELFORMAT_INDEX8);
    if (!surface) {
        return nullptr;
    }
    SDL_SetSurfacePalette(surface, static_cast<SDL_Palette*>(palette.obj()));
    auto *tex = SDL_CreateTextureFromSurface(static_cast<SDL_Renderer*>(renderer->renderer_), surface);
    SDL_FreeSurface(surface);
    if (!tex) {
        return nullptr;
    }
    auto *texture = new(std::nothrow) Texture;
    if (!texture) {
        SDL_DestroyTexture(tex);
        return nullptr;
    }
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    texture->data_ = tex;
    texture->width_ = width;
    texture->height_ = height;
    texture->originX_ = 0;
    texture->originY_ = 0;
    return texture;
}

void TextureMgr::setPalette(const ColorPalette &col) {
    palette_ = &col;
}

bool TextureMgr::loadFromRLE(const std::vector<std::string> &data) {
    auto sz = data.size();
    for (size_t i = 0; i < sz; ++i) {
        auto *tex = Texture::loadFromRLE(renderer_, data[i], *palette_);
        if (!tex) {
            continue;
        }
        textures_[i].reset(tex);
        textureIdMax_ = std::max<std::int32_t>(i, textureIdMax_);
    }
    return true;
}

bool TextureMgr::mergeFromRLE(const std::vector<std::string> &data) {
    auto sz = data.size();
    for (size_t i = 0; i < sz; ++i) {
        if (textures_.find(i) != textures_.end()) {
            continue;
        }
        auto *tex = Texture::loadFromRLE(renderer_, data[i], *palette_);
        if (!tex) {
            continue;
        }
        textures_[i].reset(tex);
        textureIdMax_ = std::max<std::int32_t>(i, textureIdMax_);
    }
    return true;
}

bool TextureMgr::loadFromRAW(const std::vector<std::string> &data, int width, int height) {
    auto sz = data.size();
    for (size_t i = 0; i < sz; ++i) {
        auto *tex = Texture::loadFromRAW(renderer_, data[i], width, height, *palette_);
        if (!tex) {
            continue;
        }
        textures_[i].reset(tex);
        textureIdMax_ = std::max<std::int32_t>(i, textureIdMax_);
    }
    return true;
}

const Texture *TextureMgr::operator[](std::int32_t id) const {
    auto ite = textures_.find(id);
    if (ite == textures_.end()) { return nullptr; }
    return ite->second.get();
}

const Texture *TextureMgr::last() const {
    return (*this)[textureIdMax_];
}

}
