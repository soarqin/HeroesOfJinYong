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
#include "content/atomic_file.hh"
#include "content/binary_reader.hh"
#include "util/file.hh"

#include <cstdint>
#include <limits>
#include <new>
#include <string>
#include <utility>

namespace hojy::content {

namespace {

bool readFile(const std::string &path, std::string &data) {
    util::File file = util::File::open(path);
    if (!file) {
        return false;
    }
    const auto fileSize = file.size();
    if (fileSize > std::numeric_limits<std::size_t>::max()
        || fileSize > std::string().max_size()) {
        return false;
    }
    try {
        std::string loaded(static_cast<std::size_t>(fileSize), '\0');
        if (!loaded.empty() && file.read(loaded.data(), loaded.size()) != loaded.size()) {
            return false;
        }
        data = std::move(loaded);
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

}

bool GrpData::loadData(const std::string &idx, const std::string &grp, GrpData::DataSet &dset, bool isSave) {
    std::string indexData;
    std::string groupData;
    if (isSave) {
        if (!readFile(core::config.saveFilePath(idx), indexData)
            || !readFile(core::config.saveFilePath(grp), groupData)) {
            return false;
        }
    } else {
        if (!readFile(core::config.dataFilePath(idx), indexData)
            || !readFile(core::config.dataFilePath(grp), groupData)) {
            return false;
        }
    }
    const auto idxSize = indexData.size();
    const auto fileSize = groupData.size();
    if (idxSize % sizeof(std::uint32_t) != 0
        || fileSize > std::numeric_limits<std::uint32_t>::max()
        || idxSize / sizeof(std::uint32_t) > std::vector<std::string>().max_size()) {
        return false;
    }
    const auto count = static_cast<size_t>(idxSize / sizeof(std::uint32_t));
    try {
        DataSet loaded(count);
        content::BinaryReader indexReader(indexData);
        std::uint32_t offset = 0;
        bool reachedEnd = false;
        for (size_t i = 0; i < count; ++i) {
            std::uint32_t endoffset = 0;
            if (!indexReader.readPod(endoffset)) {
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
            if (size > 0) {
                loaded[i].assign(groupData.data() + offset, size);
            }
            offset = endoffset;
        }
        if (offset != fileSize) { return false; }
        dset = std::move(loaded);
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

bool GrpData::loadData(const std::string &name, GrpData::DataSet &dset, bool isSave) {
    return loadData(name + ".IDX", name + ".GRP", dset, isSave);
}

bool GrpData::saveData(const std::string &name, const GrpData::DataSet &dset, bool isSave) {
    try {
        std::uint64_t totalSize = 0;
        for (size_t i = 0; i < dset.size(); ++i) {
            const auto &data = dset[i];
            if (data.size() > std::numeric_limits<std::uint64_t>::max() - totalSize) {
                return false;
            }
            totalSize += data.size();
            if (totalSize > std::numeric_limits<std::uint32_t>::max()) {
                return false;
            }
            if (totalSize == 0 && i + 1 != dset.size()) {
                return false;
            }
        }

        if (dset.size() > std::string().max_size() / sizeof(std::uint32_t)) {
            return false;
        }
        std::string indexData;
        std::string groupData;
        indexData.reserve(dset.size() * sizeof(std::uint32_t));
        groupData.reserve(static_cast<std::size_t>(totalSize));
        std::uint32_t offset = 0;
        for (const auto &data: dset) {
            offset += static_cast<std::uint32_t>(data.size());
            indexData.append(reinterpret_cast<const char *>(&offset), sizeof(offset));
            groupData.append(data);
        }

        const auto indexPath = isSave
            ? core::config.saveFilePath(name + ".IDX")
            : core::config.dataFilePath(name + ".IDX");
        const auto groupPath = isSave
            ? core::config.saveFilePath(name + ".GRP")
            : core::config.dataFilePath(name + ".GRP");
        return content::AtomicFile::writePair({
            {indexPath, std::move(indexData)},
            {groupPath, std::move(groupData)},
        });
    } catch (const std::bad_alloc &) {
        return false;
    }
}

}
