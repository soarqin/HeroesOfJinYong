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

#include "core/config.hh"
#include "util/file.hh"

#define STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#include <external/stb_rect_pack.h>
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

enum :std::uint16_t {
    TTF_RECTPACK_WIDTH = 1024
};

struct rect_pack_data {
    stbrp_context context;
    stbrp_node nodes[TTF_RECTPACK_WIDTH];
};

TTF::TTF(void *renderer): renderer_(renderer) {
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
        SDL_DestroyTexture(static_cast<SDL_Texture*>(tex));
    }
    textures_.clear();
    fontCache_.clear();
    for (auto *&p: rectpackData_) {
        delete p;
    }
    rectpackData_.clear();
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
    FT_Set_Pixel_Sizes(fi.face, 0, font_size);
    fonts.emplace_back(fi);
#else
    if (!util::File::getFileContent(core::config.dataFilePath(filename), fi.ttf_buffer)) {
        return false;
    }
    auto *info = new stbtt_fontinfo;
    stbtt_InitFont(info, &fi.ttf_buffer[0], stbtt_GetFontOffsetForIndex(&fi.ttf_buffer[0], index));
    fi.font_scale = stbtt_ScaleForMappingEmToPixels(info, static_cast<float>(fontSize_));
    fi.font = info;
    fonts_.emplace_back(std::move(fi));
#endif
    newRectPack();
    return true;
}

void TTF::charDimension(std::uint16_t ch, std::uint8_t &width, std::int8_t &t, std::int8_t &b) {
    const FontData *fd;
    auto ite = fontCache_.find(ch);
    if (ite == fontCache_.end()) {
        fd = makeCache(ch);
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

void TTF::setColor(std::uint8_t r, std::uint8_t g, std::uint8_t b) {
    r_ = r; g_ = g; b_ = b;
}

void TTF::render(std::wstring_view str, int x, int y, int maxw) {
    auto *renderer = static_cast<SDL_Renderer*>(renderer_);
    for (auto ch: str) {
        const FontData *fd;
        auto ite = fontCache_.find(ch);
        if (ite == fontCache_.end()) {
            fd = makeCache(ch);
            if (!fd) {
                continue;
            }
        } else {
            fd = &ite->second;
            if (fd->advW == 0) continue;
        }
        SDL_Rect srcrc = {fd->rpx, fd->rpy, fd->w, fd->h};
        SDL_Rect dstrc = {x + fd->ix0 + 2, y + fd->iy0 + 2, fd->w, fd->h};
        auto *tex = static_cast<SDL_Texture*>(textures_[fd->rpidx]);
        SDL_SetTextureColorMod(tex, 0, 0, 0);
        SDL_RenderCopy(renderer, tex, &srcrc, &dstrc);
        dstrc.x -= 2; dstrc.y -= 2;
        SDL_SetTextureColorMod(tex, r_, g_, b_);
        SDL_RenderCopy(renderer, tex, &srcrc, &dstrc);
        x += fd->advW;
    }
}

void TTF::newRectPack() {
    auto *rpd = new rect_pack_data;
    stbrp_init_target(&rpd->context, TTF_RECTPACK_WIDTH, TTF_RECTPACK_WIDTH, rpd->nodes, TTF_RECTPACK_WIDTH);
    rectpackData_.push_back(rpd);
}

const TTF::FontData *TTF::makeCache(std::uint16_t ch) {
    FontInfo *fi = nullptr;
#ifndef USE_FREETYPE
    stbtt_fontinfo *info;
    uint32_t index = 0;
#endif
    for (auto &f: fonts_) {
        fi = &f;
#ifdef USE_FREETYPE
        auto index = FT_Get_Char_Index(f.face, ch);
        if (index == 0) continue;
        if (!FT_Load_Glyph(f.face, index, FT_LOAD_DEFAULT)) break;
#else
        info = static_cast<stbtt_fontinfo*>(f.font);
        index = stbtt_FindGlyphIndex(info, ch);
        if (index != 0) break;
#endif
    }
    FontData *fd = &fontCache_[ch];
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
    fd->iy0 = -slot->bitmap_top;
    fd->w = slot->bitmap.width;
    fd->h = slot->bitmap.rows;
    fd->advW = slot->advance.x >> 6;
    srcPtr = slot->bitmap.buffer;
    bitmapPitch = slot->bitmap.pitch;
#else
    /* Read font data to cache */
    int advW, leftB;
    stbtt_GetGlyphHMetrics(info, index, &advW, &leftB);
    fd->advW = static_cast<std::uint8_t>(fi->font_scale * advW + 0.5f);
    int ix0, iy0, ix1, iy1;
    stbtt_GetGlyphBitmapBoxSubpixel(info, index, fi->font_scale, fi->font_scale, 0, 0, &ix0, &iy0, &ix1, &iy1);
    fd->ix0 = ix0;
    fd->iy0 = fontSize_ + iy0;
    fd->w = ix1 - ix0;
    fd->h = iy1 - iy0;
#endif

    /* Get last rect pack bitmap */
    auto rpidx = rectpackData_.size() - 1;
    auto *rpd = rectpackData_[rpidx];
    stbrp_rect rc = {0, std::uint16_t((fd->w + 3u) & ~3u), fd->h};
    if (!stbrp_pack_rects(&rpd->context, &rc, 1)) {
        /* No space to hold the bitmap,
         * create a new bitmap */
        newRectPack();
        rpidx = rectpackData_.size() - 1;
        rpd = rectpackData_[rpidx];
        stbrp_pack_rects(&rpd->context, &rc, 1);
    }
    /* Do rect pack */
    fd->rpx = rc.x;
    fd->rpy = rc.y;
    fd->rpidx = rpidx;

    std::uint8_t dst[64 * 64];
    int dstPitch = rc.w;

#ifdef USE_FREETYPE
    auto *dstPtr = dst;
    for (int k = 0; k < fd->h; ++k) {
        memcpy(dstPtr, srcPtr, fd->w);
        srcPtr += bitmapPitch;
        dstPtr += dstPitch;
    }
#else
    stbtt_MakeGlyphBitmapSubpixel(info, dst, fd->w, fd->h, dstPitch, fi->font_scale, fi->font_scale, 3, 3, index);
#endif

    if (rpidx >= textures_.size()) {
        textures_.resize(rpidx + 1, nullptr);
    }
    auto *tex = static_cast<SDL_Texture*>(textures_[rpidx]);
    if (tex == nullptr) {
        tex = SDL_CreateTexture(static_cast<SDL_Renderer*>(renderer_),
                                SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                TTF_RECTPACK_WIDTH, TTF_RECTPACK_WIDTH);
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        textures_[rpidx] = tex;
    }
    std::uint32_t pixels[64 * 64];
    auto sz = dstPitch * fd->h;
    for (size_t i = 0; i < sz; ++i) {
        pixels[i] = 0xFFFFFFu | (std::uint32_t(dst[i]) << 24);
    }
    SDL_Rect updaterc{rc.x, rc.y, rc.w, rc.h};
    SDL_UpdateTexture(tex, &updaterc, pixels, dstPitch * 4);
    return fd;
}

}
