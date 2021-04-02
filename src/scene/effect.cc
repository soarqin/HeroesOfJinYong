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

#include "effect.hh"

#include "colorpalette.hh"
#include "data/grpdata.hh"
#include "data/factors.hh"
#include "core/config.hh"

namespace hojy::scene {

Effect gEffect;

void Effect::load(Renderer *renderer_, const std::string &filename) {
    data::GrpData::DataSet dset;
    if (!data::GrpData::loadData(filename, dset)) {
        return;
    }
    auto effectSz = data::gFactors.effectFrames.size();
    effectTexData_.resize(effectSz);
    size_t index = 0;
    for (size_t i = 0; i < effectSz; ++i) {
        auto &data = effectTexData_[i];
        size_t sz = data::gFactors.effectFrames[i];
        data.assign(dset.begin() + index, dset.begin() + index + sz);
        index += sz;
    }
}

const std::vector<std::string> &Effect::operator[](std::int16_t index) const {
    if (index < 0 || index >= effectTexData_.size()) {
        static std::vector<std::string> dummy;
        return dummy;
    }
    return effectTexData_[index];
}

}
