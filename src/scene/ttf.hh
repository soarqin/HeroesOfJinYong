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
#include <cstdint>

#ifdef USE_FREETYPE
extern "C" {
typedef struct FT_LibraryRec_ *FT_Library;
typedef struct FT_FaceRec_    *FT_Face;
}
#endif

namespace hojy::scene {

struct rect_pack_data;

class TTF {
protected:
    struct FontData {
        int16_t rpx, rpy;
        std::uint8_t rpidx;

        std::int8_t ix0, iy0;
        std::uint8_t w, h;
        std::uint8_t advW;
    };
    struct FontInfo {
#ifdef USE_FREETYPE
        FT_Face face = nullptr;
#else
        float font_scale = 0.f;
        void *font = nullptr;
        std::vector<std::uint8_t> ttf_buffer;
#endif
    };
public:
    TTF(void *renderer);
    virtual ~TTF();

    virtual void setDrawColor(std::uint8_t r, std::uint8_t g, std::uint8_t b) {}

    void init(int size, std::uint8_t width = 0);
    void deinit();
    bool add(const std::string& filename, int index = 0);
    void charDimension(std::uint16_t ch, std::uint8_t &width, std::int8_t &t, std::int8_t &b);

    inline int fontSize() const { return fontSize_; }

    void render(std::wstring_view str, int x, int y, int maxw);

private:
    void newRectPack();

protected:
    inline const FontData *getCache(std::uint16_t ch) {
        auto ite = fontCache_.find(ch);
        if (ite != fontCache_.end()) return &ite->second;
        return makeCache(ch);
    }
    const FontData *makeCache(std::uint16_t ch);

protected:
    int fontSize_ = 16;
    std::vector<FontInfo> fonts_;
    std::unordered_map<std::uint16_t, FontData> fontCache_;
    std::uint8_t monoWidth_ = 0;

private:
    void *renderer_;
    std::vector<void*> textures_;

    std::vector<rect_pack_data*> rectpackData_;
#ifdef USE_FREETYPE
    FT_Library ftLib_ = nullptr;
#endif
};

}
