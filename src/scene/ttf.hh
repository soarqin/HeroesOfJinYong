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

#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <memory>
#include <cstdint>

#ifdef USE_FREETYPE
extern "C" {
typedef struct FT_LibraryRec_ *FT_Library;
typedef struct FT_FaceRec_    *FT_Face;
}
#endif

namespace hojy::scene {

enum {
    TextLineSpacing = 5,
};

class Renderer;
class Texture;
class RectPacker;

class TTF final {
protected:
    struct FontData {
        std::int16_t rpx, rpy;
        std::uint8_t rpidx;

        std::int8_t ix0, iy0;
        std::uint8_t w, h;
        std::uint8_t advW;
    };
    struct FontInfo {
#ifdef USE_FREETYPE
        FT_Face face = nullptr;
#else
        void *font = nullptr;
        std::vector<std::uint8_t> ttf_buffer;
#endif
    };
public:
    explicit TTF(Renderer *renderer);
    ~TTF();

    bool init(int size, std::uint8_t width = 0);
    void deinit();
    bool add(const std::string& filename, int index = 0);
    [[nodiscard]] bool ready() const noexcept { return ready_; }
    [[nodiscard]] bool hasFont() const noexcept { return !fonts_.empty(); }
    void charDimension(std::uint32_t ch, std::uint8_t &width, std::int8_t &t, std::int8_t &b, int fontSize = -1);
    int stringWidth(const std::wstring &str, int fontSize = -1);
    bool preparedCharDimension(std::uint32_t ch, std::uint8_t &width,
                               std::int8_t &t, std::int8_t &b, int fontSize = -1) const;
    /** Read a glyph advance from font data without allocating render resources. */
    bool measureCharAdvance(std::uint32_t ch, int &advance,
                            int fontSize = -1) noexcept;
    /** Populate glyph resources during prepareRender; never call from render. */
    bool prepareText(std::wstring_view str, int fontSize = -1);
    /** Read-only width lookup for logic/layout consumers. */
    int preparedStringWidth(std::wstring_view str, int fontSize = -1) const;

    inline int fontSize() const { return fontSize_; }
    void setColor(std::uint8_t r, std::uint8_t g, std::uint8_t b);
    void setAltColor(int index, std::uint8_t r, std::uint8_t g, std::uint8_t b);

    /** Draw only glyphs prepared by prepareText; this overload uses setColor. */
    void render(std::wstring_view str, int x, int y, bool shadow, int fontSize = -1);
    void renderPrepared(std::wstring_view str, int x, int y, bool shadow, int fontSize = -1);
    /** Draw prepared glyphs with an explicit color. */
    void renderPrepared(std::wstring_view str, int x, int y, bool shadow, int fontSize,
                        std::uint8_t r, std::uint8_t g, std::uint8_t b);

private:
    const FontData *makeCache(std::uint32_t ch, int fontSize = - 1);

protected:
    int fontSize_ = 16;
    std::vector<FontInfo> fonts_;
    std::unordered_map<std::uint64_t, FontData> fontCache_;
    std::uint8_t monoWidth_ = 0;

private:
    Renderer *renderer_;

    bool ready_ = true;
    bool initialized_ = false;

    std::uint8_t altR_[16] = {}, altG_[16] = {}, altB_[16] = {};
    std::vector<Texture*> textures_;

    std::unique_ptr<RectPacker> rectpacker_;
#ifdef USE_FREETYPE
    FT_Library ftLib_ = nullptr;
#endif
};

}
