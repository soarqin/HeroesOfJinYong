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

#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace hojy::world::state {

class SaveData;

class Strings {
public:
    enum Type {
        Text = 0,
        CharName,
        NickName,
        ItemName,
        ItemName2,
        ItemDesc,
        SkillName,
        SubMapName,
        WarfieldName,
        StringsMax,
    };

    [[nodiscard]] bool load(const std::string &filename);
    [[nodiscard]] bool buildForSave(
        const SaveData &saveData, Strings &output) const noexcept;
    void saveDataLoaded();
    void swap(Strings &other) noexcept;
    const std::wstring &operator()(Type type, std::int16_t index) const {
        static const std::wstring empty;
        return index >= 0 && index < strings_[type].size()
            ? strings_[type][index] : empty;
    }

private:
    std::vector<std::wstring> strings_[StringsMax];
};

extern Strings gStrings;
#define GETTEXT(n) ::hojy::world::state::gStrings(::hojy::world::state::Strings::Text, (n))
#define GETCHARNAME(n) ::hojy::world::state::gStrings(::hojy::world::state::Strings::CharName, (n))
#define GETNICKNAME(n) ::hojy::world::state::gStrings(::hojy::world::state::Strings::NickName, (n))
#define GETITEMNAME(n) ::hojy::world::state::gStrings(::hojy::world::state::Strings::ItemName, (n))
#define GETITEMNAME2(n) ::hojy::world::state::gStrings(::hojy::world::state::Strings::ItemName2, (n))
#define GETITEMDESC(n) ::hojy::world::state::gStrings(::hojy::world::state::Strings::ItemDesc, (n))
#define GETSKILLNAME(n) ::hojy::world::state::gStrings(::hojy::world::state::Strings::SkillName, (n))
#define GETSUBMAPNAME(n) ::hojy::world::state::gStrings(::hojy::world::state::Strings::SubMapName, (n))
#define GETWARFIELDNAME(n) ::hojy::world::state::gStrings(::hojy::world::state::Strings::WarfieldName, (n))

}
