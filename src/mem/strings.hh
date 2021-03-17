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

namespace hojy::mem {

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

    void load(const std::string &filename);
    void saveDataLoaded();
    const std::wstring &operator()(Type type, std::int16_t index) {
        static const std::wstring empty;
        return index < strings_[type].size() ? strings_[type][index] : empty;
    }

private:
    std::vector<std::wstring> strings_[StringsMax];
};

extern Strings gStrings;
#define GETTEXT(n) ::hojy::mem::gStrings(::hojy::mem::Strings::Text, (n))
#define GETCHARNAME(n) ::hojy::mem::gStrings(::hojy::mem::Strings::CharName, (n))
#define GETNICKNAME(n) ::hojy::mem::gStrings(::hojy::mem::Strings::NickName, (n))
#define GETITEMNAME(n) ::hojy::mem::gStrings(::hojy::mem::Strings::ItemName, (n))
#define GETITEMNAME2(n) ::hojy::mem::gStrings(::hojy::mem::Strings::ItemName2, (n))
#define GETITEMDESC(n) ::hojy::mem::gStrings(::hojy::mem::Strings::ItemDesc, (n))
#define GETSKILLNAME(n) ::hojy::mem::gStrings(::hojy::mem::Strings::SkillName, (n))
#define GETSUBMAPNAME(n) ::hojy::mem::gStrings(::hojy::mem::Strings::SubMapName, (n))
#define GETWARFIELDNAME(n) ::hojy::mem::gStrings(::hojy::mem::Strings::WarfieldName, (n))

}
