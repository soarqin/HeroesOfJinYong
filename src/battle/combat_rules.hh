#pragma once

#include "battle/random.hh"
#include "world/character.hh"
#include "world/iteminfo.hh"
#include "world/skillinfo.hh"

namespace hojy::battle {

struct DamageResult {
    bool applied = false;
    bool dead = false;
    std::int16_t damage = 0;
    std::int16_t poisoned = 0;
};

struct AiSkillLevels {
    std::int16_t planning = 0;
    std::int16_t execution = 0;
};

bool isDrainSkill(const ::hojy::world::state::SkillData &skill) noexcept;
int attackCount(int doubleAttack) noexcept;
std::int16_t mergeBattleMaxMpGrowth(std::int16_t persistentEntryMaxMp,
                                    std::int16_t battleEntryMaxMp,
                                    std::int16_t battleFinalMaxMp) noexcept;

/* Out-of-range levels are defensively clamped to the skill damage table. */
std::int16_t calcRealAttack(const ::hojy::world::state::CharacterData &character,
                            std::int16_t knowledge,
                            const ::hojy::world::state::SkillData &skill,
                            std::int16_t level,
                            std::int16_t equipmentAttack = 0,
                            std::int16_t skillWeaponBonus = 0);
std::int16_t calcRealDefense(const ::hojy::world::state::CharacterData &character, std::int16_t knowledge);
std::int16_t calcPredictDamage(std::int16_t attack, std::int16_t defence,
                               std::int16_t stamina, std::int16_t hurt,
                               std::int16_t distance);
std::int16_t calcRealSkillLevel(std::int16_t reqMp, std::int16_t level, std::int16_t currentMp);
/* Resolve an already-selected skill even when only level 0 is affordable. */
std::int16_t calcForcedSkillLevel(std::int16_t reqMp, std::int16_t level,
                                  std::int16_t currentMp);
std::int16_t calcRepeatedSkillLevel(std::int16_t reqMp, std::int16_t level,
                                    std::int16_t currentMp);
AiSkillLevels resolveAiSkillLevels(std::int16_t reqMp,
                                   std::int16_t storedSkillLevel,
                                   std::int16_t currentMp) noexcept;
int calcTechniqueRange(int ability);

DamageResult applyDamage(::hojy::world::state::CharacterData &attacker, ::hojy::world::state::CharacterData &defender,
                         std::int16_t attackerKnowledge, std::int16_t defenderKnowledge,
                          int distance, const ::hojy::world::state::SkillData &skill, std::int16_t level,
                          RandomSource &random,
                          std::int16_t equipmentAttack = 0,
                          std::int16_t skillWeaponBonus = 0,
                          std::int16_t storedSkillLevel = -1);

std::int16_t applyPoison(::hojy::world::state::CharacterData &attacker, ::hojy::world::state::CharacterData &defender,
                         std::int16_t stamina);
void finishUtilityAction(::hojy::world::state::CharacterData &actor, std::uint16_t &experience);
std::int16_t applyMedic(::hojy::world::state::CharacterData &attacker, ::hojy::world::state::CharacterData &defender,
                        std::int16_t stamina, RandomSource &random);
std::int16_t applyDepoison(::hojy::world::state::CharacterData &attacker, ::hojy::world::state::CharacterData &defender,
                           std::int16_t stamina, RandomSource &random);
std::int16_t applyThrow(::hojy::world::state::CharacterData &attacker, ::hojy::world::state::CharacterData &defender,
                        const ::hojy::world::state::ItemData &item, std::int16_t stamina,
                        RandomSource &random, bool &dead);
std::int16_t applyPoisonDamage(::hojy::world::state::CharacterData &character);
std::int16_t applyRoundEndDamage(::hojy::world::state::CharacterData &character);
void applyRest(::hojy::world::state::CharacterData &character, RandomSource &random,
               int remainingSteps = -1);

}
