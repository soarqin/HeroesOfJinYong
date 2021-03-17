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
Trad2SimpConv trad2SimpConv;

std::wstring Conv::toUnicode(std::string_view str) {
    size_t len = str.length();
    const char *cstr = str.data();
    const char *cstrEnd = cstr + len;
    std::wstring result;
    result.reserve(len);
    while (cstr < cstrEnd) {
        auto c = std::uint8_t(*cstr);
        if (c == 0) { break; }
        if (c < 0x80) {
            result += wchar_t(c);
            ++cstr;
            continue;
        }
        if (cstr + 1 >= cstrEnd) {
            break;
        }
        std::uint32_t charcode = (std::uint16_t(c) << 8) | std::uint8_t(*(cstr + 1));
        auto ite = std::lower_bound(table_.begin(), table_.end(), std::make_pair(charcode, std::uint32_t(0)));
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
        auto c = std::uint32_t(*cstr);
        if (c < 0x80) {
            result += char(c);
            ++cstr;
            continue;
        }
        auto ite = std::lower_bound(tableRev_.begin(), tableRev_.end(), std::make_pair(c, std::uint32_t(0)));
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

Big5Conv::Big5Conv() noexcept {
    table_ =
#include "big5table.inl"
    postInit();
}

std::wstring Utf8Conv::toUnicode(std::string_view str) {
    size_t sz = str.size();
    const auto *n = reinterpret_cast<const uint8_t*>(str.data());
    size_t i = 0;
    std::wstring res;
    while (i < sz) {
        if (n[i] < 0x80) {
            res += wchar_t(n[i]);
            ++i; continue;
        }
        if (n[i] < 0xE0) {
            if (i + 2 > sz) { break; }
            res += wchar_t((std::uint32_t(n[i] & 0x1F) << 6) | std::uint32_t(n[i + 1] & 0x3F));
            i += 2; continue;
        }
        if (n[i] < 0xF0) {
            if (i + 3 > sz) { break; }
            res += wchar_t((std::uint32_t(n[i] & 0x0F) << 12) | ((std::uint32_t(n[i + 1] & 0x3F) << 6))
                           | (std::uint32_t(n[i + 2] & 0x3F)));
            i += 3; continue;
        }
        if (n[i] < 0xF8) {
            if (i + 4 > sz) { break; }
            if constexpr (sizeof(wchar_t) > 2) {
                res += wchar_t((std::uint32_t(n[i] & 0x07) << 18) | ((std::uint32_t(n[i + 1] & 0x3F) << 12))
                               | ((std::uint32_t(n[i + 2] & 0x3F) << 6)) | (std::uint32_t(n[i + 3] & 0x3F)));
            }
            i += 4; continue;
        }
        if (n[i] < 0xFC) {
            if (i + 5 > sz) { break; }
            if constexpr (sizeof(wchar_t) > 2) {
                res += wchar_t((std::uint32_t(n[i] & 0x03) << 24) | ((std::uint32_t(n[i + 1] & 0x3F) << 18))
                               | ((std::uint32_t(n[i + 2] & 0x3F) << 12)) | ((std::uint32_t(n[i + 3] & 0x3F) << 6))
                               | (std::uint32_t(n[i + 4] & 0x3F)));
            }
            i += 5; continue;
        }
        if (i + 6 > sz) { break; }
        if constexpr (sizeof(wchar_t) > 2) {
            res += wchar_t((std::uint32_t(n[i] & 0x01) << 30) | ((std::uint32_t(n[i + 1] & 0x3F) << 24))
                           | ((std::uint32_t(n[i + 2] & 0x3F) << 18)) | ((std::uint32_t(n[i + 3] & 0x3F) << 12))
                           | ((std::uint32_t(n[i + 4] & 0x3F) << 6)) | (std::uint32_t(n[i + 5] & 0x3F)));
        }
        i += 6;
    }
    return res;
}

std::string Utf8Conv::fromUnicode(std::wstring_view wstr) {
    std::string res;
    for (auto &ch: wstr) {
        if (ch < 0x80) {
            res += char(ch);
            continue;
        }
        if (ch < 0x800) {
            res += char(0xC0 | (ch >> 6));
            res += char(0x80 | (ch & 0x3F));
            continue;
        }
        if constexpr (sizeof(wchar_t) > 2) {
            if (ch < 0x10000) {
                res += char(0xE0 | (ch >> 12));
                res += char(0x80 | ((ch >> 6) & 0x3F));
                res += char(0x80 | (ch & 0x3F));
                continue;
            }
            if (ch < 0x200000) {
                res += char(0xF0 | (ch >> 18));
                res += char(0x80 | ((ch >> 12) & 0x3F));
                res += char(0x80 | ((ch >> 6) & 0x3F));
                res += char(0x80 | (ch & 0x3F));
                continue;
            }
            if (ch < 0x4000000) {
                res += char(0xF8 | (ch >> 24));
                res += char(0x80 | ((ch >> 18) & 0x3F));
                res += char(0x80 | ((ch >> 12) & 0x3F));
                res += char(0x80 | ((ch >> 6) & 0x3F));
                res += char(0x80 | (ch & 0x3F));
                continue;
            }
            res += char(0xFC | (ch >> 30));
            res += char(0x80 | ((ch >> 24) & 0x3F));
            res += char(0x80 | ((ch >> 18) & 0x3F));
            res += char(0x80 | ((ch >> 12) & 0x3F));
            res += char(0x80 | ((ch >> 6) & 0x3F));
            res += char(0x80 | (ch & 0x3F));
        }
    }
    return res;
}

Trad2SimpConv::Trad2SimpConv() noexcept {
    charTable_ = {
#include "tschars.inl"
    };
    std::vector<std::pair<std::vector<uint32_t>, std::vector<uint32_t>>> wordTable = {
#include "tswords.inl"
    };
    for (auto &p: wordTable) {
        auto *node = &root_;
        for (auto c: p.first) {
            node = &node->nodes[c];
        }
        node->word = std::move(p.second);
    }
}

std::wstring Trad2SimpConv::convert(const std::wstring &str) {
    std::wstring res;
    size_t sz = str.size();
    res.reserve(sz);
    for (size_t i = 0; i < sz;) {
        {
            auto *node = &root_;
            size_t j = i;
            bool notfound = false;
            while (j < sz) {
                auto c = str[j++];
                auto ite = node->nodes.find(c);
                if (ite == node->nodes.end()) {
                    notfound = true;
                    break;
                }
                node = &ite->second;
            }
            if (!notfound) {
                res.insert(res.end(), node->word.begin(), node->word.end());
                i = j;
                continue;
            }
        }
        auto c = str[i];
        auto ite = charTable_.find(c);
        if (ite == charTable_.end()) res += c;
        else res += wchar_t(ite->second);
        ++i;
    }
    return res;
}

}
