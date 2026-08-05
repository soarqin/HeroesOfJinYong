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
#include "logic/font_metrics.hh"
#include "util/file.hh"

#ifdef USE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#else
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include <external/stb_truetype.h>
#endif

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <limits>
#include <memory>
#include <new>

#ifndef USE_FREETYPE
namespace {

std::uint32_t readBigEndian32(const std::uint8_t *data) {
    return (std::uint32_t(data[0]) << 24)
        | (std::uint32_t(data[1]) << 16)
        | (std::uint32_t(data[2]) << 8)
        | std::uint32_t(data[3]);
}

bool getStbFontOffset(const std::vector<std::uint8_t> &buffer, int index, int &offset) {
    if (index < 0 || buffer.size() < 4) { return false; }

    const auto *data = buffer.data();
    if (buffer.size() >= 4 && std::memcmp(data, "ttcf", 4) == 0) {
        if (buffer.size() < 12) { return false; }
        const auto version = readBigEndian32(data + 4);
        if (version != 0x00010000u && version != 0x00020000u) { return false; }
        const auto count = readBigEndian32(data + 8);
        if (count == 0 || static_cast<std::size_t>(count) > (buffer.size() - 12) / 4
            || static_cast<std::size_t>(index) >= count) {
            return false;
        }
        const auto tableOffset = 12 + static_cast<std::size_t>(index) * 4;
        const auto fontOffset = readBigEndian32(data + tableOffset);
        if (fontOffset > static_cast<std::uint32_t>(std::numeric_limits<int>::max())
            || static_cast<std::size_t>(fontOffset) >= buffer.size()) {
            return false;
        }
        offset = static_cast<int>(fontOffset);
        return buffer.size() - static_cast<std::size_t>(offset) >= 12;
    }

    offset = stbtt_GetFontOffsetForIndex(data, index);
    return offset >= 0 && static_cast<std::size_t>(offset) < buffer.size()
        && buffer.size() - static_cast<std::size_t>(offset) >= 12;
}

}
#endif

namespace hojy::scene {

std::size_t TTF::KerningKeyHash::operator()(const KerningKey &key) const noexcept {
    auto seed = std::hash<int>{}(key.fontSize);
    seed ^= std::hash<std::uint32_t>{}(key.left)
        + 0x9e3779b9u + (seed << 6) + (seed >> 2);
    seed ^= std::hash<std::uint32_t>{}(key.right)
        + 0x9e3779b9u + (seed << 6) + (seed >> 2);
    return seed;
}

TTF::TTF(Renderer *renderer): renderer_(renderer), rectpacker_(new RectPacker(RectPackWidthDefault, RectPackWidthDefault)) {
    ready_ = renderer_ != nullptr && rectpacker_ != nullptr;
#ifdef USE_FREETYPE
    if (ready_ && FT_Init_FreeType(&ftLib_) != 0) {
        ready_ = false;
    }
#endif
}

TTF::~TTF() {
    deinit();
#ifdef USE_FREETYPE
    if (ftLib_) { FT_Done_FreeType(ftLib_); }
#endif
}

bool TTF::init(int size, std::uint8_t width) {
    if (!ready_ || size <= 0) {
        initialized_ = false;
        return false;
    }
    fontSize_ = size;
    monoWidth_ = width;
    initialized_ = true;
    return true;
}

void TTF::deinit() {
    initialized_ = false;
    for (auto &tex: textures_) {
        delete tex;
    }
    textures_.clear();
    fontCache_.clear();
    kerningCache_.clear();
    for (auto &p: fonts_) {
#ifdef USE_FREETYPE
        FT_Done_Face(p.face);
#else
        delete static_cast<stbtt_fontinfo *>(p.font);
        p.ttf_buffer.clear();
#endif
    }
    fonts_.clear();
    // Glyph coordinates belong to the atlas skyline.  Rebuild the packer
    // together with the textures so a later init cannot reuse stale space.
    rectpacker_.reset(new RectPacker(RectPackWidthDefault, RectPackWidthDefault));
}

bool TTF::add(const std::string &filename, int index) {
    if (!ready_ || !initialized_ || filename.empty() || index < 0) {
        return false;
    }
    FontInfo fi;
#ifdef USE_FREETYPE
    if (FT_New_Face(ftLib_, filename.c_str(), index, &fi.face)) return false;
    try {
        fonts_.emplace_back(fi);
    } catch (...) {
        FT_Done_Face(fi.face);
        throw;
    }
#else
    if (!util::File::getFileContent(filename, fi.ttf_buffer)) {
        return false;
    }
    int offset = 0;
    if (!getStbFontOffset(fi.ttf_buffer, index, offset)) {
        return false;
    }
    std::unique_ptr<stbtt_fontinfo> info(new(std::nothrow) stbtt_fontinfo);
    if (!info || !stbtt_InitFont(info.get(), fi.ttf_buffer.data(), offset)) {
        return false;
    }
    fi.font = info.get();
    fonts_.emplace_back(std::move(fi));
    info.release();
#endif
    return true;
}

void TTF::charDimension(std::uint32_t ch, std::uint8_t &width, std::int8_t &t, std::int8_t &b, int fontSize) {
    if (fontSize < 0) fontSize = fontSize_;
    const auto key = (std::uint64_t(fontSize) << 32) | std::uint64_t(ch);
    const auto ite = fontCache_.find(key);
    if (ite == fontCache_.end() || ite->second.advW == 0) {
        width = t = b = 0;
        return;
    }
    const auto *fd = &ite->second;
    if (monoWidth_)
        width = std::max(fd->advW, monoWidth_);
    else
        width = fd->advW;
    t = fd->iy0;
    b = fd->iy0 + fd->h;
}

int TTF::stringWidth(const std::wstring &str, int fontSize) {
    return preparedStringWidth(str, fontSize);
}

bool TTF::preparedCharDimension(std::uint32_t ch, std::uint8_t &width,
                                std::int8_t &t, std::int8_t &b, int fontSize) const {
    if (fontSize < 0) { fontSize = fontSize_; }
    const auto key = (std::uint64_t(fontSize) << 32) | std::uint64_t(ch);
    const auto ite = fontCache_.find(key);
    if (ite == fontCache_.end() || ite->second.advW == 0) {
        width = t = b = 0;
        return false;
    }
    const auto &fd = ite->second;
    width = monoWidth_ ? std::max(fd.advW, monoWidth_) : fd.advW;
    t = fd.iy0;
    b = fd.iy0 + fd.h;
    return true;
}

bool TTF::measureCharAdvance(std::uint32_t ch, int &advance,
                             int fontSize) noexcept {
    if (fontSize < 0) { fontSize = fontSize_; }
    if (fontSize <= 0) { return false; }
    for (auto &font: fonts_) {
#ifdef USE_FREETYPE
        const auto index = FT_Get_Char_Index(font.face, ch);
        if (index == 0
            || FT_Set_Pixel_Sizes(
                font.face, 0, static_cast<FT_UInt>(fontSize))
            || FT_Load_Glyph(font.face, index, FT_LOAD_DEFAULT)) {
            continue;
        }
        const auto measured = font.face->glyph->advance.x >> 6;
#else
        auto *info = static_cast<stbtt_fontinfo *>(font.font);
        const auto index = stbtt_FindGlyphIndex(info, ch);
        if (index == 0) { continue; }
        int rawAdvance = 0;
        int leftBearing = 0;
        stbtt_GetGlyphHMetrics(info, index, &rawAdvance, &leftBearing);
        (void)leftBearing;
        const auto scale = stbtt_ScaleForMappingEmToPixels(
            info, static_cast<float>(fontSize));
        const auto measured = std::lround(scale * static_cast<float>(rawAdvance));
#endif
        if (measured <= 0 || measured > std::numeric_limits<int>::max()) {
            return false;
        }
        advance = monoWidth_ ? std::max<int>(measured, monoWidth_) : measured;
        return true;
    }
    return false;
}

bool TTF::findGlyph(std::uint32_t ch, std::size_t &fontIndex,
                    std::uint32_t &glyphIndex) const noexcept {
    for (std::size_t index = 0; index < fonts_.size(); ++index) {
#ifdef USE_FREETYPE
        const auto glyph = FT_Get_Char_Index(fonts_[index].face, ch);
#else
        const auto glyph = stbtt_FindGlyphIndex(
            static_cast<const stbtt_fontinfo *>(fonts_[index].font), ch);
#endif
        if (glyph != 0) {
            fontIndex = index;
            glyphIndex = static_cast<std::uint32_t>(glyph);
            return true;
        }
    }
    return false;
}

bool TTF::measureKerningAdvance(std::uint32_t left, std::uint32_t right,
                                int &advance, int fontSize) noexcept {
    if (fontSize < 0) { fontSize = fontSize_; }
    advance = 0;
    if (fontSize <= 0) { return false; }
    const auto prepared = kerningCache_.find(KerningKey{fontSize, left, right});
    if (prepared != kerningCache_.end()) {
        advance = prepared->second;
        return true;
    }
    std::size_t leftFont = 0, rightFont = 0;
    std::uint32_t leftGlyph = 0, rightGlyph = 0;
    if (!findGlyph(left, leftFont, leftGlyph)
        || !findGlyph(right, rightFont, rightGlyph)) {
        return false;
    }
    if (monoWidth_ || leftFont != rightFont) { return true; }

    double measured = 0.0;
#ifdef USE_FREETYPE
    auto *face = fonts_[leftFont].face;
    if (!FT_HAS_KERNING(face)) { return true; }
    if (FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(fontSize))) {
        return false;
    }
    FT_Vector delta{};
    if (FT_Get_Kerning(face, static_cast<FT_UInt>(leftGlyph),
                       static_cast<FT_UInt>(rightGlyph),
                       FT_KERNING_DEFAULT, &delta)) {
        return false;
    }
    measured = static_cast<double>(delta.x) / 64.0;
#else
    auto *info = static_cast<const stbtt_fontinfo *>(fonts_[leftFont].font);
    const auto raw = stbtt_GetGlyphKernAdvance(
        info, static_cast<int>(leftGlyph), static_cast<int>(rightGlyph));
    const auto scale = stbtt_ScaleForMappingEmToPixels(
        info, static_cast<float>(fontSize));
    measured = static_cast<double>(raw) * static_cast<double>(scale);
#endif
    if (!std::isfinite(measured)
        || measured < static_cast<double>(std::numeric_limits<int>::min())
        || measured > static_cast<double>(std::numeric_limits<int>::max())) {
        return false;
    }
    advance = static_cast<int>(std::lround(measured));
    return true;
}

bool TTF::prepareKerning(std::uint32_t left, std::uint32_t right,
                         int fontSize) noexcept {
    const KerningKey key{fontSize, left, right};
    if (kerningCache_.find(key) != kerningCache_.end()) { return true; }
    int advance = 0;
    if (!measureKerningAdvance(left, right, advance, fontSize)) {
        return false;
    }
    try {
        kerningCache_.emplace(key, advance);
        return true;
    } catch (...) {
        return false;
    }
}

int TTF::preparedKerningAdvance(std::uint32_t left, std::uint32_t right,
                                int fontSize) const noexcept {
    const auto found = kerningCache_.find(KerningKey{fontSize, left, right});
    return found == kerningCache_.end() ? 0 : found->second;
}

bool TTF::prepareText(std::wstring_view str, int fontSize) {
    if (fontSize < 0) { fontSize = fontSize_; }
    bool ok = true;
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
        const auto cached = fontCache_.find(key);
        bool glyphReady = false;
        if (cached != fontCache_.end()) {
            glyphReady = cached->second.advW != 0;
        } else {
            glyphReady = makeCache(ch, fontSize) != nullptr;
        }
        if (!glyphReady) {
            ok = false;
            hasPrevious = false;
            continue;
        }
        if (hasPrevious) {
            (void)prepareKerning(previous, ch, fontSize);
        }
        previous = ch;
        hasPrevious = true;
    }
    return ok;
}

int TTF::preparedStringWidth(std::wstring_view str, int fontSize) const {
    if (fontSize < 0) { fontSize = fontSize_; }
    std::int64_t result = 0;
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
        if (hasPrevious) {
            result += preparedKerningAdvance(previous, ch, fontSize);
        }
        result += effectiveAdvance(ite->second);
        if (result > std::numeric_limits<int>::max()) {
            return std::numeric_limits<int>::max();
        }
        previous = ch;
        hasPrevious = true;
    }
    return result <= 0 ? 0 : static_cast<int>(result);
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

const TTF::FontData *TTF::makeCache(std::uint32_t ch, int fontSize) {
    if (fontSize < 0) fontSize = fontSize_;
    const std::uint64_t key = (std::uint64_t(fontSize) << 32) | std::uint64_t(ch);
    const auto cached = fontCache_.find(key);
    if (cached != fontCache_.end()) {
        return cached->second.advW == 0 ? nullptr : &cached->second;
    }

    FontInfo *fi = nullptr;
#ifndef USE_FREETYPE
    stbtt_fontinfo *info;
    std::uint32_t index = 0;
#endif
    for (auto &f: fonts_) {
#ifdef USE_FREETYPE
        auto index = FT_Get_Char_Index(f.face, ch);
        if (index == 0) continue;
        if (FT_Set_Pixel_Sizes(f.face, 0, static_cast<FT_UInt>(fontSize))) {
            continue;
        }
        auto err = FT_Load_Glyph(f.face, index, FT_LOAD_DEFAULT);
        if (!err) { fi = &f; break; }
#else
        info = static_cast<stbtt_fontinfo*>(f.font);
        index = stbtt_FindGlyphIndex(info, ch);
        if (index != 0) { fi = &f; break; }
#endif
    }
    if (fi == nullptr) {
        fontCache_.insert_or_assign(key, FontData{});
        return nullptr;
    }

    FontData candidate{};

#ifdef USE_FREETYPE
    unsigned char *srcPtr;
    int bitmapPitch;
    if (FT_Render_Glyph(fi->face->glyph, FT_RENDER_MODE_NORMAL)) return nullptr;
    FT_GlyphSlot slot = fi->face->glyph;
    std::int64_t glyphY = 0;
    if (!logic::centeredGlyphTop(
            fontSize,
            static_cast<double>(fi->face->size->metrics.ascender) / 64.0,
            static_cast<double>(fi->face->size->metrics.descender) / 64.0,
            -static_cast<std::int64_t>(slot->bitmap_top), glyphY)) {
        return nullptr;
    }
    if (slot->bitmap.width > std::numeric_limits<std::uint8_t>::max()
        || slot->bitmap.rows > std::numeric_limits<std::uint8_t>::max()
        || slot->bitmap_left < std::numeric_limits<std::int8_t>::min()
        || slot->bitmap_left > std::numeric_limits<std::int8_t>::max()
        || glyphY < std::numeric_limits<std::int8_t>::min()
        || glyphY > std::numeric_limits<std::int8_t>::max()
        || slot->advance.x < 0
        || (slot->advance.x >> 6) > std::numeric_limits<std::uint8_t>::max()) {
        return nullptr;
    }
    candidate.ix0 = static_cast<std::int8_t>(slot->bitmap_left);
    candidate.iy0 = static_cast<std::int8_t>(glyphY);
    candidate.w = static_cast<std::uint8_t>(slot->bitmap.width);
    candidate.h = static_cast<std::uint8_t>(slot->bitmap.rows);
    candidate.advW = static_cast<std::uint8_t>(slot->advance.x >> 6);
    srcPtr = slot->bitmap.buffer;
    bitmapPitch = slot->bitmap.pitch;
#else
    /* Read font data to cache */
    int advW, leftB;
    float fontScale = stbtt_ScaleForMappingEmToPixels(info, static_cast<float>(fontSize));
    stbtt_GetGlyphHMetrics(info, index, &advW, &leftB);
    int ascent, descent;
    stbtt_GetFontVMetrics(info, &ascent, &descent, nullptr);
    const auto advance = std::lround(fontScale * float(advW));
    int x0, y0, x1, y1;
    stbtt_GetGlyphBitmapBox(info, index, fontScale, fontScale, &x0, &y0, &x1, &y1);
    const auto w = x1 - x0;
    const auto h = y1 - y0;
    std::int64_t glyphY = 0;
    if (!logic::centeredGlyphTop(
            fontSize, static_cast<double>(ascent) * fontScale,
            static_cast<double>(descent) * fontScale, y0, glyphY)) {
        return nullptr;
    }
    if (advance < 0 || advance > std::numeric_limits<std::uint8_t>::max()
        || w < 0 || w > std::numeric_limits<std::uint8_t>::max()
        || h < 0 || h > std::numeric_limits<std::uint8_t>::max()
        || x0 < std::numeric_limits<std::int8_t>::min() || x0 > std::numeric_limits<std::int8_t>::max()
        || glyphY < std::numeric_limits<std::int8_t>::min() || glyphY > std::numeric_limits<std::int8_t>::max()) {
        return nullptr;
    }
    candidate.advW = static_cast<std::uint8_t>(advance);
    candidate.ix0 = static_cast<std::int8_t>(x0);
    candidate.iy0 = static_cast<std::int8_t>(glyphY);
    candidate.w = static_cast<std::uint8_t>(w);
    candidate.h = static_cast<std::uint8_t>(h);
#endif

    if (candidate.advW == 0) {
        fontCache_.insert_or_assign(key, FontData{});
        return nullptr;
    }
    if (candidate.w == 0 || candidate.h == 0) {
        auto inserted = fontCache_.emplace(key, candidate);
        return &inserted.first->second;
    }

    const int dstPitch = int((candidate.w + 1u) & ~1u);
    if (dstPitch <= 0 || candidate.h > 0
        && static_cast<std::size_t>(dstPitch) > std::numeric_limits<std::size_t>::max() / candidate.h) {
        return nullptr;
    }
    std::vector<std::uint8_t> dst(static_cast<std::size_t>(dstPitch) * candidate.h, 0);

    /* Get last rect pack bitmap */
    auto rpidx = rectpacker_->pack(dstPitch, candidate.h, candidate.rpx, candidate.rpy);
    if (rpidx < 0 || rpidx > std::numeric_limits<std::uint8_t>::max()) {
        if (rpidx >= 0) { rectpacker_->rollbackLast(); }
        return nullptr;
    }
    candidate.rpidx = static_cast<std::uint8_t>(rpidx);

#ifdef USE_FREETYPE
    auto *dstPtr = dst.data();
    for (int k = 0; k < candidate.h; ++k) {
        memcpy(dstPtr, srcPtr, candidate.w);
        srcPtr += bitmapPitch;
        dstPtr += dstPitch;
    }
#else
    stbtt_MakeGlyphBitmapSubpixel(info, dst.data(), candidate.w, candidate.h,
                                  dstPitch, fontScale, fontScale, 0, 0, index);
#endif

    std::unique_ptr<Texture> textureCandidate;
    auto *tex = rpidx < static_cast<int>(textures_.size()) ? textures_[rpidx] : nullptr;
    if (!tex) {
        textureCandidate.reset(Texture::create(renderer_, RectPackWidthDefault, RectPackWidthDefault));
        if (!textureCandidate || !textureCandidate->enableBlendMode(true)) {
            rectpacker_->rollbackLast();
            return nullptr;
        }
        tex = textureCandidate.get();
    }

    int pitch = 0;
    TextureLock lock(tex, pitch, candidate.rpx, candidate.rpy, dstPitch, candidate.h);
    if (!lock.valid() || pitch < dstPitch) {
        rectpacker_->rollbackLast();
        return nullptr;
    }
    auto *pixels = lock.pixels();
    auto *source = dst.data();
    const int offset = pitch - dstPitch;
    for (int row = 0; row < candidate.h; ++row) {
        for (int column = 0; column < dstPitch; ++column) {
            *pixels++ = 0xFFFFFFu | (std::uint32_t(*source++) << 24);
        }
        pixels += offset;
    }
    lock.unlock();

    decltype(fontCache_.begin()) inserted;
    try {
        inserted = fontCache_.emplace(key, candidate).first;
    } catch (...) {
        rectpacker_->rollbackLast();
        return nullptr;
    }
    if (textureCandidate) {
        try {
            if (rpidx >= static_cast<int>(textures_.size())) {
                textures_.resize(static_cast<std::size_t>(rpidx) + 1, nullptr);
            }
        } catch (...) {
            fontCache_.erase(key);
            rectpacker_->rollbackLast();
            return nullptr;
        }
        textures_[rpidx] = textureCandidate.release();
    }
    return &inserted->second;
}

}
