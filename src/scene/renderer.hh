#pragma once

namespace hojy::scene {

class Texture;

class Renderer final {
    friend class Texture;

public:
    explicit Renderer(void *win);
    Renderer(const Renderer&) = delete;
    ~Renderer();

    void setTargetTexture(Texture *tex);
    void renderTexture(const Texture *tex, int x, int y, bool ignoreOrigin = false);

    void present();

private:
    void *renderer_ = nullptr;
};

}
