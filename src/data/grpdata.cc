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

#include "grpdata.hh"

#include "core/config.hh"
#include "util/file.hh"

#include <limits>
#include <utility>

namespace hojy::data {

bool GrpData::loadData(const std::string &idx, const std::string &grp, GrpData::DataSet &dset, bool isSave) {
    util::File ifs, ifs2;
    if (isSave) {
        ifs = util::File::open(core::config.saveFilePath(idx));
        ifs2 = util::File::open(core::config.saveFilePath(grp));
    } else {
        ifs = util::File::open(core::config.dataFilePath(idx));
        ifs2 = util::File::open(core::config.dataFilePath(grp));
    }
    if (!ifs || !ifs2) {
        return false;
    }
    const auto idxSize = ifs.size();
    const auto fileSize = ifs2.size();
    if (idxSize % sizeof(std::uint32_t) != 0
        || fileSize > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }
    const auto count = static_cast<size_t>(idxSize / sizeof(std::uint32_t));
    DataSet loaded(count);
    std::uint32_t offset = 0;
    bool reachedEnd = false;
    for (size_t i = 0; i < count; ++i) {
        std::uint32_t endoffset = 0;
        if (ifs.read(&endoffset, sizeof(endoffset)) != sizeof(endoffset)) {
            return false;
        }
        if (endoffset == 0) {
            reachedEnd = true;
            endoffset = static_cast<std::uint32_t>(fileSize);
        } else if (reachedEnd) {
            return false;
        }
        if (endoffset < offset || endoffset > fileSize) {
            return false;
        }
        const auto size = static_cast<size_t>(endoffset - offset);
        loaded[i].resize(size);
        if (size > 0
            && (ifs2.seek(offset) != offset || ifs2.read(loaded[i].data(), size) != size)) {
            return false;
        }
        offset = endoffset;
    }
    if (offset != fileSize) { return false; }
    dset = std::move(loaded);
    return true;
}

bool GrpData::loadData(const std::string &name, GrpData::DataSet &dset, bool isSave) {
    return loadData(name + ".IDX", name + ".GRP", dset, isSave);
}

bool GrpData::saveData(const std::string &name, const GrpData::DataSet &dset, bool isSave) {
    std::uint64_t totalSize = 0;
    for (size_t i = 0; i < dset.size(); ++i) {
        const auto &data = dset[i];
        totalSize += data.size();
        if (totalSize > std::numeric_limits<std::uint32_t>::max()) {
            return false;
        }
        if (totalSize == 0 && i + 1 != dset.size()) {
            return false;
        }
    }

    util::File ifs, ifs2;
    if (isSave) {
        ifs = util::File::create(core::config.saveFilePath(name + ".IDX"));
        ifs2 = util::File::create(core::config.saveFilePath(name + ".GRP"));
    } else {
        ifs = util::File::create(core::config.dataFilePath(name + ".IDX"));
        ifs2 = util::File::create(core::config.dataFilePath(name + ".GRP"));
    }
    if (!ifs || !ifs2) {
        return false;
    }
    std::uint32_t offset = 0;
    for (auto &d: dset) {
        offset += static_cast<std::uint32_t>(d.size());
        if ((d.size() > 0 && ifs2.write(d.data(), d.size()) != d.size())
            || ifs.write(&offset, sizeof(std::uint32_t)) != sizeof(std::uint32_t)) {
            return false;
        }
    }
    return true;
}

}
