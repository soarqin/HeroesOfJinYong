#pragma once

#include <cstdint>

namespace hojy::scene {

class Texture;

class Renderer final {
    friend class Texture;

public:
    explicit Renderer(void *win);
    Renderer(const Renderer&) = delete;
    ~Renderer();

    void setTargetTexture(Texture *tex);
    void enableBlendMode(bool r);
    void setClipRect(int l, int r, int w, int h);
    void unsetClipRect();
    void fill(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a);
    void renderTexture(const Texture *tex, int x, int y, bool ignoreOrigin = false);
    void renderTexture(const Texture *tex, int x, int y, int w, int h, bool ignoreOrigin = false);

    void present();

private:
    void *renderer_ = nullptr;
};

}
