#pragma once

#include "map.hh"
#include "renderer.hh"

namespace hojy::scene {

class Window final {
public:
    Window(int w, int h);
    ~Window();

    [[nodiscard]] Renderer *renderer() {
        return renderer_;
    }

    bool processEvents();
    void render();

    void flush();

private:
    void *win_ = nullptr;
    Renderer *renderer_ = nullptr;
    Map *map_ = nullptr;
};

}
