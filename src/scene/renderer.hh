#pragma once

namespace hojy::scene {

class Texture;

class Renderer final {
    friend class Texture;

public:
    explicit Renderer(void *win);
    Renderer(const Renderer&) = delete;
    ~Renderer();

    void renderTexture(const Texture *tex, int x, int y);

    void present();

private:
    void *renderer_ = nullptr;
};

}
