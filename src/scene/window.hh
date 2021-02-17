#pragma once

namespace hojy::scene {

class Texture;

struct WindowCtx;

class Window final {
public:
    Window(int w, int h);
    ~Window();

    [[nodiscard]] void *renderer();

    bool processEvents();

    void flush();

    void render(const Texture *tex, int x, int y);

private:
    WindowCtx *ctx_;
};

}
