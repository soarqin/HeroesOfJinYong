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
