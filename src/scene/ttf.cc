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

#include "rectpacker.hh"
#include "renderer.hh"
#include "texture.hh"
#include "util/file.hh"

#ifdef USE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#else
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <external/stb_truetype.h>
#endif
#include <SDL.h>

namespace hojy::scene {

TTF::TTF(Renderer *renderer): renderer_(renderer), rectpacker_(new RectPacker) {
#ifdef USE_FREETYPE
    FT_Init_FreeType(&ftLib_);
#endif
}

TTF::~TTF() {
    deinit();
#ifdef USE_FREETYPE
    FT_Done_FreeType(ftLib_);
#endif
}

void TTF::init(int size, std::uint8_t width) {
    fontSize_ = size;
    monoWidth_ = width;
}

void TTF::deinit() {
    for (auto &tex: textures_) {
        delete tex;
    }
    textures_.clear();
    fontCache_.clear();
    for (auto &p: fonts_) {
#ifdef USE_FREETYPE
        FT_Done_Face(p.face);
#else
        delete static_cast<stbtt_fontinfo *>(p.font);
        p.ttf_buffer.clear();
#endif
    }
    fonts_.clear();
}

bool TTF::add(const std::string &filename, int index) {
    FontInfo fi;
#ifdef USE_FREETYPE
    if (FT_New_Face(ftLib_, filename.c_str(), index, &fi.face)) return false;
    fonts_.emplace_back(fi);
#else
    if (!util::File::getFileContent(filename, fi.ttf_buffer)) {
        return false;
    }
    auto *info = new stbtt_fontinfo;
    stbtt_InitFont(info, &fi.ttf_buffer[0], stbtt_GetFontOffsetForIndex(&fi.ttf_buffer[0], index));
    fi.font = info;
    fonts_.emplace_back(std::move(fi));
#endif
    return true;
}

void TTF::charDimension(std::uint32_t ch, std::uint8_t &width, std::int8_t &t, std::int8_t &b, int fontSize) {
    if (fontSize < 0) fontSize = fontSize_;
    const FontData *fd;
    std::uint64_t key = (std::uint64_t(fontSize) << 32) | std::uint64_t(ch);
    auto ite = fontCache_.find(key);
    if (ite == fontCache_.end()) {
        fd = makeCache(ch, fontSize);
        if (!fd) {
            width = t = b = 0;
            return;
        }
    } else {
        fd = &ite->second;
        if (fd->advW == 0) {
            width = t = b = 0;
            return;
        }
    }
    if (monoWidth_)
        width = std::max(fd->advW, monoWidth_);
    else
        width = fd->advW;
    t = fd->iy0;
    b = fd->iy0 + fd->h;
}

int TTF::stringWidth(const std::wstring &str, int fontSize) {
    std::uint8_t w;
    std::int8_t t, b;
    int res = 0;
    for (auto &ch: str) {
        if (ch < 32) { continue; }
        charDimension(ch, w, t, b, fontSize);
        res += int(std::uint32_t(w));
    }
    return res;
}

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
    if (fontSize < 0) fontSize = fontSize_;
    int colorIndex = 0;
    for (auto ch: str) {
        if (ch > 0 && ch < 17) { colorIndex = ch - 1; continue; }
        const FontData *fd;
        std::uint64_t key = (std::uint64_t(fontSize) << 32) | std::uint64_t(ch);
        auto ite = fontCache_.find(key);
        if (ite == fontCache_.end()) {
            fd = makeCache(ch, fontSize);
            if (!fd) {
                continue;
            }
        } else {
            fd = &ite->second;
            if (fd->advW == 0) continue;
        }
        if (shadow) {
            auto *tex = textures_[fd->rpidx];
            tex->setBlendColor(0, 0, 0, 255);
            renderer_->renderTexture(tex, x + fd->ix0 + 2, y + fd->iy0 + 2, fd->rpx, fd->rpy, fd->w, fd->h, true);
            tex->setBlendColor(altR_[colorIndex], altG_[colorIndex], altB_[colorIndex], 255);
            renderer_->renderTexture(tex, x + fd->ix0, y + fd->iy0, fd->rpx, fd->rpy, fd->w, fd->h, true);
        } else {
            auto *tex = textures_[fd->rpidx];
            tex->setBlendColor(altR_[colorIndex], altG_[colorIndex], altB_[colorIndex], 255);
            renderer_->renderTexture(tex, x + fd->ix0, y + fd->iy0, fd->rpx, fd->rpy, fd->w, fd->h, true);
        }
        x += fd->advW;
    }
}

const TTF::FontData *TTF::makeCache(std::uint32_t ch, int fontSize) {
    if (fontSize < 0) fontSize = fontSize_;
    FontInfo *fi = nullptr;
#ifndef USE_FREETYPE
    stbtt_fontinfo *info;
    std::uint32_t index = 0;
#endif
    for (auto &f: fonts_) {
#ifdef USE_FREETYPE
        auto index = FT_Get_Char_Index(f.face, ch);
        if (index == 0) continue;
        FT_Set_Pixel_Sizes(f.face, 0, fontSize);
        auto err = FT_Load_Glyph(f.face, index, FT_LOAD_DEFAULT);
        if (!err) { fi = &f; break; }
#else
        info = static_cast<stbtt_fontinfo*>(f.font);
        index = stbtt_FindGlyphIndex(info, ch);
        if (index != 0) { fi = &f; break; }
#endif
    }
    std::uint64_t key = (std::uint64_t(fontSize) << 32) | std::uint64_t(ch);
    FontData *fd = &fontCache_[key];
    if (fi == nullptr) {
        memset(fd, 0, sizeof(FontData));
        return nullptr;
    }

#ifdef USE_FREETYPE
    unsigned char *srcPtr;
    int bitmapPitch;
    if (FT_Render_Glyph(fi->face->glyph, FT_RENDER_MODE_NORMAL)) return nullptr;
    FT_GlyphSlot slot = fi->face->glyph;
    fd->ix0 = slot->bitmap_left;
    fd->iy0 = fontSize * 7 / 8 - slot->bitmap_top;
    fd->w = slot->bitmap.width;
    fd->h = slot->bitmap.rows;
    fd->advW = slot->advance.x >> 6;
    srcPtr = slot->bitmap.buffer;
    bitmapPitch = slot->bitmap.pitch;
#else
    /* Read font data to cache */
    int advW, leftB;
    float fontScale = stbtt_ScaleForMappingEmToPixels(info, static_cast<float>(fontSize));
    stbtt_GetGlyphHMetrics(info, index, &advW, &leftB);
    fd->advW = std::uint8_t(std::lround(fontScale * float(advW)));
    int ix0, iy0, ix1, iy1;
    stbtt_GetGlyphBitmapBoxSubpixel(info, index, fontScale, fontScale, 0, 0, &ix0, &iy0, &ix1, &iy1);
    fd->ix0 = ix0;
    fd->iy0 = fontSize * 7 / 8 + iy0;
    fd->w = ix1 - ix0;
    fd->h = iy1 - iy0;
#endif

    int dstPitch = int((fd->w + 1u) & ~1u);
    /* Get last rect pack bitmap */
    auto rpidx = rectpacker_->pack(dstPitch, fd->h, fd->rpx, fd->rpy);
    if (rpidx < 0) {
        memset(fd, 0, sizeof(FontData));
        return nullptr;
    }
    // stbrp_rect rc = {0, std::uint16_t((fd->w + 3u) & ~3u), fd->h};
    fd->rpidx = rpidx;

    std::uint8_t dst[64 * 64];

#ifdef USE_FREETYPE
    auto *dstPtr = dst;
    for (int k = 0; k < fd->h; ++k) {
        memcpy(dstPtr, srcPtr, fd->w);
        srcPtr += bitmapPitch;
        dstPtr += dstPitch;
    }
#else
    stbtt_MakeGlyphBitmapSubpixel(info, dst, fd->w, fd->h, dstPitch, fontScale, fontScale, 3, 3, index);
#endif

    if (rpidx >= textures_.size()) {
        textures_.resize(rpidx + 1, nullptr);
    }
    auto *tex = textures_[rpidx];
    if (tex == nullptr) {
        tex = Texture::create(renderer_, RectPackWidth, RectPackWidth);
        tex->enableBlendMode(true);
        textures_[rpidx] = tex;
    }
    int pitch;
    uint32_t *pixels = tex->lock(pitch);
    if (pixels) {
        dstPtr = dst;
        pixels += fd->rpy * pitch + fd->rpx;
        int offset = pitch - dstPitch;
        int h = fd->h;
        while (h--) {
            int w = dstPitch;
            while (w--) {
                *pixels++ = 0xFFFFFFu | (std::uint32_t(*dstPtr++) << 24);
            }
            pixels += offset;
        }
        tex->unlock();
    }
    return fd;
}

}
