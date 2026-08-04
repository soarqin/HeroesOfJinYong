#include "combat_rules.hh"

#include "formulas.hh"
#include "turn_order.hh"
#include "content/constants.hh"

#include <algorithm>

namespace hojy::battle {

bool isDrainSkill(const ::hojy::world::state::SkillData &skill) noexcept {
    return skill.damageType > 0;
}

int attackCount(int doubleAttack) noexcept {
    return doubleAttack == 1 ? 2 : 1;
}

std::int16_t mergeBattleMaxMpGrowth(std::int16_t persistentEntryMaxMp,
                                    std::int16_t battleEntryMaxMp,
                                    std::int16_t battleFinalMaxMp) noexcept {
    const auto growth = std::max(0, static_cast<int>(battleFinalMaxMp)
                                    - static_cast<int>(battleEntryMaxMp));
    return static_cast<std::int16_t>(std::clamp(
        static_cast<int>(persistentEntryMaxMp) + growth,
        0, static_cast<int>(::hojy::content::MaxMpMax)));
}

std::int16_t calcRealAttack(const ::hojy::world::state::CharacterData &character,
                            std::int16_t knowledge,
                            const ::hojy::world::state::SkillData &skill,
                            std::int16_t level,
                            std::int16_t equipmentAttack,
                            std::int16_t skillWeaponBonus) {
    level = std::clamp<std::int16_t>(level, 0, ::hojy::content::SkillLevelMaxDiv);
    const int attack = character.attack - equipmentAttack;
    return static_cast<std::int16_t>((attack * 3 + skill.damage[level]) / 2
                                     + equipmentAttack + skillWeaponBonus + knowledge * 2);
}

std::int16_t calcRealDefense(const ::hojy::world::state::CharacterData &character, std::int16_t knowledge) {
    return static_cast<std::int16_t>(character.defence + knowledge * 2);
}

std::int16_t calcPredictDamage(std::int16_t attack, std::int16_t defence,
                               std::int16_t stamina, std::int16_t hurt,
                               std::int16_t distance) {
    return static_cast<std::int16_t>(predictDamage(
        DamageInput{attack, defence, stamina, hurt, distance}));
}

std::int16_t calcRealSkillLevel(std::int16_t reqMp, std::int16_t level, std::int16_t currentMp) {
    return static_cast<std::int16_t>(resolveSkillLevel(reqMp, level, currentMp));
}

std::int16_t calcForcedSkillLevel(std::int16_t reqMp, std::int16_t level,
                                  std::int16_t currentMp) {
    return std::max<std::int16_t>(0, calcRealSkillLevel(reqMp, level, currentMp));
}

std::int16_t calcRepeatedSkillLevel(std::int16_t reqMp, std::int16_t level,
                                    std::int16_t currentMp) {
    /* Kept as a compatibility spelling for callers outside the AI path. */
    return calcForcedSkillLevel(reqMp, level, currentMp);
}

AiSkillLevels resolveAiSkillLevels(std::int16_t reqMp,
                                   std::int16_t storedSkillLevel,
                                   std::int16_t currentMp) noexcept {
    const auto planning = std::clamp<std::int16_t>(
        storedSkillLevel / 100, 0, ::hojy::content::SkillLevelMaxDiv);
    return AiSkillLevels{
        planning,
        calcForcedSkillLevel(reqMp, planning, currentMp),
    };
}

int calcTechniqueRange(int ability) {
    return ability / 15 + 1;
}

DamageResult applyDamage(::hojy::world::state::CharacterData &attacker, ::hojy::world::state::CharacterData &defender,
                         std::int16_t attackerKnowledge, std::int16_t defenderKnowledge,
                         int distance, const ::hojy::world::state::SkillData &skill, std::int16_t level,
                         RandomSource &random, std::int16_t equipmentAttack,
                         std::int16_t skillWeaponBonus,
                         std::int16_t storedSkillLevel) {
    DamageResult result;
    level = std::clamp<std::int16_t>(level, 0, ::hojy::content::SkillLevelMaxDiv);
    if (isDrainSkill(skill)) {
        const auto rawLevel = std::clamp<std::int16_t>(
            storedSkillLevel >= 0 ? storedSkillLevel / 100 : level,
            0, ::hojy::content::SkillLevelMaxDiv);
        const auto addMp = skill.addMp[rawLevel];
        /* Z.DAT:0x395EC samples the caster fluctuation before the fixed gain. */
        const auto selfDelta = originalRandom(random, 3) - originalRandom(random, 3);
        attacker.mp = static_cast<std::int16_t>(attacker.mp + addMp);
        attacker.maxMp = std::clamp<std::int16_t>(
            static_cast<std::int16_t>(attacker.maxMp
                + originalRandom(random, addMp / 2)),
            0, ::hojy::content::MaxMpMax);
        attacker.mp = static_cast<std::int16_t>(attacker.mp + selfDelta);
        attacker.mp = std::clamp<std::int16_t>(attacker.mp, 0, attacker.maxMp);
        const auto targetDelta = originalRandom(random, 3) - originalRandom(random, 3);
        const auto oldMp = defender.mp;
        defender.mp = std::max<std::int16_t>(
            0, static_cast<std::int16_t>(defender.mp - skill.drainMp[rawLevel] - targetDelta));
        result.applied = true;
        result.damage = static_cast<std::int16_t>(oldMp - defender.mp);
        result.dead = defender.hp <= 0;
        return result;
    }
    const int attack = calcRealAttack(attacker, attackerKnowledge, skill, level,
                                      equipmentAttack, skillWeaponBonus);
    const int defence = calcRealDefense(defender, defenderKnowledge);
    const int damage = calcDamage(
        DamageInput{attack, defence, attacker.stamina, defender.hurt, distance}, random);
    defender.hp = std::max<std::int16_t>(0, defender.hp - damage);
    defender.hurt = std::clamp<std::int16_t>(
        defender.hurt + damage / 10, 0, ::hojy::content::HurtMax);
    result.applied = true;
    result.damage = static_cast<std::int16_t>(damage);
    const auto rawLevel = std::clamp<std::int16_t>(
        storedSkillLevel >= 0 ? storedSkillLevel / 100 : level,
        0, ::hojy::content::SkillLevelMaxDiv);
    const auto poison = poisonOnHit(attacker.poisonAmp, rawLevel * 100,
                                    skill.addPoison, defender.antipoison);
    const auto oldPoison = defender.poisoned;
    defender.poisoned = std::clamp<std::int16_t>(
        static_cast<std::int16_t>(defender.poisoned + poison), 0, ::hojy::content::PoisonedMax);
    result.poisoned = static_cast<std::int16_t>(defender.poisoned - oldPoison);
    result.dead = defender.hp <= 0;
    return result;
}

std::int16_t applyPoison(::hojy::world::state::CharacterData &attacker, ::hojy::world::state::CharacterData &defender,
                         std::int16_t stamina) {
    const auto potential = poisonAmount(attacker.poison, defender.antipoison);
    const auto applied = std::min<int>(potential,
        std::max(0, ::hojy::content::PoisonedMax - defender.poisoned));
    defender.poisoned = static_cast<std::int16_t>(defender.poisoned + applied);
    if (stamina) {
        attacker.stamina = std::clamp<std::int16_t>(attacker.stamina - stamina, 0, ::hojy::content::StaminaMax);
    }
    return static_cast<std::int16_t>(applied);
}

void finishUtilityAction(::hojy::world::state::CharacterData &actor, std::uint16_t &experience) {
    /* Z.DAT:0x399FE-0x39A33 adds one experience and charges two stamina
     * after poison, depoison and medic, independent of the effect result. */
    ++experience;
    actor.stamina = std::clamp<std::int16_t>(
        static_cast<std::int16_t>(actor.stamina - 2), 0, ::hojy::content::StaminaMax);
}

std::int16_t applyMedic(::hojy::world::state::CharacterData &attacker, ::hojy::world::state::CharacterData &defender,
                        std::int16_t stamina, RandomSource &random) {
    const auto oldHp = defender.hp;
    const auto medic = std::max(0, static_cast<int>(attacker.medic));
    auto heal = medicHeal(medic, defender.hurt, random);
    auto hurtCut = medic;
    if (defender.hurt > medic + 20) { heal = 0; hurtCut = 0; }
    defender.hp = std::clamp<std::int16_t>(
        defender.hp + heal, 0, defender.maxHp);
    defender.hurt = std::clamp<std::int16_t>(
        defender.hurt - hurtCut, 0, ::hojy::content::HurtMax);
    if (stamina) {
        attacker.stamina = std::clamp<std::int16_t>(attacker.stamina - stamina, 0, ::hojy::content::StaminaMax);
    }
    return static_cast<std::int16_t>(defender.hp - oldHp);
}

std::int16_t applyDepoison(::hojy::world::state::CharacterData &attacker, ::hojy::world::state::CharacterData &defender,
                           std::int16_t stamina, RandomSource &random) {
    auto removed = depoisonAmount(attacker.depoison, defender.poisoned, random);
    defender.poisoned = static_cast<std::int16_t>(defender.poisoned - removed);
    if (stamina) {
        attacker.stamina = std::clamp<std::int16_t>(attacker.stamina - stamina, 0, ::hojy::content::StaminaMax);
    }
    return static_cast<std::int16_t>(removed);
}

std::int16_t applyThrow(::hojy::world::state::CharacterData &attacker, ::hojy::world::state::CharacterData &defender,
                        const ::hojy::world::state::ItemData &item, std::int16_t stamina,
                        RandomSource &random, bool &dead) {
    const auto oldHp = defender.hp;
    const auto hpChange = static_cast<std::int16_t>(
        throwDamage(item.addHp, defender.hurt, attacker.throwing, random));
    defender.hp = std::clamp<std::int16_t>(
        defender.hp + hpChange, 0, defender.maxHp);
    defender.hurt = std::clamp<std::int16_t>(
        defender.hurt - hpChange / 4, 0, ::hojy::content::HurtMax);
    const auto poisonChange = throwPoison(
        item.addPoisoned, attacker.throwing, defender.antipoison, random);
    defender.poisoned = std::clamp<std::int16_t>(
        defender.poisoned + poisonChange, 0, ::hojy::content::PoisonedMax);
    if (stamina) {
        attacker.stamina = std::clamp<std::int16_t>(attacker.stamina - stamina, 0, ::hojy::content::StaminaMax);
    }
    dead = defender.hp <= 0;
    return static_cast<std::int16_t>(defender.hp - oldHp);
}

std::int16_t applyPoisonDamage(::hojy::world::state::CharacterData &character) {
    if (character.hp <= 0 || character.poisoned <= 0) { return 0; }
    const auto oldHp = character.hp;
    character.hp = std::clamp<std::int16_t>(character.hp - character.poisoned / 10, 1, character.maxHp);
    return static_cast<std::int16_t>(character.hp - oldHp);
}

std::int16_t applyRoundEndDamage(::hojy::world::state::CharacterData &character) {
    if (character.hp <= 0
        || (character.hurt <= 0
            && !(character.poisoned > 0 && character.stamina > 0))) {
        return 0;
    }
    const auto oldHp = character.hp;
    character.hp = static_cast<std::int16_t>(character.hp
                                             - character.hurt / 20
                                             - character.poisoned / 10);
    if (character.hp < 1) { character.hp = 1; }
    return static_cast<std::int16_t>(character.hp - oldHp);
}

void applyRest(::hojy::world::state::CharacterData &character, RandomSource &random,
               int remainingSteps) {
    const auto initialSteps = calculateMovementSteps(character.speed, character.hurt);
    const bool moved = remainingSteps >= 0 && remainingSteps != initialSteps;
    const auto gain = restGain(character.stamina, moved, random);
    const auto staminaGain = gain.stamina;
    character.stamina = std::clamp<std::int16_t>(
        character.stamina + staminaGain, 0, ::hojy::content::StaminaMax);
    character.hp = std::clamp<std::int16_t>(character.hp + gain.hp, 0, character.maxHp);
    character.mp = std::clamp<std::int16_t>(character.mp + gain.mp, 0, character.maxMp);
}

}
