#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include <cstdint>

namespace hojy::scene {

class Texture final {
    friend class TextureMgr;

public:
    [[nodiscard]] void *data() const { return data_; }

private:
    bool loadFromRLE(void *renderer, const std::vector<std::uint8_t> &data, void *palette);

private:
    void *data_ = nullptr;
};

class TextureMgr final {
public:
    inline void setRenderer(void *renderer) { renderer_ = renderer; }
    void setPalette(const std::uint32_t *colors, std::size_t size);
    bool loadFromRLE(std::int32_t id, const std::vector<std::uint8_t> &data);
    const Texture &operator[](std::int32_t id) const;

private:
    std::unordered_map<std::int32_t, Texture> textures_;
    void *renderer_ = nullptr;
    void *palette_ = nullptr;
};

extern TextureMgr mapTextureMgr;

}
