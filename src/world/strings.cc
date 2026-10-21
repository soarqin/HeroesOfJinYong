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
#include "content/warfielddata.hh"
#include "core/config.hh"
#include "util/conv.hh"
#include "util/file.hh"
#include <external/toml.hpp>

#include <utility>

namespace hojy::world::state {

namespace {

constexpr size_t RequiredTextCount = 138;

}

Strings gStrings;

bool Strings::load(const std::string &filename) {
    toml::table tbl;
    const auto content = util::File::getFileContent(core::config.dataFilePath(filename));
    try {
        tbl = toml::parse(content);
    } catch (const toml::parse_error &err) {
        std::cerr << "Parsing failed: " << err << std::endl;
        return false;
    }
    auto arr = tbl["strings"].as_array();
    if (!arr || arr->size() < RequiredTextCount) { return false; }
    std::vector<std::wstring> strings;
    strings.reserve(arr->size());
    for (auto &n: *arr) {
        strings.emplace_back(util::Utf8Conv::toUnicode(n.value_or<std::string>("")));
    }
    if (core::config.simplifiedChinese()) {
        auto backupCharName = strings[0];
        for (auto &n: strings) {
            n = util::trad2SimpConv.convert(n);
        }
        /* allow traditional chinese chars in default user name */
        strings[0] = backupCharName;
    }
    strings_[Text] = std::move(strings);
    return true;
}

void Strings::saveDataLoaded() {
    Strings candidate;
    if (buildForSave(gSaveData, candidate)) {
        swap(candidate);
    }
}

bool Strings::buildForSave(
        const SaveData &saveData, Strings &output) const noexcept {
    try {
        Strings candidate = *this;
        auto sz = saveData.charInfo.size();
        candidate.strings_[CharName].resize(sz);
        candidate.strings_[NickName].resize(sz);
        for (size_t i = 0; i < sz; ++i) {
            const auto *character = saveData.charInfo[i];
            if (!character) { return false; }
            candidate.strings_[CharName][i] = util::big5Conv.toUnicode(
                std::string_view(character->name, 10));
            candidate.strings_[NickName][i] = util::big5Conv.toUnicode(
                std::string_view(character->nick, 10));
        }
        sz = saveData.itemInfo.size();
        candidate.strings_[ItemName].resize(sz);
        candidate.strings_[ItemName2].resize(sz);
        candidate.strings_[ItemDesc].resize(sz);
        for (size_t i = 0; i < sz; ++i) {
            const auto *item = saveData.itemInfo[i];
            if (!item) { return false; }
            candidate.strings_[ItemName][i] = util::big5Conv.toUnicode(
                std::string_view(item->name, 20));
            candidate.strings_[ItemName2][i] = util::big5Conv.toUnicode(
                std::string_view(item->name2, 20));
            candidate.strings_[ItemDesc][i] = util::big5Conv.toUnicode(
                std::string_view(item->desc, 30));
        }
        sz = saveData.skillInfo.size();
        candidate.strings_[SkillName].resize(sz);
        for (size_t i = 0; i < sz; ++i) {
            const auto *skill = saveData.skillInfo[i];
            if (!skill) { return false; }
            candidate.strings_[SkillName][i] = util::big5Conv.toUnicode(
                std::string_view(skill->name, 10));
        }
        sz = saveData.subMapInfo.size();
        candidate.strings_[SubMapName].resize(sz);
        for (size_t i = 0; i < sz; ++i) {
            const auto *subMap = saveData.subMapInfo[i];
            if (!subMap) { return false; }
            candidate.strings_[SubMapName][i] = util::big5Conv.toUnicode(
                std::string_view(subMap->name, 10));
        }
        sz = ::hojy::content::gWarfieldData.size();
        candidate.strings_[WarfieldName].resize(sz);
        for (size_t i = 0; i < sz; ++i) {
            const auto *warfield = ::hojy::content::gWarfieldData.info(i);
            if (!warfield) { return false; }
            candidate.strings_[WarfieldName][i] = util::big5Conv.toUnicode(
                std::string_view(warfield->name, 10));
        }
        if (core::config.simplifiedChinese()
            && !candidate.strings_[CharName].empty()) {
            std::wstring backupCharName = candidate.strings_[CharName][0];
            for (auto t = int(CharName); t < int(StringsMax); ++t) {
                for (auto &value: candidate.strings_[t]) {
                    value = util::trad2SimpConv.convert(value);
                }
            }
            candidate.strings_[CharName][0] = std::move(backupCharName);
        }
        output.swap(candidate);
        return true;
    } catch (...) {
        return false;
    }
}

void Strings::swap(Strings &other) noexcept {
    for (int type = 0; type < StringsMax; ++type) {
        strings_[type].swap(other.strings_[type]);
    }
}

}
