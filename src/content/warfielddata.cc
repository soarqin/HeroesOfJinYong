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
#include <cstring>
#include <limits>
#include <new>
#include <utility>

namespace hojy::content {

WarfieldData gWarfieldData;

bool WarfieldData::load(const std::string &warsta, const std::string &warfld) {
    auto file = util::File::open(core::config.dataFilePath(warsta));
    if (!file) { return false; }
    constexpr std::size_t maxRecords =
        static_cast<std::size_t>(std::numeric_limits<std::int16_t>::max()) + 1;
    const auto fileSize = file.size();
    if (fileSize == 0 || fileSize % sizeof(WarfieldInfo) != 0
        || fileSize / sizeof(WarfieldInfo) > maxRecords
        || fileSize / sizeof(WarfieldInfo) > std::vector<WarfieldInfo>().max_size()) {
        return false;
    }
    const auto infoCount = static_cast<std::size_t>(fileSize / sizeof(WarfieldInfo));
    try {
        std::vector<WarfieldInfo> info(infoCount);
        const auto infoBytes = info.size() * sizeof(WarfieldInfo);
        if (file.read(info.data(), infoBytes) != infoBytes) {
            return false;
        }
        GrpData::DataSet dset;
        if (!GrpData::loadData(warfld, dset)) { return false; }
        if (dset.size() > maxRecords
            || dset.size() > std::vector<WarfieldLayers>().max_size()) {
            return false;
        }
        std::vector<WarfieldLayers> layers(dset.size());
        constexpr std::size_t layerSize = sizeof(std::int16_t)
            * ::hojy::content::WarFieldWidth * ::hojy::content::WarFieldHeight;
        for (std::size_t i = 0; i < dset.size(); ++i) {
            if (dset[i].size() > sizeof(WarfieldLayers::layers)
                || dset[i].size() % layerSize != 0) {
                return false;
            }
            if (!dset[i].empty()) {
                std::memcpy(layers[i].layers, dset[i].data(), dset[i].size());
            }
        }
        for (const auto &warfield: info) {
            if (warfield.warFieldId < 0
                || static_cast<std::size_t>(warfield.warFieldId) >= layers.size()) {
                return false;
            }
            if (dset[warfield.warFieldId].empty()) { return false; }
        }
        info_ = std::move(info);
        layers_ = std::move(layers);
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

const WarfieldInfo *WarfieldData::info(std::int16_t id) const {
    if (id < 0 || static_cast<std::size_t>(id) >= info_.size()) {
        return nullptr;
    }
    return &info_[id];
}

const WarfieldLayers *WarfieldData::layers(std::int16_t id) const {
    if (id < 0 || static_cast<std::size_t>(id) >= layers_.size()) {
        return nullptr;
    }
    return &layers_[id];
}

}
