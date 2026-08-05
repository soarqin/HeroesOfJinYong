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
#include "content/grpdata.hh"
#include "content/factors.hh"

#include <cstddef>
#include <new>
#include <utility>

namespace hojy::scene {

Effect gEffect;

bool Effect::load(const std::string &filename) {
    try {
        ::hojy::content::GrpData::DataSet dset;
        if (!::hojy::content::GrpData::loadData(filename, dset)) {
            return false;
        }
        auto effectSz = ::hojy::content::gFactors.effectFrames.size();
        std::vector<std::vector<std::string>> loaded(effectSz);
        size_t index = 0;
        for (size_t i = 0; i < effectSz; ++i) {
            auto &data = loaded[i];
            const auto frameCount = ::hojy::content::gFactors.effectFrames[i];
            if (frameCount < 0
                || static_cast<std::size_t>(frameCount) > dset.size() - index) {
                return false;
            }
            data.assign(dset.begin() + static_cast<std::ptrdiff_t>(index),
                        dset.begin() + static_cast<std::ptrdiff_t>(index + frameCount));
            index += static_cast<std::size_t>(frameCount);
        }
        // The original loader consumed the frame slices and intentionally
        // ignored a trailing IDX/GRP entry.  Some shipped EFT bundles contain
        // exactly one such padding entry, so it is not a startup failure.
        effectTexData_ = std::move(loaded);
        return true;
    } catch (const std::bad_alloc &) {
        return false;
    }
}

const std::vector<std::string> &Effect::operator[](std::int16_t index) const {
    if (index < 0 || index >= effectTexData_.size()) {
        static const std::vector<std::string> dummy;
        return dummy;
    }
    return effectTexData_[index];
}

}
