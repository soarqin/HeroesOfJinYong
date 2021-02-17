#include "config.hh"

#include "external/toml.hpp"

#include <fstream>
#include <iostream>

namespace hojy::core {

Config config;

bool Config::load(const std::string &filename) {
    toml::table tbl;
    try {
        tbl = toml::parse_file(filename);
    } catch (const toml::parse_error &err) {
        std::cerr << "Parsing failed: " << err << std::endl;
        return false;
    }
    dataPath_ = tbl["main"]["data_path"].value_or<std::string>(".");
    if (!dataPath_.empty()) { dataPath_ += '/'; }
    return true;
}

}
