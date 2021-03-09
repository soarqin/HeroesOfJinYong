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

#include "factors.hh"

#include "util/file.hh"
#include "core/config.hh"

namespace hojy::data {

Factors gFactors;

void Factors::load(const std::string &filename) {
    auto file = util::File::open(core::config.dataFilePath(filename));
    if (!file) { return; }
    if (file.size() == 0x5F000) {
        /* Z.DAT from swimmingfish's FishEdit 0.72 */
        file.seek(0x20ce5);
        file.read(leaveTeamChars.data(), sizeof(std::int16_t) * leaveTeamChars.size());
        file.seek(0x25cc6);
        file.read(&leaveTeamStartEvents, sizeof(std::int16_t));
        file.seek(0x26d6e);
        file.read(&initSubMapId, sizeof(std::int16_t));
        file.seek(0x26db7);
        file.read(&initSubMapX, sizeof(std::int16_t));
        file.seek(0x26dc0);
        file.read(&initSubMapY, sizeof(std::int16_t));
        file.seek(0x26e2e);
        file.read(&initMainCharTex, sizeof(std::int16_t));
        file.seek(0x5b43a);
        file.read(expForLevelUp.data(), sizeof(std::uint16_t) * expForLevelUp.size());
        file.seek(0x5b36c);
        file.read(effectFrames.data(), sizeof(std::int16_t) * effectFrames.size());
        file.seek(0x5b110);
        file.read(skillWeaponsBindings.data(), sizeof(std::int16_t) * skillWeaponsBindings.size());
    } else {
        file.seek(0x1a6e5);
        file.read(leaveTeamChars.data(), sizeof(std::int16_t) * leaveTeamChars.size());
        file.seek(0x1f6c6);
        file.read(&leaveTeamStartEvents, sizeof(std::int16_t));
        file.seek(0x2076e);
        file.read(&initSubMapId, sizeof(std::int16_t));
        file.seek(0x207b7);
        file.read(&initSubMapX, sizeof(std::int16_t));
        file.seek(0x207c0);
        file.read(&initSubMapY, sizeof(std::int16_t));
        file.seek(0x2082e);
        file.read(&initMainCharTex, sizeof(std::int16_t));
        file.seek(0x4df90);
        file.read(expForLevelUp.data(), sizeof(std::uint16_t) * expForLevelUp.size());
        file.seek(0x4f4ce);
        file.read(effectFrames.data(), sizeof(std::int16_t) * effectFrames.size());
        file.seek(0x4f538);
        file.read(skillWeaponsBindings.data(), sizeof(std::int16_t) * skillWeaponsBindings.size());
    }
}

}
