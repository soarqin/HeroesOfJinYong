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
        for (auto &path: dataPath_) {
            if (!path.empty() && path.back() != '/') { path += '/'; }
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

}
