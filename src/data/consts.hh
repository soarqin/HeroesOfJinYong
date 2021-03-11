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

#ifdef __GNUC__
#define ATTR_PACKED __attribute__((packed))
#define ATTR_PACKED2 __attribute__((aligned(2)))
#else
#define ATTR_PACKED
#define ATTR_PACKED2
#endif

namespace hojy::data {

enum {
    /* Base Info */
    TeamMemberCount = 6,
    BagItemCount = 200,

    /* Char Info */
    CharFrameCount = 5,
    LearnSkillCount = 10,
    CarryItemCount = 4,

    SkillLevelMax = 999,
    SkillLevelMaxDiv = 9,

    LevelMax = 30,
    ExpMax = 65000,
    HPMax = 999,
    MPMax = 999,
    StaminaMax = 100,
    PoisonedMax = 100,
    AttackMax = 100,
    DefenceMax = 100,
    SpeedMax = 100,
    MedicMax = 100,
    PoisonMax = 100,
    DepoisonMax = 100,
    AntipoisonMax = 100,
    FistMax = 100,
    SwordMax = 100,
    BladeMax = 100,
    SpecialMax = 100,
    ThrowingMax = 100,
    KnowledgeMax = 100,
    IntegrityMax = 100,
    PoisonAmpMax = 100,
    ReputationMax = 999,
    PotentialMax = 100,

    /* Item Info */
    ItemTexIdStart = 3501,
    MakeItemCount = 5,
    ItemIDMoney = 174,
    ItemIDCompass = 182,

    /* Shop Info */
    ShopItemCount = 5,

    /* Skill Info */
    SkillCheckCount = 10,

    /* Sub Map Info */
    SubMapWidth = 64,
    SubMapHeight = 64,
    SubMapLayerCount = 6,
    SubMapEventCount = 200,

    /* War Field Data */
    WarFieldEnemyCount = 20,
    WarFieldLayerCount = 8,
    WarFieldWidth = 64,
    WarFieldHeight = 64,
};

}
