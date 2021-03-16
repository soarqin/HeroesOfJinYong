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

#include "config.hh"

#include "util/file.hh"
#include "util/conv.hh"
#include "external/toml.hpp"

#include <iostream>

namespace hojy::core {

Config config;

bool Config::load(const std::string &filename) {
    toml::table tbl;
    try {
        tbl = toml::parse(util::File::getFileContent(filename));
    } catch (const toml::parse_error &err) {
        std::cerr << "Parsing failed: " << err << std::endl;
        return false;
    }
    auto main = tbl["main"];
    if (main) {
        auto dpath = main["data_path"];
        if (dpath.is_string()) {
            dataPath_.resize(1);
            dataPath_[0] = dpath.value_or<std::string>(".");
        } else if (dpath.is_array()) {
            for (auto &p: *dpath.as_array()) {
                dataPath_.emplace_back(p.value_or<std::string>("."));
            }
        }
        auto fixPath = [](std::string &path) {
            if (!path.empty() && path.back() != '/') { path += '/'; }
        };
        for (auto &path: dataPath_) {
            fixPath(path);
        }
        musicPath_ = main["music_path"].value_or("");
        fixPath(musicPath_);
        soundPath_ = main["sound_path"].value_or("");
        fixPath(soundPath_);
        savePath_ = main["save_path"].value_or("");
        fixPath(savePath_);
        auto fonts = main["fonts"];
        if (fonts.is_string()) {
            fonts_ = {fonts.value_or<std::string>("")};
        } else if (fonts.is_array()) {
            for (auto &p: *fonts.as_array()) {
                fonts_.emplace_back(p.value_or<std::string>(""));
            }
        }
    }
    auto window = tbl["window"];
    if (window) {
        windowWidth_ = window["width"].value_or<int>(640);
        windowHeight_ = window["height"].value_or<int>(480);
    }
    auto ui = tbl["ui"];
    if (ui) {
        showPotential_ = ui["show_potential"].value_or<bool>(false);
        scale_ = ui["scale"].value_or<float>(2.f);
        animationSpeed_ = ui["animation_speed"].value_or<float>(1.f);
        fadeSpeed_ = ui["fade_speed"].value_or<float>(1.f);
        noNameInput_ = ui["no_name_input"].value_or<bool>(false);
        defaultName_ = util::Utf8Conv::toUnicode(ui["default_name"].value_or<std::string>(""));
    }
    if (defaultName_.empty()) {
        defaultName_ = L"徐小俠";
    }
    return true;
}

std::string Config::dataFilePathFirst(const std::string &filename) const {
    if (dataPath_.empty()) {
        return filename;
    }
    return dataPath_[0] + filename;
}

std::vector<std::string> Config::dataFilePath(const std::string &filename) const {
    std::vector<std::string> res;
    for (auto &d: dataPath_) {
        res.emplace_back(d + filename);
    }
    return res;
}

std::string Config::musicFilePath(const std::string &filename) const {
    return musicPath_.empty() ? dataFilePathFirst(filename) : musicPath_ + filename;
}

std::string Config::soundFilePath(const std::string &filename) const {
    return soundPath_.empty() ? dataFilePathFirst(filename) : soundPath_ + filename;
}

std::string Config::saveFilePath(const std::string &filename) const {
    return savePath_.empty() ? dataFilePathFirst(filename) : savePath_ + filename;
}

}
