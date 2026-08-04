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

#include <cstdint>

namespace hojy::content {

Factors gFactors;

bool Factors::load(const std::string &filename) {
    auto file = util::File::open(core::config.dataFilePath(filename));
    if (!file) { return false; }

    struct Layout {
        std::uint64_t leaveTeamChars;
        std::uint64_t leaveTeamStartEvents;
        std::uint64_t initSubMapId;
        std::uint64_t initSubMapX;
        std::uint64_t initSubMapY;
        std::uint64_t initMainCharTex;
        std::uint64_t expForLevelUp;
        std::uint64_t effectFrames;
        std::uint64_t skillWeaponsBindings;
        std::uint64_t requiredEnd;
    } layout{};
    constexpr std::uint64_t fishEditSize = 0x5F000;
    constexpr Layout fishEditLayout{
        0x20ce5, 0x25cc6, 0x26d6e, 0x26db7, 0x26dc0, 0x26e2e,
        0x5b43a, 0x5b36c, 0x5b110,
        0x5b43a + sizeof(std::uint16_t) * 29,
    };
    constexpr Layout legacyLayout{
        0x1a6e5, 0x1f6c6, 0x2076e, 0x207b7, 0x207c0, 0x2082e,
        0x4df90, 0x4f4ce, 0x4f538,
        0x4f538 + sizeof(std::int16_t) * 21,
    };
    const auto fileSize = file.size();
    if (fileSize == fishEditSize) {
        layout = fishEditLayout;
    } else if (fileSize >= legacyLayout.requiredEnd) {
        layout = legacyLayout;
    } else {
        return false;
    }

    Factors loaded{};
    auto readAt = [&file, fileSize](std::uint64_t offset, void *data, size_t size) {
        if (offset > fileSize || static_cast<std::uint64_t>(size) > fileSize - offset) {
            return false;
        }
        return file.seek(static_cast<std::int64_t>(offset)) == offset
            && file.read(data, size) == size;
    };
    if (!readAt(layout.leaveTeamChars, loaded.leaveTeamChars.data(),
                sizeof(std::int16_t) * loaded.leaveTeamChars.size())
        || !readAt(layout.leaveTeamStartEvents, &loaded.leaveTeamStartEvents, sizeof(std::int16_t))
        || !readAt(layout.initSubMapId, &loaded.initSubMapId, sizeof(std::int16_t))
        || !readAt(layout.initSubMapX, &loaded.initSubMapX, sizeof(std::int16_t))
        || !readAt(layout.initSubMapY, &loaded.initSubMapY, sizeof(std::int16_t))
        || !readAt(layout.initMainCharTex, &loaded.initMainCharTex, sizeof(std::int16_t))
        || !readAt(layout.expForLevelUp, loaded.expForLevelUp.data(),
                   sizeof(std::uint16_t) * loaded.expForLevelUp.size())
        || !readAt(layout.effectFrames, loaded.effectFrames.data(),
                   sizeof(std::int16_t) * loaded.effectFrames.size())
        || !readAt(layout.skillWeaponsBindings, loaded.skillWeaponsBindings.data(),
                   sizeof(std::int16_t) * loaded.skillWeaponsBindings.size())) {
        return false;
    }
    *this = loaded;
    return true;
}

}
