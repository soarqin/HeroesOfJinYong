#include <iostream>

#include "core/config.hh"
#include "data/loader.hh"

#include "data/grpdata.hh"
#include "data/colorpalette.hh"
#include "scene/texture.hh"
#include "scene/window.hh"

using namespace hojy;

enum {
    START = 3715,
    COUNT = 32,
};

int main() {
    core::config.load("config.toml");
    data::loadData();
    scene::Window win(640, 480);
    scene::mapTextureMgr.setRenderer(win.renderer());
    scene::mapTextureMgr.setPalette(data::normalPalette.colors(), data::normalPalette.size());
    for (int i = 0, j = START; i < COUNT; ++i, ++j) {
        scene::mapTextureMgr.loadFromRLE(j, data::grpData["MMAP"][j]);
    }
    while (win.processEvents()) {
        for (int i = 0, j = START; i < COUNT; ++i, ++j) {
            win.render(&scene::mapTextureMgr[j], 50 + (i / 8) * 50, 50 + (i % 8) * 50);
        }
        win.flush();
    }
    return 0;
}
