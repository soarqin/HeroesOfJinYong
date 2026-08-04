#include "battle/ai_strategy.hh"
#include "battle/random.hh"
#include "test_support.hh"

#include <iostream>
#include <vector>

namespace {

using hojy::battle::AiFollowupAction;
using hojy::battle::AiFollowupDecision;
using hojy::battle::AiSkillOption;
using hojy::battle::AiStrategyActor;
using hojy::battle::AiStrategyCharacter;
using hojy::battle::AiThrowingOption;
using hojy::battle::SelectableCell;
using hojy::battle::SelectableCells;
using hojy::battle::SequenceRandom;

AiStrategyCharacter character(int side, int hp, int maxHp, int attack,
                              int poison = 0, bool valid = true,
                              bool alive = true) {
    return AiStrategyCharacter{
        side, valid, alive, 0, 0, hp, maxHp, attack, 0, poison, 0, 0,
    };
}

AiStrategyActor actor(int side = 1) {
    AiStrategyActor value;
    value.side = side;
    value.hp = 100;
    value.attack = 10;
    value.stamina = 100;
    value.mp = 50;
    value.medic = 0;
    value.poison = 0;
    value.depoison = 0;
    value.throwing = 30;
    return value;
}

void checkDecision(const AiFollowupDecision &decision, AiFollowupAction action,
                   int targetIndex = -1, int selectionIndex = -1) {
    HOJY_CHECK_EQ(decision.action, action);
    HOJY_CHECK_EQ(decision.targetIndex, targetIndex);
    HOJY_CHECK_EQ(decision.selectionIndex, selectionIndex);
}

void testMedicSupportGateAndLargestStrictGap() {
    auto value = actor(1);
    value.attack = 10;
    value.hp = 10;
    value.medic = 20;
    value.stamina = 50;
    std::vector<AiStrategyCharacter> characters{
        character(1, 3000, 3000, 1000),
        character(1, 2500, 3000, 1000),
        character(0, 1000, 1000, 0),
        character(0, 1000, 1000, 0),
    };
    SequenceRandom random({});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, characters, {}, {}, random);
    checkDecision(decision, AiFollowupAction::MedicSupport, 1);
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

void testMedicSupportTieKeepsFirstSlotAndGateIsStrict() {
    auto value = actor(1);
    value.medic = 20;
    value.stamina = 50;
    value.attack = 30;
    value.hp = 20;
    std::vector<AiStrategyCharacter> characters{
        character(1, 3000, 4000, 1000),
        character(1, 3000, 4000, 1000),
        character(0, 1000, 1000, 0),
        character(0, 1000, 1000, 0),
    };
    SequenceRandom random({});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, characters, {}, {}, random);
    checkDecision(decision, AiFollowupAction::MedicSupport, 1);
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

void testDepoisonSupportBranchUsesLargestPoisonAndNoMedicFallback() {
    auto value = actor(1);
    value.medic = 19;
    value.depoison = 20;
    value.stamina = 50;
    value.attack = 20;
    value.hp = 20;
    std::vector<AiStrategyCharacter> characters{
        character(1, 3000, 3000, 1000, 30),
        character(1, 3000, 3000, 1000, 30),
        character(0, 1000, 1000, 0),
        character(0, 1000, 1000, 0),
    };
    SequenceRandom random({});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, characters, {}, {}, random);
    checkDecision(decision, AiFollowupAction::DepoisonSupport, 1);
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

void testSupportGateEqualityFallsThroughStrictly() {
    auto value = actor(1);
    value.attack = 10;
    value.hp = 10;
    value.medic = 20;
    value.stamina = 50;
    std::vector<AiStrategyCharacter> characters{
        character(1, 90, 100, 10),
        character(1, 90, 100, 10),
        character(0, 40, 40, 0),
        character(0, 40, 40, 0),
    };
    SequenceRandom random({0});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, characters, {}, {}, random);
    checkDecision(decision, AiFollowupAction::Rest);
    HOJY_CHECK_EQ(random.callCount(), 1U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 49);
}

void testSupportSkipsInvalidAndDeadCandidates() {
    auto value = actor(1);
    value.medic = 19;
    value.depoison = 20;
    value.stamina = 50;
    std::vector<AiStrategyCharacter> characters{
        character(1, 3000, 3000, 1000, 0),
        character(1, 3000, 3000, 1000, 90, false, true),
        character(1, 3000, 3000, 1000, 80, true, false),
        character(1, 3000, 3000, 1000, 40, true, true),
        character(0, 1000, 1000, 0),
        character(0, 1000, 1000, 0),
    };
    SequenceRandom random({});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, characters, {}, {}, random);
    checkDecision(decision, AiFollowupAction::DepoisonSupport, 3);
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

void testSupportPowerStillCountsInactiveBattleSlots() {
    auto value = actor(1);
    value.attack = 10;
    value.hp = 10;
    value.medic = 20;
    value.stamina = 50;
    std::vector<AiStrategyCharacter> characters{
        character(1, 10, 10, 10),
        character(1, 10, 100, 0),
        character(1, 0, 100, 3000, 0, false, false),
        character(0, 1000, 1000, 0),
    };
    SequenceRandom random({49});

    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, characters, {}, {}, random);

    checkDecision(decision, AiFollowupAction::MedicSupport, 1);
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

void testSupportGateFailureFallsThroughToPoison() {
    auto value = actor(1);
    value.attack = 50;
    value.hp = 50;
    value.poison = 100;
    value.medic = 20;
    value.stamina = 50;
    std::vector<AiStrategyCharacter> characters{
        character(1, 100, 100, 10),
        character(0, 100, 100, 10),
    };
    SequenceRandom random({0, 0});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, characters, {}, {}, random);
    checkDecision(decision, AiFollowupAction::Poison);
    HOJY_CHECK_EQ(random.callCount(), 2U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 49);
    HOJY_CHECK_EQ(random.calls()[1].maximum, 149);
}

void testPoisonFirstGateFailureDoesNotConsumeSecondRandom() {
    auto value = actor(1);
    value.attack = 10;
    value.hp = 100;
    value.poison = 11;
    SequenceRandom random({10, 0});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, {character(1, 100, 100, 10), character(0, 100, 100, 10)},
        {}, {}, random);
    checkDecision(decision, AiFollowupAction::Rest);
    HOJY_CHECK_EQ(random.callCount(), 1U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 49);
}

void testPoisonSecondGateFailureConsumesBothRandoms() {
    auto value = actor(1);
    value.attack = 10;
    value.hp = 100;
    value.poison = 30;
    SequenceRandom random({0, 30});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, {character(1, 100, 100, 10), character(0, 100, 100, 10)},
        {}, {}, random);
    checkDecision(decision, AiFollowupAction::Rest);
    HOJY_CHECK_EQ(random.callCount(), 2U);
    HOJY_CHECK_EQ(random.calls()[1].maximum, 149);
}

void testNpcThrowChecksNegativeAndPoisonOnSameItemInOrder() {
    auto value = actor(1);
    value.attack = 10;
    value.hp = 100;
    std::vector<AiThrowingOption> items{
        {7, 101, -20, 30},
    };
    SequenceRandom random({9, 9, 2});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, {character(1, 100, 100, 10), character(0, 100, 100, 10)},
        items, {}, random);
    checkDecision(decision, AiFollowupAction::Throw, -1, 7);
    HOJY_CHECK_EQ(random.callCount(), 3U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 49);
    HOJY_CHECK_EQ(random.calls()[1].maximum, 9);
    HOJY_CHECK_EQ(random.calls()[2].maximum, 9);
}

void testNpcThrowPreservesSlotOrderAfterFailedItem() {
    auto value = actor(1);
    value.attack = 10;
    value.hp = 100;
    std::vector<AiThrowingOption> items{
        {2, 101, -20, 0},
        {5, 102, 0, 30},
    };
    SequenceRandom random({9, 9, 2});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, {character(1, 100, 100, 10), character(0, 100, 100, 10)},
        items, {}, random);
    checkDecision(decision, AiFollowupAction::Throw, -1, 5);
    HOJY_CHECK_EQ(random.callCount(), 3U);
}

void testNpcThrowSkipsEquipmentAndBookSlots() {
    auto value = actor(1);
    value.attack = 10;
    std::vector<AiThrowingOption> items{
        {7, 101, -100, 0, 1},
        {8, 102, -20, 0, 3},
    };
    SequenceRandom random({0, 2});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, {character(1, 100, 100, 10), character(0, 100, 100, 10)},
        items, {}, random);
    checkDecision(decision, AiFollowupAction::Throw, -1, 8);
    HOJY_CHECK_EQ(random.callCount(), 2U);
}

void testPlayerThrowUsesAttackThresholdAndThrowingRoll() {
    auto value = actor(0);
    value.attack = 10;
    value.throwing = 30;
    value.hp = 100;
    std::vector<AiThrowingOption> items{
        {3, 201, -16, 0},
    };
    SequenceRandom random({9, 21});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, {character(0, 100, 100, 10), character(1, 100, 100, 10)},
        items, {}, random);
    checkDecision(decision, AiFollowupAction::Throw, -1, 3);
    HOJY_CHECK_EQ(random.callCount(), 2U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 49);
    HOJY_CHECK_EQ(random.calls()[1].maximum, 29);
}

void testPlayerThrowPoisonUsesTenSidedRollAfterFailedHeal() {
    auto value = actor(0);
    value.attack = 10;
    value.throwing = 30;
    value.hp = 100;
    std::vector<AiThrowingOption> items{
        {4, 202, 0, 16},
    };
    SequenceRandom random({9, 2});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, {character(0, 100, 100, 10), character(1, 100, 100, 10)},
        items, {}, random);
    checkDecision(decision, AiFollowupAction::Throw, -1, 4);
    HOJY_CHECK_EQ(random.callCount(), 2U);
    HOJY_CHECK_EQ(random.calls()[1].maximum, 9);
}

void testStaminaRestSkipsSkillScan() {
    auto value = actor(1);
    value.stamina = 10;
    value.poison = 0;
    SequenceRandom random({0});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, {}, {}, {{0, 1, 0}}, random);
    checkDecision(decision, AiFollowupAction::Rest);
    HOJY_CHECK_EQ(random.callCount(), 1U);
}

void testNoSkillsReturnsRest() {
    auto value = actor(1);
    value.poison = 0;
    SequenceRandom random({0});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, {}, {}, {{0, 0, 0}}, random);
    checkDecision(decision, AiFollowupAction::Rest);
    HOJY_CHECK_EQ(random.callCount(), 1U);
}

void testMpBelowMinimumPositiveSkillRequirementRests() {
    auto value = actor(1);
    value.mp = 9;
    value.poison = 0;
    SequenceRandom random({0});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, {}, {}, {{0, 1, 10}, {1, 2, 20}, {9, 0, 0}}, random);
    checkDecision(decision, AiFollowupAction::Rest);
    HOJY_CHECK_EQ(random.callCount(), 1U);
}

void testMpAtMinimumPositiveSkillRequirementReturnsSkillWithoutRandom() {
    auto value = actor(1);
    value.mp = 10;
    value.poison = 0;
    SequenceRandom random({0});
    const auto decision = hojy::battle::chooseAiFollowupAction(
        0, value, {}, {}, {{0, 1, 10}, {1, 2, 20}, {9, 0, 0}}, random);
    checkDecision(decision, AiFollowupAction::Skill);
    HOJY_CHECK_EQ(random.callCount(), 1U);
}

void testTargetIntegrityBranchesUseStrictAttackTieOrder() {
    auto value = actor(0);
    value.integrity = 80;
    std::vector<AiStrategyCharacter> characters{
        character(0, 100, 100, 10),
        character(1, 100, 100, 50),
        character(1, 100, 100, 50),
        character(1, 100, 100, 20, 0, false),
    };
    SequenceRandom random({6});
    const auto target = hojy::battle::chooseAiTarget(
        0, value, characters, random,
        [](int) { return 99; });
    HOJY_CHECK_EQ(target.has_value(), true);
    HOJY_CHECK_EQ(*target, 1);
    HOJY_CHECK_EQ(random.callCount(), 1U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 9);
}

void testTargetStrategyContinuesWhenTriggeredBranchFindsNoCandidate() {
    auto value = actor(0);
    value.integrity = 80;
    value.potential = 70;
    std::vector<AiStrategyCharacter> characters{
        character(0, 100, 100, 10),
        character(1, 100, 100, 0),
        character(1, 100, 100, 0),
    };
    characters[0].poisonTechnique = 30;
    characters[1].depoison = 10;
    characters[2].medic = 20;
    SequenceRandom random({0, 0});
    const auto target = hojy::battle::chooseAiTarget(
        0, value, characters, random,
        [](int index) { return index == 1 ? 1 : 5; });
    HOJY_CHECK_EQ(target.has_value(), true);
    HOJY_CHECK_EQ(*target, 2);
    HOJY_CHECK_EQ(random.callCount(), 2U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 9);
    HOJY_CHECK_EQ(random.calls()[1].maximum, 9);
}

void testTargetLowIntegrityFallsThroughToNearestWhenRollMisses() {
    auto value = actor(0);
    value.integrity = 20;
    std::vector<AiStrategyCharacter> characters{
        character(0, 100, 100, 10),
        character(1, 100, 100, 100),
        character(1, 100, 100, 1),
    };
    SequenceRandom random({7});
    const auto target = hojy::battle::chooseAiTarget(
        0, value, characters, random,
        [](int index) { return index == 1 ? 5 : 2; });
    HOJY_CHECK_EQ(target.has_value(), true);
    HOJY_CHECK_EQ(*target, 2);
    HOJY_CHECK_EQ(random.callCount(), 1U);
}

void testPotentialAbilityTargetPreservesOriginalFallbackDefect() {
    auto value = actor(0);
    value.integrity = 50;
    value.potential = 70;
    auto deadPoisoner = character(0, 0, 100, 0, 30, true, false);
    deadPoisoner.poisonTechnique = 30;
    std::vector<AiStrategyCharacter> characters{
        character(0, 100, 100, 10), deadPoisoner,
        character(1, 100, 100, 90, 0, true, true),
        character(1, 100, 100, 5, 0, true, true),
    };
    characters[2].depoison = 25;
    characters[3].depoison = 30;
    SequenceRandom random({0});
    const auto target = hojy::battle::chooseAiTarget(
        0, value, characters, random,
        [](int) { return 1; });
    // A depoison value >= 20 sets the DOS scan's first flag, but the
    // uninitialised medical-success flag still routes to lowest attack.
    HOJY_CHECK_EQ(target.has_value(), true);
    HOJY_CHECK_EQ(*target, 3);
}

void testPotentialAbilityTargetUsesMedicalScanBaselineWithoutReset() {
    auto value = actor(0);
    value.integrity = 50;
    value.potential = 70;
    std::vector<AiStrategyCharacter> characters{
        character(0, 100, 100, 10, 30),
        character(1, 100, 100, 90),
        character(1, 100, 100, 5),
    };
    characters[0].poisonTechnique = 30;
    characters[1].depoison = 10;
    characters[1].medic = 20;
    characters[2].depoison = 15;
    characters[2].medic = 14;
    SequenceRandom random({0});
    const auto target = hojy::battle::chooseAiTarget(
        0, value, characters, random,
        [](int) { return 1; });
    HOJY_CHECK_EQ(target.has_value(), true);
    HOJY_CHECK_EQ(*target, 1);
}

void testTargetFallbackUsesNearestDistanceAndKeepsFirstTie() {
    auto value = actor(0);
    value.integrity = 50;
    value.potential = 0;
    std::vector<AiStrategyCharacter> characters{
        character(0, 100, 100, 10),
        character(1, 100, 100, 50),
        character(1, 100, 100, 10),
    };
    SequenceRandom random({});
    const auto target = hojy::battle::chooseAiTarget(
        0, value, characters, random,
        [](int) { return 3; });
    HOJY_CHECK_EQ(target.has_value(), true);
    HOJY_CHECK_EQ(*target, 1);
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

void testNearestTargetScanIsDeterministicAndCanExcludeFirstTarget() {
    auto value = actor(0);
    std::vector<AiStrategyCharacter> characters{
        character(0, 100, 100, 10),
        character(1, 100, 100, 50),
        character(1, 100, 100, 20),
        character(1, 0, 100, 90, 0, true, false),
    };
    const auto nearest = hojy::battle::chooseNearestAiTarget(
        0, value.side, characters,
        [](int index) { return index == 1 ? 2 : index == 2 ? 4 : 1; });
    HOJY_CHECK_EQ(nearest.has_value(), true);
    HOJY_CHECK_EQ(*nearest, 1);

    const auto fallback = hojy::battle::chooseNearestAiTarget(
        0, value.side, characters,
        [](int index) { return index == 1 ? 2 : index == 2 ? 4 : 1; }, 1);
    HOJY_CHECK_EQ(fallback.has_value(), true);
    HOJY_CHECK_EQ(*fallback, 2);
}

void testNearestTargetUsesOriginalThousandStepCutoff() {
    auto value = actor(0);
    std::vector<AiStrategyCharacter> characters{
        character(0, 100, 100, 10),
        character(1, 100, 100, 50),
    };
    const auto outsideCutoff = hojy::battle::chooseNearestAiTarget(
        0, value.side, characters, [](int) { return 1000; });
    HOJY_CHECK_EQ(outsideCutoff.has_value(), false);

    const auto insideCutoff = hojy::battle::chooseNearestAiTarget(
        0, value.side, characters, [](int) { return 999; });
    HOJY_CHECK_EQ(insideCutoff.has_value(), true);
    HOJY_CHECK_EQ(*insideCutoff, 1);
}

void testPoisonTargetUsesPotentialGateThenHighestAttack() {
    auto value = actor(0);
    value.potential = 61;
    value.poison = 50;
    std::vector<AiStrategyCharacter> characters{
        character(0, 100, 100, 10),
        character(1, 100, 100, 20, 94),
        character(1, 100, 100, 70, 20),
        character(1, 100, 100, 90, 95),
    };
    characters[1].antipoison = 10;
    characters[2].antipoison = 49;
    characters[3].antipoison = 0;
    SequenceRandom random({6});
    const auto target = hojy::battle::choosePoisonTarget(
        0, value, characters, random,
        [](int index) { return index; });
    HOJY_CHECK_EQ(target.has_value(), true);
    HOJY_CHECK_EQ(*target, 2);
    HOJY_CHECK_EQ(random.callCount(), 1U);
}

void testPoisonTargetFallsBackToNearestEligibleEnemy() {
    auto value = actor(0);
    value.potential = 60;
    value.poison = 30;
    std::vector<AiStrategyCharacter> characters{
        character(0, 100, 100, 10),
        character(1, 100, 100, 90, 20),
        character(1, 100, 100, 10, 20),
    };
    characters[1].antipoison = 29;
    characters[2].antipoison = 30;
    SequenceRandom random({});
    const auto target = hojy::battle::choosePoisonTarget(
        0, value, characters, random,
        [](int index) { return index == 1 ? 3 : 1; });
    HOJY_CHECK_EQ(target.has_value(), true);
    HOJY_CHECK_EQ(*target, 1);
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

void testOriginalSkillSlotUsesPositiveCountAndCompactOrdinal() {
    std::vector<AiSkillOption> skills{
        {0, 11, 5}, {1, 22, 10}, {2, 33, 15},
    };
    SequenceRandom random({1});
    const auto slot = hojy::battle::chooseOriginalSkillSlot(skills, random);
    HOJY_CHECK_EQ(slot.has_value(), true);
    HOJY_CHECK_EQ(*slot, 1);
    HOJY_CHECK_EQ(random.callCount(), 1U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 2);
}

void testOriginalSkillSlotWithOneSkillDoesNotConsumeRandom() {
    SequenceRandom random({42});
    const auto slot = hojy::battle::chooseOriginalSkillSlot(
        {{4, 99, 1}, {5, 0, 0}}, random);
    HOJY_CHECK_EQ(slot.has_value(), true);
    HOJY_CHECK_EQ(*slot, 4);
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

void testOriginalSkillSlotReturnsStoredSlotFromCompactOptions() {
    SequenceRandom random({1});
    const auto slot = hojy::battle::chooseOriginalSkillSlot(
        {{3, 11, 1}, {8, 22, 1}}, random);
    HOJY_CHECK_EQ(slot.has_value(), true);
    HOJY_CHECK_EQ(*slot, 8);
}

void testOriginalSkillSlotSkipsEmptySparseSlots() {
    SequenceRandom random({1});
    const auto slot = hojy::battle::chooseOriginalSkillSlot(
        {{0, 0, 0}, {4, 11, 1}, {7, 22, 1}}, random);
    HOJY_CHECK_EQ(slot.has_value(), true);
    HOJY_CHECK_EQ(*slot, 7);
    HOJY_CHECK_EQ(random.callCount(), 1U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 1);
}

void testRetreatPositionRequiresExactStepsAndStrictScore() {
    SelectableCells cells;
    cells[{0, 0}] = SelectableCell{0, 0, 2, 0, nullptr, nullptr};
    cells[{1, 1}] = SelectableCell{1, 1, 2, 0, nullptr, nullptr};
    cells[{2, 2}] = SelectableCell{2, 2, 1, 0, nullptr, nullptr};
    const auto position = hojy::battle::chooseRetreatPosition(
        cells, 2, {{0, 3}, {3, 0}});
    HOJY_CHECK_EQ(position.has_value(), true);
    HOJY_CHECK_EQ(*position, std::make_pair(0, 0));
}

}

int main() {
    try {
        testMedicSupportGateAndLargestStrictGap();
        testMedicSupportTieKeepsFirstSlotAndGateIsStrict();
        testDepoisonSupportBranchUsesLargestPoisonAndNoMedicFallback();
        testSupportGateEqualityFallsThroughStrictly();
        testSupportSkipsInvalidAndDeadCandidates();
        testSupportPowerStillCountsInactiveBattleSlots();
        testSupportGateFailureFallsThroughToPoison();
        testPoisonFirstGateFailureDoesNotConsumeSecondRandom();
        testPoisonSecondGateFailureConsumesBothRandoms();
        testNpcThrowChecksNegativeAndPoisonOnSameItemInOrder();
        testNpcThrowPreservesSlotOrderAfterFailedItem();
        testNpcThrowSkipsEquipmentAndBookSlots();
        testPlayerThrowUsesAttackThresholdAndThrowingRoll();
        testPlayerThrowPoisonUsesTenSidedRollAfterFailedHeal();
        testStaminaRestSkipsSkillScan();
        testNoSkillsReturnsRest();
        testMpBelowMinimumPositiveSkillRequirementRests();
        testMpAtMinimumPositiveSkillRequirementReturnsSkillWithoutRandom();
        testTargetIntegrityBranchesUseStrictAttackTieOrder();
        testNearestTargetUsesOriginalThousandStepCutoff();
        testTargetStrategyContinuesWhenTriggeredBranchFindsNoCandidate();
        testTargetLowIntegrityFallsThroughToNearestWhenRollMisses();
        testPotentialAbilityTargetPreservesOriginalFallbackDefect();
        testPotentialAbilityTargetUsesMedicalScanBaselineWithoutReset();
        testTargetFallbackUsesNearestDistanceAndKeepsFirstTie();
        testNearestTargetScanIsDeterministicAndCanExcludeFirstTarget();
        testPoisonTargetUsesPotentialGateThenHighestAttack();
        testPoisonTargetFallsBackToNearestEligibleEnemy();
        testOriginalSkillSlotUsesPositiveCountAndCompactOrdinal();
        testOriginalSkillSlotWithOneSkillDoesNotConsumeRandom();
        testOriginalSkillSlotReturnsStoredSlotFromCompactOptions();
        testOriginalSkillSlotSkipsEmptySparseSlots();
        testRetreatPositionRequiresExactStepsAndStrictScore();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
