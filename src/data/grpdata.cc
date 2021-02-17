#include "grpdata.hh"

#include "core/config.hh"

#include <fstream>

namespace hojy::data {

GrpData grpData;

bool GrpData::load(const std::string &name) {
    std::ifstream ifs(core::config.dataFilePath(name + ".IDX"), std::ios::in | std::ios::binary);
    std::ifstream ifs2(core::config.dataFilePath(name + ".GRP"), std::ios::in | std::ios::binary);
    if (!ifs.is_open() || !ifs2.is_open()) {
        return false;
    }
    ifs.seekg(0, std::ios::end);
    size_t count = ifs.tellg() / sizeof(std::uint32_t);
    ifs.seekg(0, std::ios::beg);
    DataSet &dset = data_[name];
    dset.resize(count);
    std::uint32_t offset = 0;
    for (size_t i = 0; i < count; ++i) {
        std::uint32_t endoffset;
        ifs.read(reinterpret_cast<char*>(&endoffset), sizeof(endoffset));
        ifs2.seekg(offset, std::ios::beg);
        if (endoffset > offset) {
            dset[i].resize(endoffset - offset);
            ifs2.read(reinterpret_cast<char*>(dset[i].data()), endoffset - offset);
        }
        offset = endoffset;
    }
    ifs2.close();
    ifs.close();
    return true;
}

const GrpData::DataSet &GrpData::operator[](const std::string &name) const {
    auto ite = data_.find(name);
    if (ite == data_.end()) {
        static const DataSet empty;
        return empty;
    }
    return ite->second;
}

}
