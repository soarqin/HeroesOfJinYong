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

#include "consts.hh"
#include <string>
#include <vector>
#include <cstdint>

namespace hojy::data {

#pragma pack(push, 2)
struct WarFieldInfo {
    std::int16_t id;
    char name[10];
    std::int16_t warFieldId, exp, music;
    std::int16_t ally[TeamMemberCount], autoAlly[TeamMemberCount], allyX[TeamMemberCount], allyY[TeamMemberCount];
    std::int16_t enemy[WarFieldEnemyCount], enemyX[WarFieldEnemyCount], enemyY[WarFieldEnemyCount];
} ATTR_PACKED2;

struct WarFieldLayers {
    std::int16_t layers[data::WarFieldLayerCount][data::WarFieldWidth *data::WarFieldHeight];
} ATTR_PACKED2;
#pragma pack(pop)

class WarFieldData {
public:
    void load(const std::string &warsta, const std::string &warfld);
    [[nodiscard]] const WarFieldInfo *info(std::int16_t id) const;
    [[nodiscard]] const WarFieldLayers *layers(std::int16_t id) const;

private:
    std::vector<WarFieldInfo> info_;
    std::vector<WarFieldLayers> layers_;
};

extern WarFieldData gWarFieldData;

}
