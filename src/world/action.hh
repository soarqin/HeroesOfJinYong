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

#include "character_style.hh"

#include "character.hh"
#include "iteminfo.hh"
#include "skillinfo.hh"

#include <string>
#include <tuple>
#include <map>
#include <cstdint>

namespace hojy::battle {
class RandomSource;
}

namespace hojy::world::state {

class Bag;
class SaveData;

enum class PropType {
    Hp = 0, MaxHp, Poisoned, Stamina, MpType, Mp, MaxMp,
    Attack, Speed, Defence, Medic, Poison, Depoison, Antipoison,
    Fist, Sword, Blade, Special, Throwing, Knowledge, Integrity, DoubleAttack, PoisonAmp,
};
const std::wstring &propToName(PropType type);
void addUpPropFromEquipToChar(CharacterData *info);
void addUpPropFromEquipToChar(const ItemInfo &items, CharacterData *info);
std::uint16_t getExpForLevelUp(std::int16_t level);
std::uint16_t getExpForSkillLearn(std::int16_t itemId, std::int16_t level, std::int16_t potential);
std::uint16_t getExpForSkillLearn(const ItemInfo &items, std::int16_t itemId,
                                 std::int16_t level, std::int16_t potential);
std::uint16_t getExpForMakeItem(std::int16_t itemId, std::int16_t potential);
bool leaveTeam(std::int16_t id);
bool leaveTeam(SaveData &saveData, std::int16_t id);
bool skillFull(std::int16_t charId);
bool skillFull(const SaveData &saveData, std::int16_t charId);
bool equipItem(std::int16_t charId, std::int16_t itemId);
bool equipItem(SaveData &saveData, std::int16_t charId, std::int16_t itemId);
bool useItem(CharacterData *charInfo, std::int16_t itemId, std::map<PropType, std::int16_t> &changes);
bool useItem(Bag &bag, CharacterData *charInfo, std::int16_t itemId,
             std::map<PropType, std::int16_t> &changes);
bool useItem(const ItemInfo &items, Bag &bag, CharacterData *charInfo,
             std::int16_t itemId,
             std::map<PropType, std::int16_t> &changes);
std::int16_t tryUseBagItem(CharacterData *charInfo, PropType type, std::int16_t value);
std::int16_t tryUseBagItem(const Bag &bag, CharacterData *charInfo,
                           PropType type, std::int16_t value);
/* Removes one carried item without applying its consumable effects. */
bool consumeNpcItem(CharacterData *charInfo, std::int16_t itemId);
std::int16_t findNpcItemSlot(const CharacterData *charInfo,
                             std::int16_t itemId);
/* Removes one item from an explicitly selected carry slot. */
bool consumeNpcItemAt(CharacterData *charInfo, int slot,
                      std::int16_t expectedItemId = -1);
bool useNpcItem(CharacterData *charInfo, std::int16_t itemId, std::map<PropType, std::int16_t> &changes);
std::int16_t tryUseNpcItem(CharacterData *charInfo, PropType type, std::int16_t value);
bool applyItemChanges(CharacterData *charInfo, const ItemData *itemInfo, std::map<PropType, std::int16_t> &changes);
/*
 * Property gains a skill book grants when its training completes. This is a
 * different rule set from applyItemChanges: it never touches the consumable
 * fields and has its own mp-type and double-attack rules.
 */
void applyBookChanges(CharacterData *charInfo, const ItemData *itemInfo);
bool canUseItem(const CharacterData *charInfo, const ItemData *itemInfo);
std::int16_t getLeaveEventId(std::int16_t id);
std::int16_t calcRealAttack(const CharacterData *c, std::int16_t knowledge, const SkillData *skill, std::int16_t level);
std::int16_t calcRealDefense(const CharacterData *c, std::int16_t knowledge);
std::int16_t calcPredictDamage(std::int16_t atk, std::int16_t def, std::int16_t stamina, std::int16_t hurt, std::int16_t distance);
std::int16_t calcRealSkillLevel(std::int16_t reqMp, std::int16_t level, std::int16_t currMp);
std::int16_t calcSkillMpCost(const SkillData *skill, std::int16_t level);
/*
 * knowledgeSelf and knowledgeOther are relative to the attacker, not to a
 * fixed side. exp receives the experience earned for this hit.
 */
bool actDamage(CharacterData *c1, CharacterData *c2, std::int16_t knowledgeSelf, std::int16_t knowledgeOther,
                int distance, int index, int level, std::int16_t &damage, std::int16_t &poisoned,
                std::int16_t &exp, bool &dead);
bool actDamage(CharacterData *c1, CharacterData *c2, std::int16_t knowledgeSelf, std::int16_t knowledgeOther,
               int distance, int index, int level, std::int16_t &damage, std::int16_t &poisoned,
               std::int16_t &exp, bool &dead, ::hojy::battle::RandomSource &random);
/* Charges the skill MP cost once per attack, not once per target. */
void postDamage(CharacterData *c, int index, int level, std::int16_t stamina, bool &levelup);
void postDamage(CharacterData *c, int index, int level, std::int16_t stamina,
                bool &levelup, ::hojy::battle::RandomSource &random);
std::int16_t actPoison(CharacterData *c1, CharacterData *c2, std::int16_t stamina);
std::int16_t actMedic(CharacterData *c1, CharacterData *c2, std::int16_t stamina);
std::int16_t actMedic(CharacterData *c1, CharacterData *c2, std::int16_t stamina,
                      ::hojy::battle::RandomSource &random);
std::int16_t actDepoison(CharacterData *c1, CharacterData *c2, std::int16_t stamina);
std::int16_t actDepoison(CharacterData *c1, CharacterData *c2, std::int16_t stamina,
                         ::hojy::battle::RandomSource &random);
std::int16_t actThrow(CharacterData *c1, CharacterData *c2, std::int16_t itemId, std::int16_t stamina, bool &dead);
std::int16_t actThrow(CharacterData *c1, CharacterData *c2, std::int16_t itemId,
                      std::int16_t stamina, bool &dead,
                      ::hojy::battle::RandomSource &random);
/* Applied to every participant at the end of a round. */
std::int16_t actRoundEndDrain(CharacterData *c, bool inactive);
void actRest(CharacterData *c, bool moved);
void actRest(CharacterData *c, bool moved, ::hojy::battle::RandomSource &random);
/* Applies every level gained at once, as the original does. */
void actLevelup(CharacterData *c, int gainedLevels);
void actLevelup(CharacterData *c, int gainedLevels,
                ::hojy::battle::RandomSource &random);

}
