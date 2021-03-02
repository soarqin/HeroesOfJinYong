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
    MemBuf(const char *p, size_t size) {
        auto *ptr = const_cast<char*>(p);
        setg(ptr, ptr, ptr + size);
    }
};


void Serializable::serialize(std::string &data) {
    std::ostringstream stm;
    *this >> stm;
    data = std::move(stm.str());
}

void Serializable::deserialize(const std::string &data) {
    MemBuf buf(data.data(), data.size());
    std::istream istm(&buf);
    *this << istm;
}

}
