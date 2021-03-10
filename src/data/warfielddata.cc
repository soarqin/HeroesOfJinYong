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

#include "warfielddata.hh"

#include "grpdata.hh"
#include "core/config.hh"
#include "util/file.hh"

namespace hojy::data {

WarFieldData gWarFieldData;

void WarFieldData::load(const std::string &warsta, const std::string &warfld) {
    util::File::getFileContent(core::config.dataFilePath(warsta), info_);
    GrpData::DataSet dset;
    if (GrpData::loadData(warfld, dset)) {
        layers_.resize(dset.size());
        for (size_t i = 0; i < dset.size(); ++i) {
            auto sz = std::min(sizeof(layers_[i].layers), dset[i].size());
            memcpy(layers_[i].layers, dset[i].data(), sz);
        }
    }
}

const WarFieldInfo *WarFieldData::info(std::int16_t id) const {
    if (id < 0 || id >= info_.size()) {
        return nullptr;
    }
    return &info_[id];
}

const WarFieldLayers *WarFieldData::layers(std::int16_t id) const {
    if (id < 0 || id >= layers_.size()) {
        return nullptr;
    }
    return &layers_[id];
}

}
