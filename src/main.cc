#include <iostream>

#include "core/config.hh"
#include "data/loader.hh"

#include "data/grpdata.hh"
#include "data/colorpalette.hh"
#include "scene/texture.hh"
#include "scene/window.hh"

using namespace hojy;

int main() {
    core::config.load("config.toml");
    data::loadData();
    scene::Window win(640, 480);
    scene::mapTextureMgr.setRenderer(win.renderer());
    scene::mapTextureMgr.setPalette(data::normalPalette.colors(), data::normalPalette.size());
    scene::mapTextureMgr.loadFromRLE(500, data::grpData["MMAP"][500]);
    while (win.processEvents()) {
        win.render(&scene::mapTextureMgr[500], 100, 100);
        win.flush();
    }
    return 0;
}
