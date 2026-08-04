#pragma once

#include "battle/random.hh"
#include "mem/character.hh"
#include "mem/iteminfo.hh"
#include "mem/skillinfo.hh"

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

bool isDrainSkill(const mem::SkillData &skill) noexcept;
int attackCount(int doubleAttack) noexcept;
std::int16_t mergeBattleMaxMpGrowth(std::int16_t persistentEntryMaxMp,
                                    std::int16_t battleEntryMaxMp,
                                    std::int16_t battleFinalMaxMp) noexcept;

/* Out-of-range levels are defensively clamped to the skill damage table. */
std::int16_t calcRealAttack(const mem::CharacterData &character,
                            std::int16_t knowledge,
                            const mem::SkillData &skill,
                            std::int16_t level,
                            std::int16_t equipmentAttack = 0,
                            std::int16_t skillWeaponBonus = 0);
std::int16_t calcRealDefense(const mem::CharacterData &character, std::int16_t knowledge);
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

DamageResult applyDamage(mem::CharacterData &attacker, mem::CharacterData &defender,
                         std::int16_t attackerKnowledge, std::int16_t defenderKnowledge,
                          int distance, const mem::SkillData &skill, std::int16_t level,
                          RandomSource &random,
                          std::int16_t equipmentAttack = 0,
                          std::int16_t skillWeaponBonus = 0,
                          std::int16_t storedSkillLevel = -1);

std::int16_t applyPoison(mem::CharacterData &attacker, mem::CharacterData &defender,
                         std::int16_t stamina);
void finishUtilityAction(mem::CharacterData &actor, std::uint16_t &experience);
std::int16_t applyMedic(mem::CharacterData &attacker, mem::CharacterData &defender,
                        std::int16_t stamina, RandomSource &random);
std::int16_t applyDepoison(mem::CharacterData &attacker, mem::CharacterData &defender,
                           std::int16_t stamina, RandomSource &random);
std::int16_t applyThrow(mem::CharacterData &attacker, mem::CharacterData &defender,
                        const mem::ItemData &item, std::int16_t stamina,
                        RandomSource &random, bool &dead);
std::int16_t applyPoisonDamage(mem::CharacterData &character);
std::int16_t applyRoundEndDamage(mem::CharacterData &character);
void applyRest(mem::CharacterData &character, RandomSource &random,
               int remainingSteps = -1);

}
