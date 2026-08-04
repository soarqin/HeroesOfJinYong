#include "battle/combat_rules.hh"
#include "battle/random.hh"
#include "data/consts.hh"
#include "test_support.hh"

#include <iostream>

namespace {

hojy::mem::CharacterData makeCharacter() {
    hojy::mem::CharacterData c{};
    c.maxHp = c.hp = 100;
    c.maxMp = c.mp = 100;
    c.stamina = 100;
    c.attack = 60;
    c.defence = 10;
    c.hurt = 0;
    return c;
}

hojy::mem::SkillData makeSkill() {
    hojy::mem::SkillData skill{};
    skill.id = 1;
    skill.damageType = 0;
    skill.reqMp = 10;
    skill.damage[0] = 40;
    return skill;
}

void testSkillLevelDowngrade() {
    HOJY_CHECK_EQ(hojy::data::SkillLevelMax, 900);
    HOJY_CHECK_EQ(hojy::data::SkillLevelStoreMax, 999);
    HOJY_CHECK_EQ(hojy::data::ExpMax, 60000);
    HOJY_CHECK_EQ(hojy::battle::calcRealSkillLevel(10, 5, 24), 4);
    HOJY_CHECK_EQ(hojy::battle::calcRealSkillLevel(10, 5, 9), -1);
    HOJY_CHECK_EQ(hojy::battle::calcRealSkillLevel(10, 0, 0), -1);
    HOJY_CHECK_EQ(hojy::battle::calcRealSkillLevel(0, 5, 0), 5);
}

void testRepeatedSkillContinuesAtMinimumLevelWhenMpIsExhausted() {
    HOJY_CHECK_EQ(hojy::battle::calcRepeatedSkillLevel(10, 5, 0), 0);
    HOJY_CHECK_EQ(hojy::battle::calcRepeatedSkillLevel(10, 5, 24), 4);
}

void testForcedSkillLevelSharesRepeatSemanticsForAiSelection() {
    HOJY_CHECK_EQ(hojy::battle::calcForcedSkillLevel(10, 5, 0), 0);
    HOJY_CHECK_EQ(hojy::battle::calcForcedSkillLevel(10, 5, 9), 0);
    HOJY_CHECK_EQ(hojy::battle::calcForcedSkillLevel(10, 5, 24), 4);
}

void testAiSkillPlanningKeepsStoredRangeWhileExecutionDowngrades() {
    const auto levels = hojy::battle::resolveAiSkillLevels(10, 500, 9);
    HOJY_CHECK_EQ(levels.planning, 5);
    HOJY_CHECK_EQ(levels.execution, 0);

    const auto affordable = hojy::battle::resolveAiSkillLevels(10, 500, 24);
    HOJY_CHECK_EQ(affordable.planning, 5);
    HOJY_CHECK_EQ(affordable.execution, 4);
}

void testTechniqueRangeUsesOriginalInclusiveBaseCell() {
    HOJY_CHECK_EQ(hojy::battle::calcTechniqueRange(0), 1);
    HOJY_CHECK_EQ(hojy::battle::calcTechniqueRange(14), 1);
    HOJY_CHECK_EQ(hojy::battle::calcTechniqueRange(15), 2);
    HOJY_CHECK_EQ(hojy::battle::calcTechniqueRange(60), 5);
}

void testDrainPopupClassificationUsesSkillTypeNotSignedDamage() {
    auto skill = makeSkill();
    HOJY_CHECK_EQ(hojy::battle::isDrainSkill(skill), false);
    skill.damageType = 1;
    HOJY_CHECK_EQ(hojy::battle::isDrainSkill(skill), true);
}

void testBattleMaxMpMergePersistsOnlyGrowthBeyondEntryBonus() {
    HOJY_CHECK_EQ(hojy::battle::mergeBattleMaxMpGrowth(100, 120, 128), 108);
    HOJY_CHECK_EQ(hojy::battle::mergeBattleMaxMpGrowth(100, 120, 110), 100);
    HOJY_CHECK_EQ(hojy::battle::mergeBattleMaxMpGrowth(32760, 32760, 32767),
                  hojy::data::MaxMpMax);
}

void testDoubleAttackRequiresExactOriginalFlag() {
    HOJY_CHECK_EQ(hojy::battle::attackCount(0), 1);
    HOJY_CHECK_EQ(hojy::battle::attackCount(1), 2);
    HOJY_CHECK_EQ(hojy::battle::attackCount(2), 1);
    HOJY_CHECK_EQ(hojy::battle::attackCount(-1), 1);
}

void testRealAttackClampsSkillLevelBeforeDamageLookup() {
    auto character = makeCharacter();
    auto skill = makeSkill();
    skill.addPoison = 567;
    skill.damage[9] = 220;
    skill.selRange[0] = 1234;

    HOJY_CHECK_EQ(hojy::battle::calcRealAttack(
                      character, 0, skill, -1, 0, 0), 110);
    HOJY_CHECK_EQ(hojy::battle::calcRealAttack(
                      character, 0, skill, 10, 0, 0), 200);
}

void testDamageUsesDeterministicRandomOrder() {
    auto attacker = makeCharacter();
    auto defender = makeCharacter();
    auto skill = makeSkill();
    hojy::battle::SequenceRandom random({19, 0});

    const auto result = hojy::battle::applyDamage(
        attacker, defender, 0, 0, 1, skill, 0, random);

    HOJY_CHECK_EQ(result.applied, true);
    HOJY_CHECK_EQ(result.dead, false);
    HOJY_CHECK_EQ(result.damage, 78);
    HOJY_CHECK_EQ(defender.hp, 22);
    HOJY_CHECK_EQ(defender.hurt, 7);
    HOJY_CHECK_EQ(attacker.mp, 100);
    HOJY_CHECK_EQ(random.callCount(), 2U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 19);
    HOJY_CHECK_EQ(random.calls()[1].maximum, 19);
}

void testSkillMpCostUsesSelectedBattleLevel() {
    auto attacker = makeCharacter();
    auto defender = makeCharacter();
    auto skill = makeSkill();
    hojy::battle::SequenceRandom random({0, 0});

    const auto result = hojy::battle::applyDamage(
        attacker, defender, 0, 0, 1, skill, 5, random);

    HOJY_CHECK_EQ(result.applied, true);
    HOJY_CHECK_EQ(attacker.mp, 100);
}

void testPredictDamageAppliesDistanceAtZeroAndOne() {
    HOJY_CHECK_EQ(hojy::battle::calcPredictDamage(100, 0, 0, 0, 0), 67);
    HOJY_CHECK_EQ(hojy::battle::calcPredictDamage(100, 0, 0, 0, 1), 66);
}

void testPredictDamageKeepsAdditionsWhenPreliminaryDamageIsZero() {
    HOJY_CHECK_EQ(hojy::battle::calcPredictDamage(0, 0, 30, 20, 1), 3);
}

void testDamageFallbackUsesOriginalRandomBound() {
    auto attacker = makeCharacter();
    auto defender = makeCharacter();
    defender.defence = 100;
    auto skill = makeSkill();
    hojy::battle::SequenceRandom random({0, 19, 3, 0});

    const auto result = hojy::battle::applyDamage(
        attacker, defender, 0, 0, 1, skill, 0, random);

    HOJY_CHECK_EQ(result.damage, 20);
    HOJY_CHECK_EQ(random.callCount(), 4U);
    HOJY_CHECK_EQ(random.calls()[2].maximum, 3);
    HOJY_CHECK_EQ(random.calls()[3].maximum, 3);
}

void testZeroPreliminaryDamageUsesFallbackBranch() {
    auto attacker = makeCharacter();
    auto defender = makeCharacter();
    defender.defence = 36;
    auto skill = makeSkill();
    hojy::battle::SequenceRandom random({0, 1, 3, 0});

    const auto result = hojy::battle::applyDamage(
        attacker, defender, 0, 0, 1, skill, 0, random);

    HOJY_CHECK_EQ(result.damage, 20);
    HOJY_CHECK_EQ(random.callCount(), 4U);
}

void testSkillPoisonUsesOneBasedBattleLevel() {
    auto attacker = makeCharacter();
    auto defender = makeCharacter();
    auto skill = makeSkill();
    skill.addPoison = 30;
    hojy::battle::SequenceRandom random({0, 0});

    const auto result = hojy::battle::applyDamage(
        attacker, defender, 0, 0, 1, skill, 0, random);

    HOJY_CHECK_EQ(result.applied, true);
    HOJY_CHECK_EQ(defender.poisoned, 2);
    HOJY_CHECK_EQ(result.poisoned, 2);
}

void testLethalSkillStillWritesHurtAndPoison() {
    auto attacker = makeCharacter();
    auto defender = makeCharacter();
    defender.hp = 1;
    auto skill = makeSkill();
    skill.addPoison = 30;
    hojy::battle::SequenceRandom random({0, 0});

    const auto result = hojy::battle::applyDamage(
        attacker, defender, 0, 0, 1, skill, 0, random);

    HOJY_CHECK_EQ(result.dead, true);
    HOJY_CHECK_EQ(defender.hp, 0);
    HOJY_CHECK_EQ(defender.hurt, 5);
    HOJY_CHECK_EQ(defender.poisoned, 2);
}

void testDrainAndStatusActions() {
    auto attacker = makeCharacter();
    auto defender = makeCharacter();
    auto skill = makeSkill();
    skill.damageType = 1;
    skill.addMp[0] = 20;
    skill.drainMp[0] = 20;
    hojy::battle::SequenceRandom random({0, 0, 4, 2, 1});
    const auto result = hojy::battle::applyDamage(
        attacker, defender, 0, 0, 1, skill, 0, random);
    HOJY_CHECK_EQ(result.damage, 21);
    HOJY_CHECK_EQ(defender.mp, 79);
    HOJY_CHECK_EQ(attacker.mp, 104);
    HOJY_CHECK_EQ(attacker.maxMp, 104);
    HOJY_CHECK_EQ(random.callCount(), 5U);

    attacker.poison = 80;
    defender.antipoison = 0;
    defender.poisoned = 0;
    HOJY_CHECK_EQ(hojy::battle::applyPoison(attacker, defender, 2), 20);
    HOJY_CHECK_EQ(defender.poisoned, 20);

    attacker.medic = 50;
    attacker.stamina = 100;
    defender.hp = 50;
    hojy::battle::SequenceRandom healRandom({5});
    HOJY_CHECK_EQ(hojy::battle::applyMedic(attacker, defender, 2, healRandom), 40);
    HOJY_CHECK_EQ(defender.hp, 90);
    HOJY_CHECK_EQ(attacker.stamina, 98);
    HOJY_CHECK_EQ(healRandom.callCount(), 1U);
    HOJY_CHECK_EQ(healRandom.calls()[0].maximum, 4);
}

void testUtilityActionSharedTailAddsExperienceAndStaminaCost() {
    auto character = makeCharacter();
    character.stamina = 3;
    std::uint16_t experience = 7;

    hojy::battle::finishUtilityAction(character, experience);

    HOJY_CHECK_EQ(experience, std::uint16_t(8));
    HOJY_CHECK_EQ(character.stamina, 1);

    hojy::battle::finishUtilityAction(character, experience);
    HOJY_CHECK_EQ(experience, std::uint16_t(9));
    HOJY_CHECK_EQ(character.stamina, 0);
}

void testDrainUsesOriginalWriteAndRandomOrder() {
    auto attacker = makeCharacter();
    auto defender = makeCharacter();
    auto skill = makeSkill();
    skill.damageType = 1;
    skill.addMp[0] = 20;
    skill.drainMp[0] = 20;
    hojy::battle::SequenceRandom random({1, 2, 0, 1, 2});

    const auto result = hojy::battle::applyDamage(
        attacker, defender, 0, 0, 1, skill, 0, random);

    HOJY_CHECK_EQ(result.damage, 19);
    HOJY_CHECK_EQ(attacker.maxMp, 100);
    HOJY_CHECK_EQ(attacker.mp, 100);
    HOJY_CHECK_EQ(defender.mp, 81);
    HOJY_CHECK_EQ(random.callCount(), 5U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 2);
    HOJY_CHECK_EQ(random.calls()[1].maximum, 2);
    HOJY_CHECK_EQ(random.calls()[2].maximum, 9);
    HOJY_CHECK_EQ(random.calls()[3].maximum, 2);
    HOJY_CHECK_EQ(random.calls()[4].maximum, 2);
}

void testDepoisonUsesOriginalRandomBoundsAndThreshold() {
    auto attacker = makeCharacter();
    auto defender = makeCharacter();
    attacker.depoison = 30;
    defender.poisoned = 40;
    hojy::battle::SequenceRandom random({9, 0});

    HOJY_CHECK_EQ(hojy::battle::applyDepoison(attacker, defender, 4, random), 19);
    HOJY_CHECK_EQ(defender.poisoned, 21);
    HOJY_CHECK_EQ(attacker.stamina, 96);
    HOJY_CHECK_EQ(random.callCount(), 2U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 9);
    HOJY_CHECK_EQ(random.calls()[1].maximum, 9);

    defender.poisoned = 60;
    hojy::battle::SequenceRandom blockedRandom({0, 0});
    HOJY_CHECK_EQ(hojy::battle::applyDepoison(attacker, defender, 0, blockedRandom), 0);
    HOJY_CHECK_EQ(defender.poisoned, 60);
}

void testThrowUsesOriginalPoisonAndHurtUpdates() {
    auto attacker = makeCharacter();
    auto defender = makeCharacter();
    attacker.throwing = 0;
    defender.poisoned = 0;
    hojy::mem::ItemData item{};
    item.addHp = -60;
    item.addPoisoned = 20;
    bool dead = false;
    hojy::battle::SequenceRandom random({0});

    const auto result = hojy::battle::applyThrow(
        attacker, defender, item, 0, random, dead);

    HOJY_CHECK_EQ(result, -5);
    HOJY_CHECK_EQ(defender.hp, 95);
    HOJY_CHECK_EQ(defender.hurt, 1);
    HOJY_CHECK_EQ(defender.poisoned, 5);
    HOJY_CHECK_EQ(dead, false);
    HOJY_CHECK_EQ(random.callCount(), 1U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 4);

    item.addPoisoned = -30;
    defender.poisoned = 50;
    hojy::battle::SequenceRandom antidoteRandom({0, 0, 4});
    HOJY_CHECK_EQ(hojy::battle::applyThrow(
                      attacker, defender, item, 0, antidoteRandom, dead), -6);
    HOJY_CHECK_EQ(defender.poisoned, 31);
    HOJY_CHECK_EQ(antidoteRandom.callCount(), 3U);
    HOJY_CHECK_EQ(antidoteRandom.calls()[1].maximum, 4);
    HOJY_CHECK_EQ(antidoteRandom.calls()[2].maximum, 4);
}

void testRestUsesOriginalMovementAndStaminaTiers() {
    auto character = makeCharacter();
    character.speed = 60;
    character.stamina = 38;
    character.hp = 50;
    character.mp = 50;
    hojy::battle::SequenceRandom random({0, 1, 0});

    hojy::battle::applyRest(character, random, 0);

    HOJY_CHECK_EQ(character.stamina, 40);
    HOJY_CHECK_EQ(character.hp, 54);
    HOJY_CHECK_EQ(character.mp, 53);
    HOJY_CHECK_EQ(random.callCount(), 3U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 2);
    HOJY_CHECK_EQ(random.calls()[1].maximum, 1);
    HOJY_CHECK_EQ(random.calls()[2].maximum, 1);
}

void testRestDerivesInitialStepsFromSpeedAndHurt() {
    auto character = makeCharacter();
    character.speed = 60;
    character.hurt = 40;
    character.stamina = 38;
    character.hp = 50;
    character.mp = 50;
    hojy::battle::SequenceRandom random({0, 1, 0});

    // speed 60 / 15 - hurt 40 / 40 = 3; no movement keeps the stationary bonus.
    hojy::battle::applyRest(character, random, 3);

    HOJY_CHECK_EQ(character.stamina, 41);
}

void testRoundEndStatusDamagePreservesOneHp() {
    auto character = makeCharacter();
    character.hurt = 40;
    character.poisoned = 20;
    HOJY_CHECK_EQ(hojy::battle::applyRoundEndDamage(character), -4);
    HOJY_CHECK_EQ(character.hp, 96);

    character.hp = 10;
    character.poisoned = 95;
    character.hurt = 40;
    HOJY_CHECK_EQ(hojy::battle::applyRoundEndDamage(character), -9);
    HOJY_CHECK_EQ(character.hp, 1);

    character.hp = 1;
    character.poisoned = 99;
    character.hurt = 99;
    HOJY_CHECK_EQ(hojy::battle::applyRoundEndDamage(character), 0);
    HOJY_CHECK_EQ(character.hp, 1);
}

void testRoundEndStatusDamageUsesOriginalActivationOrder() {
    auto dead = makeCharacter();
    dead.hp = 0;
    dead.hurt = 40;
    dead.poisoned = 20;
    HOJY_CHECK_EQ(hojy::battle::applyRoundEndDamage(dead), 0);
    HOJY_CHECK_EQ(dead.hp, 0);

    auto poisonOnly = makeCharacter();
    poisonOnly.stamina = 0;
    poisonOnly.poisoned = 20;
    HOJY_CHECK_EQ(hojy::battle::applyRoundEndDamage(poisonOnly), 0);
    HOJY_CHECK_EQ(poisonOnly.hp, 100);

    auto hurtOnly = makeCharacter();
    hurtOnly.stamina = 0;
    hurtOnly.hurt = 20;
    HOJY_CHECK_EQ(hojy::battle::applyRoundEndDamage(hurtOnly), -1);
    HOJY_CHECK_EQ(hurtOnly.hp, 99);
}

void testPoisonDamageDoesNotReviveDeadCharacter() {
    auto character = makeCharacter();
    character.hp = 0;
    character.poisoned = 20;

    HOJY_CHECK_EQ(hojy::battle::applyPoisonDamage(character), 0);
    HOJY_CHECK_EQ(character.hp, 0);

    auto invalidPoison = makeCharacter();
    invalidPoison.hp = 50;
    invalidPoison.poisoned = -10;
    HOJY_CHECK_EQ(hojy::battle::applyPoisonDamage(invalidPoison), 0);
    HOJY_CHECK_EQ(invalidPoison.hp, 50);
}

}

int main() {
    try {
        testSkillLevelDowngrade();
        testRepeatedSkillContinuesAtMinimumLevelWhenMpIsExhausted();
        testForcedSkillLevelSharesRepeatSemanticsForAiSelection();
        testAiSkillPlanningKeepsStoredRangeWhileExecutionDowngrades();
        testTechniqueRangeUsesOriginalInclusiveBaseCell();
        testDrainPopupClassificationUsesSkillTypeNotSignedDamage();
        testBattleMaxMpMergePersistsOnlyGrowthBeyondEntryBonus();
        testDoubleAttackRequiresExactOriginalFlag();
        testRealAttackClampsSkillLevelBeforeDamageLookup();
        testDamageUsesDeterministicRandomOrder();
        testSkillMpCostUsesSelectedBattleLevel();
        testPredictDamageAppliesDistanceAtZeroAndOne();
        testPredictDamageKeepsAdditionsWhenPreliminaryDamageIsZero();
        testDamageFallbackUsesOriginalRandomBound();
        testZeroPreliminaryDamageUsesFallbackBranch();
        testSkillPoisonUsesOneBasedBattleLevel();
        testLethalSkillStillWritesHurtAndPoison();
        testDrainAndStatusActions();
        testUtilityActionSharedTailAddsExperienceAndStaminaCost();
        testDrainUsesOriginalWriteAndRandomOrder();
        testDepoisonUsesOriginalRandomBoundsAndThreshold();
        testThrowUsesOriginalPoisonAndHurtUpdates();
        testRestUsesOriginalMovementAndStaminaTiers();
        testRestDerivesInitialStepsFromSpeedAndHurt();
        testRoundEndStatusDamagePreservesOneHp();
        testRoundEndStatusDamageUsesOriginalActivationOrder();
        testPoisonDamageDoesNotReviveDeadCharacter();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
