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

#include <iostream>

#ifdef __GNUC__
#define ATTR_PACKED __attribute__((packed))
#else
#define ATTR_PACKED
#endif

namespace hojy::mem {

class Serializable {
public:
    virtual ~Serializable() = default;
    Serializable &operator>>(std::ostream&);
    Serializable &operator<<(std::istream&);

protected:
    virtual void serialize(std::ostream&) {}
    virtual void deserialize(std::istream&) {}
};

}
