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
    dataPath_ = tbl["main"]["data_path"].value_or<std::string>(".");
    if (!dataPath_.empty()) { dataPath_ += '/'; }
    return true;
}

}
