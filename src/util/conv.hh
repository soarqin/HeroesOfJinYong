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

#include <vector>
#include <string>
#include <string_view>
#include <utility>
#include <cstdint>

namespace hojy::util {

class Conv {
public:
    std::wstring toUnicode(std::string_view str);
    std::string fromUnicode(std::wstring_view wstr);

protected:
    void postInit();

protected:
    std::vector<std::pair<std::uint16_t, std::uint16_t>> table_, tableRev_;
};

class Big5Conv: public Conv {
public:
    Big5Conv();
};

extern Big5Conv big5Conv;

}
