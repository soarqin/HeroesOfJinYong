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
    auto *texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    tex->data_ = texture;
    tex->width_ = w;
    tex->height_ = h;
    return tex;
}

Texture *Texture::create(Renderer *renderer, std::int16_t w, std::int16_t h) {
    auto *tex = new Texture;
    auto *ren = static_cast<SDL_Renderer*>(renderer->renderer_);
    auto *texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
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
    if (data.empty()) { return nullptr; }
    const auto *arr = reinterpret_cast<const uint16_t*>(data.data());
    auto w = arr[0], h = arr[1];
    auto *tex = Texture::create(renderer, w, h);
    if (!tex) { return nullptr; }
    tex->enableBlendMode(true);
    std::uint32_t *pixels;
    int pitch;
    SDL_LockTexture(static_cast<SDL_Texture*>(tex->data()), nullptr, reinterpret_cast<void**>(&pixels), &pitch);
    pitch /= sizeof(std::uint32_t);
    Texture::renderRLE(data, palette.colors(), pixels, pitch, h, 0, 0, true);
    SDL_UnlockTexture(static_cast<SDL_Texture*>(tex->data()));
    tex->width_ = w;
    tex->height_ = h;
    tex->originX_ = arr[2];
    tex->originY_ = arr[3];
    return tex;
}

Texture *Texture::loadFromRAW(Renderer *renderer, const std::string &data, int width, int height, const ColorPalette &palette) {
    if (data.empty()) { return nullptr; }
    const auto *buf = reinterpret_cast<const uint8_t*>(data.data());
    auto *tex = Texture::create(renderer, width, height);
    if (!tex) { return nullptr; }
    const auto *colors = palette.colors();
    std::uint32_t *pixels;
    int pitch;
    SDL_LockTexture(static_cast<SDL_Texture*>(tex->data()), nullptr, reinterpret_cast<void**>(&pixels), &pitch);
    pitch /= sizeof(std::uint32_t);
    int h = height;
    while (h--) {
        int w = width;
        auto *ptr = pixels;
        while (w--) {
            auto c = *buf++;
            *ptr++ = c ? colors[c] : 0xFF000000U;
        }
        pixels += pitch;
    }
    SDL_UnlockTexture(static_cast<SDL_Texture*>(tex->data()));
    tex->width_ = width;
    tex->height_ = height;
    tex->originX_ = 0;
    tex->originY_ = 0;
    return tex;
}

void Texture::renderRLE(const std::string &data, const std::uint32_t *colors, std::uint32_t *pixels, int pitch, int height, int ox, int oy, bool ignoreOrigin) {
    size_t left = data.size();
    if (left < 8) {
        return;
    }
    const auto *obuf = reinterpret_cast<const std::uint8_t*>(data.data());
    struct Header {
        std::int16_t w, h, x, y;
    };
    const auto *hdr = reinterpret_cast<const Header*>(obuf);
    obuf += 8;
    left -= 8;
    if (!ignoreOrigin) {
        ox -= hdr->x;
        oy -= hdr->y;
    }
    std::int32_t w = hdr->w, h = hdr->h;
    if (ox + w <= 0 || oy + h <= 0) { return; }
    while (left && h--) {
        auto size = std::uint32_t(*obuf++);
        if (--left < size) {
            break;
        }
        const auto *buf = obuf;
        left -= size;
        obuf += size;
        if (oy < 0) { ++oy; continue; }
        if (oy >= height) { break; }
        auto *ptr = pixels + ox + pitch * (oy++);
        int x = ox;
        while (size) {
            auto cnt = *buf++;
            --size;
            if (!size) {
                break;
            }
            ptr += cnt;
            x += cnt;
            cnt = *buf++;
            --size;
            if (size < cnt) {
                break;
            }
            if (x < 0) {
                if (x + cnt <= 0) {
                    ptr += cnt;
                    buf += cnt;
                } else {
                    ptr -= x;
                    buf -= x;
                    for (int z = x + cnt; z; --z) {
                        *ptr++ = colors[*buf++];
                    }
                }
            } else if (x + cnt > pitch) {
                if (x >= pitch) {
                    ptr += cnt;
                    buf += cnt;
                } else {
                    for (int z = pitch - x; z; --z) {
                        *ptr++ = colors[*buf++];
                    }
                    int offset = x + cnt - pitch;
                    ptr += offset;
                    buf += offset;
                }
            } else {
                for (int z = cnt; z; --z) {
                    *ptr++ = colors[*buf++];
                }
            }
            x += cnt;
            size -= cnt;
        }
    }
}

TextureMgr::~TextureMgr() {
    for (auto &p: textures_) {
        delete p.second;
    }
    textures_.clear();
}

void TextureMgr::setPalette(const ColorPalette &col) {
    palette_ = &col;
}

Texture *TextureMgr::loadFromRLE(const std::string &data, std::int16_t index) {
    if (textures_.find(index) != textures_.end()) {
        return nullptr;
    }
    auto *tex = Texture::loadFromRLE(renderer_, data, *palette_);
    if (!tex) {
        return nullptr;
    }
    textures_[index] = tex;
    textureIdMax_ = std::max<std::int32_t>(index, textureIdMax_);
    return tex;
}

void TextureMgr::loadFromRLE(const std::vector<std::string> &data) {
    int sz = int(data.size());
    for (int i = 0; i < sz; ++i) {
        loadFromRLE(data[i], i);
    }
}

Texture *TextureMgr::loadFromRAW(const std::string &data, int width, int height, std::int16_t index) {
    if (textures_.find(index) != textures_.end()) {
        return nullptr;
    }
    auto *tex = Texture::loadFromRAW(renderer_, data, width, height, *palette_);
    if (!tex) {
        return nullptr;
    }
    textures_[index] = tex;
    textureIdMax_ = std::max<std::int32_t>(index, textureIdMax_);
    return tex;
}

void TextureMgr::loadFromRAW(const std::vector<std::string> &data, int width, int height) {
    int sz = int(data.size());
    for (int i = 0; i < sz; ++i) {
        loadFromRAW(data[i], width, height, i);
    }
}

const Texture *TextureMgr::operator[](std::int32_t id) const {
    auto ite = textures_.find(id);
    if (ite == textures_.end()) { return nullptr; }
    return ite->second;
}

const Texture *TextureMgr::last() const {
    return (*this)[textureIdMax_];
}

}
