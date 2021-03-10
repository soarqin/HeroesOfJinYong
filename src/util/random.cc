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

#include "random.hh"

namespace hojy::util {

Random gRandom;

Random::Random() noexcept: rand_(std::random_device()()), dist_(0., 1.) {

}

Random::IntType Random::operator()() {
    return rand_();
}

Random::IntType Random::operator()(Random::IntType modulo) {
    return rand_() % modulo;
}

Random::IntType Random::operator()(Random::IntType min, Random::IntType max) {
    return rand_() % (max - min + 1) + min;
}

Random::RealType Random::getReal() {
    return dist_(rand_);
}

}
