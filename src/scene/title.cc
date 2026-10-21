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

#include "title.hh"

#include "colorpalette.hh"
#include "content/grpdata.hh"
#include "core/config.hh"
#include "util/file.hh"

#include <limits>
#include <vector>

namespace hojy::scene {

Title::~Title() {
    delete big_;
}

bool Title::init() {
    if (!renderer_) { return false; }

    TextureMgr candidateTextureMgr;
    candidateTextureMgr.setPalette(gNormalPalette);
    candidateTextureMgr.setRenderer(renderer_);

    renderer_->enableLinear(true);
    auto *candidateBig = Texture::loadFromRAW(
        renderer_,
        util::File::getFileContent(core::config.dataFilePath("TITLE.BIG")),
        320, 200, gNormalPalette);
    renderer_->enableLinear(false);
    if (!candidateBig) { return false; }

    std::vector<std::string> dset;
    if (!::hojy::content::GrpData::loadData("TITLE", dset)) {
        delete candidateBig;
        return false;
    }
    for (std::size_t id = 0; id < dset.size(); ++id) {
        if (id > static_cast<std::size_t>(std::numeric_limits<std::int16_t>::max())
            || !candidateTextureMgr.loadFromRLE(
                dset[id], static_cast<std::int16_t>(id))) {
            delete candidateBig;
            return false;
        }
    }
    for (const auto id: {0, 1, 2, 3, 4, 5, 6, 7}) {
        if (!candidateTextureMgr[id]) {
            delete candidateBig;
            return false;
        }
    }

    titleTextureMgr_.swap(candidateTextureMgr);
    delete big_;
    big_ = candidateBig;
    requestPresentationRefresh();
    return true;
}

}
