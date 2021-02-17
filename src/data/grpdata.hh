#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

namespace hojy::data {

class GrpData final {
public:
    using DataSet = std::vector<std::vector<std::uint8_t>>;

public:
    bool load(const std::string &name);
    const DataSet &operator[](const std::string &name) const;

private:
    std::unordered_map<std::string, DataSet> data_;
};

extern GrpData grpData;

}
