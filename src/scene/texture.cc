#include "texture.hh"

#include <SDL.h>

namespace hojy::scene {

TextureMgr mapTextureMgr;

bool Texture::loadFromRLE(void *renderer, const std::vector<std::uint8_t> &data, void *palette) {
    size_t left = data.size();
    if (left < 8) {
        return false;
    }
    const auto *buf = data.data();
    struct Header {
        std::uint16_t w, h, x, y;
    };
    const auto *hdr = reinterpret_cast<const Header*>(buf);
    buf += 8;
    left -= 8;
    std::vector<std::uint8_t> bitmap;
    bitmap.resize(hdr->w * hdr->h);
    std::uint32_t y = 0, w = hdr->w, h = hdr->h;
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
    data_ = SDL_CreateTextureFromSurface(static_cast<SDL_Renderer*>(renderer), surface);
    SDL_FreeSurface(surface);
    return true;
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
    return false;
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
