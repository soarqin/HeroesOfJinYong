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
    while (win.processEvents()) {
        win.render();
        win.flush();
    }
    return 0;
}
