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

struct WarfieldInfo {
    std::int16_t id;
    char name[10];
    std::int16_t warFieldId;
    std::uint16_t exp;
    std::int16_t music;
    std::int16_t defaultMembers[TeamMemberCount], forceMembers[TeamMemberCount], memberX[TeamMemberCount], memberY[TeamMemberCount];
    std::int16_t enemy[WarFieldEnemyCount], enemyX[WarFieldEnemyCount], enemyY[WarFieldEnemyCount];
};

struct WarfieldLayers {
    std::int16_t layers[data::WarFieldLayerCount][data::WarFieldWidth *data::WarFieldHeight];
};

class WarfieldData {
public:
    void load(const std::string &warsta, const std::string &warfld);
    [[nodiscard]] size_t size() const { return info_.size(); }
    [[nodiscard]] const WarfieldInfo *info(std::int16_t id) const;
    [[nodiscard]] const WarfieldLayers *layers(std::int16_t id) const;

private:
    std::vector<WarfieldInfo> info_;
    std::vector<WarfieldLayers> layers_;
};

extern WarfieldData gWarfieldData;

}
