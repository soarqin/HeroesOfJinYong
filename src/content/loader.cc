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

#include "loader.hh"

#include "event.hh"
#include "factors.hh"
#include "warfielddata.hh"

#include <utility>

namespace hojy::content {

bool loadData() {
    Factors factors;
    Event events;
    WarfieldData warfields;
    if (!factors.load("Z.DAT")
        || !events.load("KDEF", "TALK")
        || !warfields.load("WAR.STA", "WARFLD")) {
        return false;
    }
    gFactors = std::move(factors);
    gEvent = std::move(events);
    gWarfieldData = std::move(warfields);
    return true;
}

}
