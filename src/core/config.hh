#pragma once

#include <string>

namespace hojy::core {

class Config {
public:
    bool load(const std::string &filename);

    inline std::string dataFilePath(const std::string &filename) const { return dataPath_ + filename; }

private:
    std::string dataPath_;
};

extern Config config;

}
