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

#include "serializable.hh"

#include <streambuf>
#include <sstream>

namespace hojy::mem {

struct MemBuf: std::streambuf {
    MemBuf(char* p, size_t size) {
        setg(p, p, p + size);
    }
};

Serializable &Serializable::operator>>(std::vector<std::uint8_t> &data) {
    std::ostringstream stm;
    serialize(stm);
    auto str = stm.str();
    data.assign(str.begin(), str.end());
    return *this;
}

Serializable &Serializable::operator<<(std::vector<std::uint8_t> &data) {
    MemBuf buf(reinterpret_cast<char*>(data.data()), data.size());
    std::istream istm(&buf);
    deserialize(istm);
    return *this;
}

}
