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

#include <random>

#include <cstdint>

namespace hojy::util {

class Random {
public:
    using IntType = std::mt19937_64::result_type;
    using RealType = std::uniform_real_distribution<>::result_type;

    Random() noexcept;
    IntType operator()();
    IntType operator()(IntType modulo);
    IntType operator()(IntType min, IntType max);
    RealType getReal();

private:
    std::mt19937_64 rand_;
    std::uniform_real_distribution<> dist_;
};

extern Random gRandom;

}
