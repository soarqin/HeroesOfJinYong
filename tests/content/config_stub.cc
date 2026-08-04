#include "core/config.hh"

namespace hojy::core {

Config config;

std::string Config::dataFilePath(const std::string &filename) const {
    return filename;
}

std::string Config::saveFilePath(const std::string &filename) const {
    return filename;
}

}
