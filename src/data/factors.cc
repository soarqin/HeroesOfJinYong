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

bool Factors::load(const std::string &filename) {
    auto file = util::File::open(core::config.dataFilePath(filename));
    if (!file) { return false; }
    Factors loaded{};
    auto readAt = [&file](std::uint64_t offset, void *data, size_t size) {
        return file.seek(offset) == offset && file.read(data, size) == size;
    };
    if (file.size() == 0x5F000) {
        /* Z.DAT from swimmingfish's FishEdit 0.72 */
        if (!readAt(0x20ce5, loaded.leaveTeamChars.data(), sizeof(std::int16_t) * loaded.leaveTeamChars.size())
            || !readAt(0x25cc6, &loaded.leaveTeamStartEvents, sizeof(std::int16_t))
            || !readAt(0x26d6e, &loaded.initSubMapId, sizeof(std::int16_t))
            || !readAt(0x26db7, &loaded.initSubMapX, sizeof(std::int16_t))
            || !readAt(0x26dc0, &loaded.initSubMapY, sizeof(std::int16_t))
            || !readAt(0x26e2e, &loaded.initMainCharTex, sizeof(std::int16_t))
            || !readAt(0x5b43a, loaded.expForLevelUp.data(), sizeof(std::uint16_t) * loaded.expForLevelUp.size())
            || !readAt(0x5b36c, loaded.effectFrames.data(), sizeof(std::int16_t) * loaded.effectFrames.size())
            || !readAt(0x5b110, loaded.skillWeaponsBindings.data(), sizeof(std::int16_t) * loaded.skillWeaponsBindings.size())) {
            return false;
        }
    } else {
        if (!readAt(0x1a6e5, loaded.leaveTeamChars.data(), sizeof(std::int16_t) * loaded.leaveTeamChars.size())
            || !readAt(0x1f6c6, &loaded.leaveTeamStartEvents, sizeof(std::int16_t))
            || !readAt(0x2076e, &loaded.initSubMapId, sizeof(std::int16_t))
            || !readAt(0x207b7, &loaded.initSubMapX, sizeof(std::int16_t))
            || !readAt(0x207c0, &loaded.initSubMapY, sizeof(std::int16_t))
            || !readAt(0x2082e, &loaded.initMainCharTex, sizeof(std::int16_t))
            || !readAt(0x4df90, loaded.expForLevelUp.data(), sizeof(std::uint16_t) * loaded.expForLevelUp.size())
            || !readAt(0x4f4ce, loaded.effectFrames.data(), sizeof(std::int16_t) * loaded.effectFrames.size())
            || !readAt(0x4f538, loaded.skillWeaponsBindings.data(), sizeof(std::int16_t) * loaded.skillWeaponsBindings.size())) {
            return false;
        }
    }
    *this = loaded;
    return true;
}

}
