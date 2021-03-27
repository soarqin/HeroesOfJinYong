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

#include "math.hh"

#include <numeric>
#include <cmath>

namespace hojy::util {

std::pair<int, int> calcSmallestDivision(double x) {
    double m, n, p, q;
    double j = std::floor(x);
    m = 1;
    n = 0;
    p = j;
    q = 1;
    while (x - j > 0.001) {
        x = 1 / (x - j);
        j = std::floor(x);
        auto u = p, v = q;
        p = p * j + m;
        q = q * j + n;
        m = u; n = v;
    }
    return std::make_pair(int(p), int(q));
}

std::pair<int, int> calcSmallestDivision(int a, int b) {
    auto c = std::gcd(a, b);
    return std::make_pair(a / c, b / c);
}

}
