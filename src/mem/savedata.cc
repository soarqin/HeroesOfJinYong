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

#include <data/grpdata.hh>

namespace hojy::mem {

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

bool SaveData::load(int num) {
    std::string rangerFile, sinFile, defFile;
    buildSaveFilename(num, rangerFile, sinFile, defFile);
    data::GrpData::DataSet rangerData, sinData, defData;
    if (!data::GrpData::loadData(rangerFile, rangerData)) {
        return false;
    }
    if (rangerData.size() < 6) {
        return false;
    }
    baseInfo.deserialize(rangerData[0]);
    charInfo.deserialize(rangerData[1]);
    itemInfo.deserialize(rangerData[2]);
    subMapInfo.deserialize(rangerData[3]);
    skillInfo.deserialize(rangerData[4]);
    shopInfo.deserialize(rangerData[5]);
    if (!data::GrpData::loadData(sinFile, sinData)) {
        return false;
    }
    size_t sz = sinData.size();
    if (sz < subMapInfo.size()) {
        return false;
    }
    subMapLayerInfo.resize(sz);
    for (size_t i = 0; i < sz; ++i) {
        subMapLayerInfo[i].deserialize(sinData[i]);
    }
    if (!data::GrpData::loadData(defFile, defData)) {
        return false;
    }
    sz = defData.size();
    if (sz < subMapInfo.size()) {
        return false;
    }
    subMapEventInfo.resize(sz);
    for (size_t i = 0; i < sz; ++i) {
        subMapEventInfo[i].deserialize(defData[i]);
    }
    return true;
}

bool SaveData::save(int num) {
    std::string rangerFile, sinFile, defFile;
    buildSaveFilename(num, rangerFile, sinFile, defFile);
    data::GrpData::DataSet rangerData, sinData, defData;
    rangerData.resize(6);
    baseInfo.serialize(rangerData[0]);
    charInfo.serialize(rangerData[1]);
    itemInfo.serialize(rangerData[2]);
    subMapInfo.serialize(rangerData[3]);
    skillInfo.serialize(rangerData[4]);
    shopInfo.serialize(rangerData[5]);
    return data::GrpData::saveData(rangerFile, rangerData);
}

}
