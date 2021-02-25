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

#include "conv.hh"

#include <algorithm>

namespace hojy::util {

Big5Conv big5Conv;

std::wstring Conv::toUnicode(std::string_view str) {
    size_t len = str.length();
    const char *cstr = str.data();
    const char *cstrEnd = cstr + len;
    std::wstring result;
    result.reserve(len);
    while (cstr < cstrEnd) {
        auto c = std::uint8_t(*cstr);
        if (c < 0x80) {
            result += wchar_t(c);
            ++cstr;
            continue;
        }
        if (cstr + 1 >= cstrEnd) {
            break;
        }
        std::uint16_t charcode = (std::uint16_t(c) << 8) | std::uint8_t(*(cstr + 1));
        auto ite = std::lower_bound(table_.begin(), table_.end(), std::make_pair(charcode, std::uint16_t(0)));
        if (ite == table_.end() || ite->first != charcode) {
            result.append(L"  ");
        } else {
            result += wchar_t(ite->second);
        }
        cstr += 2;
    }
    return result;
}

std::string Conv::fromUnicode(std::wstring_view wstr) {
    size_t len = wstr.length();
    const wchar_t *cstr = wstr.data();
    const wchar_t *cstrEnd = cstr + len;
    std::string result;
    result.reserve(len * 2);
    while (cstr < cstrEnd) {
        auto c = std::uint16_t(*cstr);
        if (c < 0x80) {
            result += char(c);
            ++cstr;
            continue;
        }
        auto ite = std::lower_bound(tableRev_.begin(), tableRev_.end(), std::make_pair(c, std::uint16_t(0)));
        if (ite == tableRev_.end() || ite->first != c) {
            result.append("  ");
        } else {
            result += char(ite->second >> 8);
            result += char(ite->second & 0xFF);
        }
        ++cstr;
    }
    return result;
}

void Conv::postInit() {
    auto size = table_.size();
    tableRev_.resize(size);
    for (size_t i = 0; i < size; ++i) {
        auto &p = table_[i];
        tableRev_[i] = std::make_pair(p.second, p.first);
    }
    std::sort(table_.begin(), table_.end());
    std::sort(tableRev_.begin(), tableRev_.end());
}

Big5Conv::Big5Conv() {
    table_ =
#include "big5table.inl"
    postInit();
}

}