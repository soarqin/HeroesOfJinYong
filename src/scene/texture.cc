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

#include "logic/rle.hh"

#include "renderer.hh"
#include "colorpalette.hh"
#include "rectpacker.hh"
#include <SDL.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <memory>
#include <new>

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
    if (!renderer || w <= 0 || h <= 0) { return nullptr; }
#ifdef ALLOW_ODD_WIDTH
#else
    w = upToPowerOf2(w);
    h = upToPowerOf2(h);
#endif
    auto *tex = new(std::nothrow) Texture;
    if (!tex) { return nullptr; }
    auto *ren = static_cast<SDL_Renderer*>(renderer->renderer_);
    auto *texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, w, h);
    if (!texture) {
        delete tex;
        return nullptr;
    }
    tex->data_ = texture;
    tex->width_ = w;
    tex->height_ = h;
    return tex;
}

Texture *Texture::create(Renderer *renderer, std::int16_t w, std::int16_t h) {
    if (!renderer || w <= 0 || h <= 0) { return nullptr; }
    auto *tex = new(std::nothrow) Texture;
    if (!tex) { return nullptr; }
    auto *ren = static_cast<SDL_Renderer*>(renderer->renderer_);
    auto *texture = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
    if (!texture) {
        delete tex;
        return nullptr;
    }
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
    if (this == &other) { return *this; }
    if (data_) {
        SDL_DestroyTexture(static_cast<SDL_Texture *>(data_));
    }
    data_ = other.data_;
    width_ = other.width_;
    height_ = other.height_;
    originX_ = other.originX_;
    originY_ = other.originY_;
    other.data_ = nullptr;
    return *this;
}

bool Texture::enableBlendMode(bool r) {
    if (!data_) { return false; }
    return SDL_SetTextureBlendMode(
               static_cast<SDL_Texture*>(data_),
               r ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE) == 0;
}

void Texture::setBlendColor(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a) {
    if (!data_) { return; }
    auto *tex = static_cast<SDL_Texture*>(data_);
    SDL_SetTextureColorMod(tex, r, g, b);
    SDL_SetTextureAlphaMod(tex, a);
}

std::uint32_t *Texture::lock(int &pitch) {
    if (!data_) { return nullptr; }
    std::uint32_t *pixels;
    if (SDL_LockTexture(static_cast<SDL_Texture*>(data_), nullptr, reinterpret_cast<void**>(&pixels), &pitch)) {
        return nullptr;
    }
    pitch /= sizeof(std::uint32_t);
    return pixels;
}

std::uint32_t *Texture::lock(int &pitch, int x, int y, int w, int h) {
    if (!data_) { return nullptr; }
    std::uint32_t *pixels;
    SDL_Rect rc {x, y, w, h};
    if (SDL_LockTexture(static_cast<SDL_Texture*>(data_), &rc, reinterpret_cast<void**>(&pixels), &pitch)) {
        return nullptr;
    }
    pitch /= sizeof(std::uint32_t);
    return pixels;
}

void Texture::unlock() {
    if (data_) { SDL_UnlockTexture(static_cast<SDL_Texture*>(data_)); }
}

Texture *Texture::loadFromRLE(Renderer *renderer, const std::string &data, const ColorPalette &palette) {
    if (!renderer || !validateRLE(data)) { return nullptr; }
    std::int16_t arr[4];
    std::memcpy(arr, data.data(), sizeof(arr));
    auto w = arr[0], h = arr[1];
    auto *tex = Texture::create(renderer, w, h);
    if (!tex) { return nullptr; }
    if (!tex->enableBlendMode(true)) { delete tex; return nullptr; }
    int pitch;
    auto *pixels = tex->lock(pitch);
    if (!pixels || !Texture::renderRLE(data, palette.colors(), pixels, pitch, h, 0, 0, true)) {
        if (pixels) { tex->unlock(); }
        delete tex;
        return nullptr;
    }
    tex->unlock();
    tex->width_ = w;
    tex->height_ = h;
    tex->originX_ = arr[2];
    tex->originY_ = arr[3];
    return tex;
}

Texture *Texture::loadFromRAW(Renderer *renderer, const std::string &data, int width, int height, const ColorPalette &palette) {
    if (!renderer || width <= 0 || height <= 0
        || width > std::numeric_limits<std::int16_t>::max()
        || height > std::numeric_limits<std::int16_t>::max()
        || static_cast<std::size_t>(width) > std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(height)
        || data.size() < static_cast<std::size_t>(width) * static_cast<std::size_t>(height)) {
        return nullptr;
    }
    const auto *buf = reinterpret_cast<const uint8_t*>(data.data());
    auto *tex = Texture::create(renderer, width, height);
    if (!tex) { return nullptr; }
    const auto *colors = palette.colors();
    int pitch;
    auto *pixels = tex->lock(pitch);
    if (!pixels) { delete tex; return nullptr; }
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
    tex->unlock();
    tex->width_ = width;
    tex->height_ = height;
    tex->originX_ = 0;
    tex->originY_ = 0;
    return tex;
}

bool Texture::validateRLE(const std::string &data) {
    return logic::validateRleData(data);
}

bool Texture::renderRLE(const std::string &data, const std::uint32_t *colors, std::uint32_t *pixels, int pitch, int height, int ox, int oy, bool ignoreOrigin) {
    if (!colors || !pixels || pitch <= 0 || height <= 0 || !validateRLE(data)) { return false; }
    if (static_cast<std::size_t>(height) >
        std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(pitch)) {
        return false;
    }
    std::int16_t header[4];
    std::memcpy(header, data.data(), sizeof(header));
    const auto rows = static_cast<int>(header[1]);
    std::int64_t originX = ox;
    std::int64_t originY = oy;
    if (!ignoreOrigin) {
        originX -= header[2];
        originY -= header[3];
    }

    std::size_t position = sizeof(header);
    for (int row = 0; row < rows; ++row) {
        if (position >= data.size()) { return false; }
        const auto rowSize = static_cast<std::size_t>(
            static_cast<std::uint8_t>(data[position++]));
        if (rowSize > data.size() - position) { return false; }
        const auto rowEnd = position + rowSize;
        std::int64_t sourceX = 0;
        const auto destinationY = originY + row;
        while (position < rowEnd) {
            if (rowEnd - position < 2) { return false; }
            const auto skip = static_cast<std::int64_t>(
                static_cast<std::uint8_t>(data[position++]));
            const auto count = static_cast<std::size_t>(
                static_cast<std::uint8_t>(data[position++]));
            if (count > rowEnd - position) { return false; }
            sourceX += skip;
            for (std::size_t index = 0; index < count; ++index) {
                const auto destinationX = originX + sourceX
                    + static_cast<std::int64_t>(index);
                if (destinationY < 0 || destinationY >= height
                    || destinationX < 0 || destinationX >= pitch) {
                    continue;
                }
                const auto rowOffset = static_cast<std::size_t>(destinationY)
                    * static_cast<std::size_t>(pitch);
                pixels[rowOffset + static_cast<std::size_t>(destinationX)] =
                    colors[static_cast<std::uint8_t>(data[position + index])];
            }
            position += count;
            sourceX += static_cast<std::int64_t>(count);
        }
    }
    return true;
}

inline std::uint32_t blendAlpha(std::uint32_t p1, std::uint32_t p2) {
    static const std::uint32_t AMASK = 0xFF000000;
    static const std::uint32_t RBMASK = 0x00FF00FF;
    static const std::uint32_t GMASK = 0x0000FF00;
    std::uint32_t a = (p2 & AMASK) >> 24;
    std::uint32_t na = 255 - a;
    std::uint32_t rb = (na * (p1 & RBMASK)) + (a * (p2 & RBMASK));
    rb = (rb + 0x10001 + ((rb >> 8) & 0xFF00FF)) >> 8;
    std::uint32_t g = (na * (p1 & GMASK)) + (a * (p2 & GMASK));
    g = ((g + 1) * 257) >> 16;
    return (rb & RBMASK) | (g & GMASK) | 0xFF000000u;
}

bool Texture::renderRLEBlending(const std::string &data, const std::uint32_t *colors, std::uint32_t *pixels, int pitch, int height, int ox, int oy, bool ignoreOrigin) {
    if (!colors || !pixels || pitch <= 0 || height <= 0 || !validateRLE(data)) { return false; }
    if (static_cast<std::size_t>(height) >
        std::numeric_limits<std::size_t>::max() / static_cast<std::size_t>(pitch)) {
        return false;
    }
    std::int16_t header[4];
    std::memcpy(header, data.data(), sizeof(header));
    const auto rows = static_cast<int>(header[1]);
    std::int64_t originX = ox;
    std::int64_t originY = oy;
    if (!ignoreOrigin) {
        originX -= header[2];
        originY -= header[3];
    }

    std::size_t position = sizeof(header);
    for (int row = 0; row < rows; ++row) {
        if (position >= data.size()) { return false; }
        const auto rowSize = static_cast<std::size_t>(
            static_cast<std::uint8_t>(data[position++]));
        if (rowSize > data.size() - position) { return false; }
        const auto rowEnd = position + rowSize;
        std::int64_t sourceX = 0;
        const auto destinationY = originY + row;
        while (position < rowEnd) {
            if (rowEnd - position < 2) { return false; }
            const auto skip = static_cast<std::int64_t>(
                static_cast<std::uint8_t>(data[position++]));
            const auto count = static_cast<std::size_t>(
                static_cast<std::uint8_t>(data[position++]));
            if (count > rowEnd - position) { return false; }
            sourceX += skip;
            for (std::size_t index = 0; index < count; ++index) {
                const auto destinationX = originX + sourceX
                    + static_cast<std::int64_t>(index);
                if (destinationY < 0 || destinationY >= height
                    || destinationX < 0 || destinationX >= pitch) {
                    continue;
                }
                const auto rowOffset = static_cast<std::size_t>(destinationY)
                    * static_cast<std::size_t>(pitch);
                auto &pixel = pixels[rowOffset + static_cast<std::size_t>(destinationX)];
                pixel = blendAlpha(pixel,
                                   colors[static_cast<std::uint8_t>(data[position + index])]);
            }
            position += count;
            sourceX += static_cast<std::int64_t>(count);
        }
    }
    return true;
}

std::uint32_t Texture::calcRLEAvgColor(const std::string &data, const std::uint32_t *colors) {
    if (!colors || !validateRLE(data)) {
        return 0;
    }
    size_t left = data.size();
    const auto *buf = reinterpret_cast<const std::uint8_t*>(data.data());
    struct Header {
        std::int16_t w, h, x, y;
    };
    const auto *hdr = reinterpret_cast<const Header*>(buf);
    if (hdr->w == 0 && hdr->h == 0) {
        return 0;
    }
    buf += 8;
    left -= 8;
    std::uint32_t r = 0, g = 0, b = 0, pixcount = 0;
    std::int32_t y = 0, w = hdr->w, h = hdr->h;
    while (left && y < h) {
        auto size = std::uint32_t(*buf++);
        if (--left < size) {
            break;
        }
        left -= size;
        while (size) {
            auto cnt = *buf++;
            --size;
            if (!size) {
                break;
            }
            cnt = *buf++;
            --size;
            if (size < cnt) {
                break;
            }
            pixcount += cnt;
            size -= cnt;
            for (; cnt; --cnt) {
                const auto *c = reinterpret_cast<const std::uint8_t*>(&colors[*buf++]);
                r += c[2];
                g += c[1];
                b += c[0];
            }
        }
    }
    if (pixcount == 0) { return 0; }
    r /= pixcount;
    g /= pixcount;
    b /= pixcount;
    return b | (g << 8) | (r << 16);
}

TextureSlice::TextureSlice(Texture *tex, std::int16_t x, std::int16_t y, std::int16_t w, std::int16_t h, std::int16_t ox, std::int16_t oy):
    x_(x), y_(y) {
    data_ = tex->data();
    width_ = w;
    height_ = h;
    originX_ = ox;
    originY_ = oy;
}

TextureSlice::~TextureSlice() {
    data_ = nullptr;
}

TextureMgr::TextureMgr(): rectPacker_(new (std::nothrow) RectPacker(
    RectPackWidthDefault, RectPackWidthDefault)) {
}

TextureMgr::~TextureMgr() {
    clear();
}

void TextureMgr::setPalette(const ColorPalette &col) {
    palette_ = &col;
}

Texture *TextureMgr::loadFromRLE(const std::string &data, std::int16_t index) {
    if (!renderer_ || !palette_ || !rectPacker_ || index < 0
        || !Texture::validateRLE(data)) {
        return nullptr;
    }
    auto ite = textures_.find(index);
    if (ite != textures_.end()) {
        return ite->second;
    }
    std::int16_t arr[4];
    std::memcpy(arr, data.data(), sizeof(arr));
    auto w = arr[0], h = arr[1];
    std::int16_t x, y;
    auto rpidx = rectPacker_->pack(w, h, x, y);
    if (rpidx < 0) {
        return nullptr;
    }
    std::unique_ptr<Texture> containerCandidate;
    auto *tex = rpidx < static_cast<int>(textureContainers_.size())
        ? textureContainers_[rpidx] : nullptr;
    if (tex == nullptr) {
        containerCandidate.reset(Texture::create(renderer_, RectPackWidthDefault, RectPackWidthDefault));
        if (!containerCandidate || !containerCandidate->enableBlendMode(true)) {
            rectPacker_->rollbackLast();
            return nullptr;
        }
        tex = containerCandidate.get();
    }
    int pitch;
    TextureLock lock(tex, pitch, x, y, w, h);
    if (!lock.valid()
        || !Texture::renderRLE(data, palette_->colors(), lock.pixels(), pitch, h, 0, 0, true)) {
        rectPacker_->rollbackLast();
        return nullptr;
    }
    lock.unlock();

    if (containerCandidate) {
        try {
            if (rpidx >= static_cast<int>(textureContainers_.size())) {
                textureContainers_.resize(static_cast<std::size_t>(rpidx) + 1, nullptr);
            }
        } catch (...) {
            rectPacker_->rollbackLast();
            return nullptr;
        }
    }
    auto *texture = new(std::nothrow) TextureSlice(tex, x, y, w, h, arr[2], arr[3]);
    if (!texture) {
        rectPacker_->rollbackLast();
        return nullptr;
    }
    try {
        const auto inserted = textures_.emplace(index, texture);
        if (!inserted.second) {
            delete texture;
            rectPacker_->rollbackLast();
            return inserted.first->second;
        }
    } catch (...) {
        delete texture;
        rectPacker_->rollbackLast();
        return nullptr;
    }
    if (containerCandidate) {
        textureContainers_[rpidx] = containerCandidate.release();
    }
    textureIdMax_ = std::max<std::int32_t>(index, textureIdMax_);
    return texture;
}

void TextureMgr::loadFromRLE(const std::vector<std::string> &data) {
    int sz = int(data.size());
    for (int i = 0; i < sz; ++i) {
        loadFromRLE(data[i], i);
    }
}

Texture *TextureMgr::loadFromRAW(const std::string &data, int width, int height, std::int16_t index) {
    if (index < 0 || !renderer_ || !palette_ || textures_.find(index) != textures_.end()) {
        return nullptr;
    }
    auto *tex = Texture::loadFromRAW(renderer_, data, width, height, *palette_);
    if (!tex) {
        return nullptr;
    }
    try {
        const auto inserted = textures_.emplace(index, tex);
        if (!inserted.second) {
            delete tex;
            return nullptr;
        }
    } catch (...) {
        delete tex;
        return nullptr;
    }
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

void TextureMgr::swap(TextureMgr &other) noexcept {
    using std::swap;
    swap(textures_, other.textures_);
    swap(textureContainers_, other.textureContainers_);
    swap(rectPacker_, other.rectPacker_);
    swap(textureIdMax_, other.textureIdMax_);
    swap(renderer_, other.renderer_);
    swap(palette_, other.palette_);
}

void TextureMgr::clear() {
    for (auto &p: textures_) {
        delete p.second;
    }
    textures_.clear();
    for (auto *p: textureContainers_) {
        delete p;
    }
    textureContainers_.clear();
    std::unique_ptr<RectPacker> replacement(new (std::nothrow) RectPacker(
        RectPackWidthDefault, RectPackWidthDefault));
    rectPacker_.swap(replacement);
    textureIdMax_ = 0;
}

}
