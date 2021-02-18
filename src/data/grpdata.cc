#include "grpdata.hh"

#include "core/config.hh"
#include "util/file.hh"

namespace hojy::data {

GrpData grpData;

bool GrpData::load(const std::string &name) {
    util::File ifs, ifs2;
    if (!ifs.open(core::config.dataFilePath(name + ".IDX")) ||
        !ifs2.open(core::config.dataFilePath(name + ".GRP"))) {
        return false;
    }
    size_t count = ifs.size() / sizeof(std::uint32_t);
    DataSet &dset = data_[name];
    dset.resize(count);
    std::uint32_t offset = 0;
    for (size_t i = 0; i < count; ++i) {
        std::uint32_t endoffset;
        ifs.read(&endoffset, sizeof(endoffset));
        if (endoffset > offset) {
            dset[i].resize(endoffset - offset);
            ifs2.seek(offset);
            ifs2.read(dset[i].data(), endoffset - offset);
        }
        offset = endoffset;
    }
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
