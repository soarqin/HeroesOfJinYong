/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef _MSC_VER
#define _UNICODE
#define UNICODE
#include <windows.h>
#endif

#include "core/config.hh"
#include "app/application.hh"
#include "content/loader.hh"
#include "world/strings.hh"

#include <cstdlib>
#include <filesystem>

using namespace hojy;

int main(int argc, char *argv[]) {
    if (!core::config.load("config.toml")) { return EXIT_FAILURE; }
    const auto optionsFile = core::config.saveFilePath("options.toml");
    std::error_code ec;
    if (std::filesystem::is_regular_file(optionsFile, ec)) {
        core::config.load(optionsFile);
    }
    if (!core::config.postLoad()) { return EXIT_FAILURE; }
    if (!::hojy::world::state::gStrings.load("strings.toml")) { return EXIT_FAILURE; }
    core::config.fixOnTextLoaded();
    if (!::hojy::content::loadData()) { return EXIT_FAILURE; }
    app::Application application(core::config.windowWidth(), core::config.windowHeight(),
                                 core::config.animationSpeed());
    return application.run();
}

#ifdef _MSC_VER
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    return main(__argc, __argv);
}
#endif
