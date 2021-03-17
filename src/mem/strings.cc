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

#include "strings.hh"

#include "savedata.hh"
#include "data/warfielddata.hh"
#include "core/config.hh"
#include "util/conv.hh"
#include "util/file.hh"
#include "external/toml.hpp"

namespace hojy::mem {

Strings gStrings;

void Strings::load(const std::string &filename) {
    toml::table tbl;
    try {
        tbl = toml::parse(util::File::getFileContent(core::config.dataFilePath(filename)));
    } catch (const toml::parse_error &err) {
        std::cerr << "Parsing failed: " << err << std::endl;
        return;
    }
    auto arr = tbl["strings"].as_array();
    strings_[Text].reserve(arr->size());
    for (auto &n: *arr) {
        strings_[Text].emplace_back(util::Utf8Conv::toUnicode(n.value_or<std::string>("")));
    }
    if (core::config.simplifiedChinese()) {
        auto backupCharName = strings_[Text][0];
        for (auto &n: strings_[Text]) {
            n = util::trad2SimpConv.convert(n);
        }
        /* allow traditional chinese chars in default user name */
        strings_[Text][0] = backupCharName;
    }
}

void Strings::saveDataLoaded() {
    auto sz = gSaveData.charInfo.size();
    strings_[CharName].resize(sz);
    strings_[NickName].resize(sz);
    for (size_t i = 0; i < sz; ++i) {
        strings_[CharName][i] = util::big5Conv.toUnicode(std::string_view(gSaveData.charInfo[i]->name, 10));
        strings_[NickName][i] = util::big5Conv.toUnicode(std::string_view(gSaveData.charInfo[i]->nick, 10));
    }
    sz = gSaveData.itemInfo.size();
    strings_[ItemName].resize(sz);
    strings_[ItemName2].resize(sz);
    strings_[ItemDesc].resize(sz);
    for (size_t i = 0; i < sz; ++i) {
        strings_[ItemName][i] = util::big5Conv.toUnicode(std::string_view(gSaveData.itemInfo[i]->name, 20));
        strings_[ItemName2][i] = util::big5Conv.toUnicode(std::string_view(gSaveData.itemInfo[i]->name2, 20));
        strings_[ItemDesc][i] = util::big5Conv.toUnicode(std::string_view(gSaveData.itemInfo[i]->desc, 30));
    }
    sz = gSaveData.skillInfo.size();
    strings_[SkillName].resize(sz);
    for (size_t i = 0; i < sz; ++i) {
        strings_[SkillName][i] = util::big5Conv.toUnicode(std::string_view(gSaveData.skillInfo[i]->name, 10));
    }
    sz = gSaveData.subMapInfo.size();
    strings_[SubMapName].resize(sz);
    for (size_t i = 0; i < sz; ++i) {
        strings_[SubMapName][i] = util::big5Conv.toUnicode(std::string_view(gSaveData.subMapInfo[i]->name, 10));
    }
    sz = data::gWarfieldData.size();
    strings_[WarfieldName].resize(sz);
    for (size_t i = 0; i < sz; ++i) {
        strings_[WarfieldName][i] = util::big5Conv.toUnicode(std::string_view(data::gWarfieldData.info(i)->name, 10));
    }
    if (core::config.simplifiedChinese()) {
        std::wstring backupCharName = strings_[CharName][0];
        for (auto t = int(CharName); t < int(StringsMax); ++t) {
            for (auto &n: strings_[t]) {
                n = util::trad2SimpConv.convert(n);
            }
        }
        /* allow traditional chinese chars in user-input name */
        strings_[CharName][0] = backupCharName;
    }
}

}
