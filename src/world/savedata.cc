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

#include "savedata.hh"

#include "content/atomic_file.hh"
#include "core/config.hh"
#include "content/grpdata.hh"

#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace hojy::world::state {

namespace {

bool stageArchive(const std::string &name,
                  const ::hojy::content::GrpData::DataSet &data,
                  std::vector<content::AtomicFileEntry> &files) {
    std::uint64_t totalSize = 0;
    for (const auto &entry: data) {
        totalSize += entry.size();
        if (totalSize > std::numeric_limits<std::uint32_t>::max()) {
            return false;
        }
    }
    if (data.size() > std::numeric_limits<std::size_t>::max() / sizeof(std::uint32_t)) {
        return false;
    }

    std::string indexData;
    std::string groupData;
    indexData.reserve(data.size() * sizeof(std::uint32_t));
    groupData.reserve(static_cast<std::size_t>(totalSize));
    std::uint32_t offset = 0;
    for (const auto &entry: data) {
        if (totalSize == 0 && &entry != &data.back()) {
            return false;
        }
        offset += static_cast<std::uint32_t>(entry.size());
        indexData.append(reinterpret_cast<const char *>(&offset), sizeof(offset));
        groupData.append(entry);
    }

    files.push_back(content::AtomicFileEntry{
        core::config.saveFilePath(name + ".IDX"), std::move(indexData)});
    files.push_back(content::AtomicFileEntry{
        core::config.saveFilePath(name + ".GRP"), std::move(groupData)});
    return true;
}

}

SaveData gSaveData;

static void buildSaveFilename(int num, std::string &rangerFile, std::string &sinFile, std::string &defFile) {
    if (num == 0) {
        rangerFile = "RANGER";
        sinFile = "ALLSIN";
        defFile = "ALLDEF";
    } else {
        rangerFile = "R" + std::to_string(num);
        sinFile = "S" + std::to_string(num);
        defFile = "D" + std::to_string(num);
    }
}

bool SaveData::newGame() {
    return load(0);
}

bool SaveData::newGame(Bag &bag) {
    return load(0, bag);
}

bool SaveData::load(int num) {
    return load(num, gBag);
}

bool SaveData::load(int num, Bag &bag) {
    std::string rangerFile, sinFile, defFile;
    buildSaveFilename(num, rangerFile, sinFile, defFile);
    ::hojy::content::GrpData::DataSet rangerData, sinData, defData;
    if (!::hojy::content::GrpData::loadData(rangerFile, rangerData, num > 0)) {
        return false;
    }
    if (rangerData.size() < 6) {
        return false;
    }
    if (!::hojy::content::GrpData::loadData(sinFile, sinData, num > 0)) {
        return false;
    }
    if (!::hojy::content::GrpData::loadData(defFile, defData, num > 0)) {
        return false;
    }

    SaveData loaded;
    if (!loaded.baseInfo.deserialize(rangerData[0])
        || !loaded.charInfo.deserialize(rangerData[1])
        || !loaded.itemInfo.deserialize(rangerData[2])
        || !loaded.subMapInfo.deserialize(rangerData[3])
        || !loaded.skillInfo.deserialize(rangerData[4])
        || !loaded.shopInfo.deserialize(rangerData[5])) {
        return false;
    }
    const auto subMapCount = loaded.subMapInfo.size();
    if (sinData.size() != subMapCount || defData.size() != subMapCount) {
        return false;
    }
    loaded.subMapLayerInfo.resize(subMapCount);
    loaded.subMapEventInfo.resize(subMapCount);
    for (size_t i = 0; i < subMapCount; ++i) {
        if (!loaded.subMapLayerInfo[i].deserialize(sinData[i])
            || !loaded.subMapEventInfo[i].deserialize(defData[i])) {
            return false;
        }
    }

    Bag loadedBag;
    if (!loadedBag.syncFrom(*loaded.baseInfo.operator->())) {
        return false;
    }
    swap(loaded);
    bag.swap(loadedBag);
    return true;
}

void SaveData::swap(SaveData &other) noexcept {
    baseInfo.swap(other.baseInfo);
    charInfo.swap(other.charInfo);
    itemInfo.swap(other.itemInfo);
    subMapInfo.swap(other.subMapInfo);
    subMapLayerInfo.swap(other.subMapLayerInfo);
    subMapEventInfo.swap(other.subMapEventInfo);
    skillInfo.swap(other.skillInfo);
    shopInfo.swap(other.shopInfo);
}

bool SaveData::save(int num) {
    if (subMapLayerInfo.size() != subMapInfo.size()
        || subMapEventInfo.size() != subMapInfo.size()) {
        return false;
    }
    std::string rangerFile, sinFile, defFile;
    buildSaveFilename(num, rangerFile, sinFile, defFile);
    SaveData snapshot = *this;
    gBag.syncTo(*snapshot.baseInfo.operator->());

    ::hojy::content::GrpData::DataSet ranger(6);
    snapshot.baseInfo.serialize(ranger[0]);
    snapshot.charInfo.serialize(ranger[1]);
    snapshot.itemInfo.serialize(ranger[2]);
    snapshot.subMapInfo.serialize(ranger[3]);
    snapshot.skillInfo.serialize(ranger[4]);
    snapshot.shopInfo.serialize(ranger[5]);

    ::hojy::content::GrpData::DataSet layers(snapshot.subMapLayerInfo.size());
    for (size_t i = 0; i < layers.size(); ++i) {
        snapshot.subMapLayerInfo[i].serialize(layers[i]);
    }
    ::hojy::content::GrpData::DataSet events(snapshot.subMapEventInfo.size());
    for (size_t i = 0; i < events.size(); ++i) {
        snapshot.subMapEventInfo[i].serialize(events[i]);
    }

    std::vector<content::AtomicFileEntry> files;
    files.reserve(6);
    if (!stageArchive(rangerFile, ranger, files)
        || !stageArchive(sinFile, layers, files)
        || !stageArchive(defFile, events, files)) {
        return false;
    }
    if (!content::AtomicFile::writePair(files)) {
        return false;
    }
    gBag.syncToSave();
    return true;
}

}
