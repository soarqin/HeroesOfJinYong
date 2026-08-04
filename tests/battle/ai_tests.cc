#include "battle/ai.hh"
#include "battle/random.hh"
#include "mem/character.hh"
#include "test_support.hh"

#include <iostream>
#include <vector>

namespace {

using hojy::battle::AiAction;
using hojy::battle::AiContext;
using hojy::battle::AiDecision;
using hojy::battle::AiItem;
using hojy::battle::AiParticipant;
using hojy::battle::AiRequest;
using hojy::battle::AiStats;
using hojy::battle::SequenceRandom;

AiStats healthy() {
    AiStats stats;
    stats.hp = 100;
    stats.maxHp = 100;
    stats.mp = 100;
    stats.maxMp = 100;
    stats.stamina = 100;
    stats.attack = 30;
    stats.minSkillReqMp = 0;
    return stats;
}

AiContext duel() {
    AiParticipant self;
    self.stats = healthy();
    self.side = 0;
    self.distance = 0;
    AiParticipant enemy;
    enemy.stats = healthy();
    enemy.side = 1;
    enemy.distance = 3;
    AiContext context;
    context.participants = {self, enemy};
    context.self = 0;
    return context;
}

void checkDecision(const AiDecision &decision, AiAction action,
                   int target = -1, int itemSlot = -1,
                   AiRequest request = AiRequest::None) {
    HOJY_CHECK_EQ(decision.action, action);
    HOJY_CHECK_EQ(decision.target, target);
    HOJY_CHECK_EQ(decision.itemSlot, itemSlot);
    HOJY_CHECK_EQ(decision.request, request);
}

void testInvalidContextsReturnSafeDefaultsWithoutRandom() {
    AiContext empty;
    SequenceRandom random({});
    checkDecision(hojy::battle::decideAiAction(empty, random), AiAction::Rest);
    HOJY_CHECK_EQ(hojy::battle::pickAiTarget(empty, random), -1);
    HOJY_CHECK_EQ(hojy::battle::pickAiPoisonTarget(empty, random), -1);
    HOJY_CHECK_EQ(random.callCount(), 0U);

    auto invalidSelf = duel();
    invalidSelf.self = 9;
    checkDecision(
        hojy::battle::decideAiAction(invalidSelf, random), AiAction::Rest);
    HOJY_CHECK_EQ(hojy::battle::pickAiTarget(invalidSelf, random), -1);
    HOJY_CHECK_EQ(hojy::battle::pickAiPoisonTarget(invalidSelf, random), -1);
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

void testHealthRecoveryMapsSelfItemAndRequestBranches() {
    {
        auto context = duel();
        context.participants[0].stats.hp = 19;
        context.participants[0].stats.medic = 40;
        context.items = {{7, 50, 0, 0}};
        SequenceRandom random({});
        checkDecision(
            hojy::battle::decideAiAction(context, random),
            AiAction::Medic, 0);
    }
    {
        auto context = duel();
        context.participants[0].stats.hp = 19;
        context.items = {
            {9, 0, 0, 0},
            {7, 10, 0, 0},
            {3, 50, 0, 0},
        };
        SequenceRandom random({});
        checkDecision(
            hojy::battle::decideAiAction(context, random),
            AiAction::UseItem, 0, 7);
    }
    {
        auto context = duel();
        context.participants[0].stats.hp = 19;
        AiParticipant provider;
        provider.stats = healthy();
        provider.stats.medic = 60;
        provider.side = 0;
        context.participants.push_back(provider);
        SequenceRandom random({});
        checkDecision(
            hojy::battle::decideAiAction(context, random),
            AiAction::Attack, -1, -1, AiRequest::Medic);
    }
}

void testDepoisonAndMpItemsKeepStrictGatesAndInputOrder() {
    {
        auto context = duel();
        context.participants[0].stats.poisoned = 60;
        context.participants[0].stats.depoison = 20;
        context.participants[0].stats.stamina = 50;
        context.items = {{4, 0, 0, -20}};
        SequenceRandom random({5});
        checkDecision(
            hojy::battle::decideAiAction(context, random),
            AiAction::UseItem, 0, 4);
    }
    {
        auto context = duel();
        context.participants[0].stats.poisoned = 60;
        context.participants[0].stats.depoison = 31;
        context.participants[0].stats.stamina = 51;
        context.items = {{4, 0, 0, -20}};
        SequenceRandom random({5});
        checkDecision(
            hojy::battle::decideAiAction(context, random),
            AiAction::Depoison, 0);
    }
    {
        auto context = duel();
        context.participants[0].stats.mp = 10;
        context.items = {{2, 0, 30, 0}};
        SequenceRandom random({9, 0});
        checkDecision(
            hojy::battle::decideAiAction(context, random),
            AiAction::UseItem, 0, 2);
    }
}

void testPendingRequestAndFollowupActionsMapWithoutDuplicatingRules() {
    {
        auto context = duel();
        context.participants[0].stats.medic = 80;
        AiParticipant ally;
        ally.stats = healthy();
        ally.side = 0;
        ally.request = AiRequest::Medic;
        context.participants.push_back(ally);
        SequenceRandom random({9, 0});
        checkDecision(
            hojy::battle::decideAiAction(context, random),
            AiAction::Medic, 2);
    }
    {
        auto context = duel();
        context.participants[0].stats.poison = 100;
        context.participants[0].stats.attack = 10;
        SequenceRandom random({9, 9, 0, 0});
        checkDecision(
            hojy::battle::decideAiAction(context, random),
            AiAction::Poison);
    }
    {
        auto context = duel();
        context.participants[0].stats.hp = 19;
        SequenceRandom random({9, 0});
        checkDecision(
            hojy::battle::decideAiAction(context, random),
            AiAction::Flee);
    }
}

void testTargetAdaptersPreserveTemperamentDistanceAndPoisonFilters() {
    AiContext context;
    AiParticipant self;
    self.stats = healthy();
    self.stats.integrity = 90;
    self.stats.poison = 60;
    self.side = 0;
    AiParticipant weak;
    weak.stats = healthy();
    weak.stats.attack = 10;
    weak.side = 1;
    weak.distance = 1;
    AiParticipant strong;
    strong.stats = healthy();
    strong.stats.attack = 90;
    strong.stats.antipoison = 80;
    strong.side = 1;
    strong.distance = 5;
    AiParticipant poisonable;
    poisonable.stats = healthy();
    poisonable.stats.attack = 50;
    poisonable.side = 1;
    poisonable.distance = 3;
    context.participants = {self, weak, strong, poisonable};
    context.self = 0;

    SequenceRandom targetRandom({0});
    HOJY_CHECK_EQ(hojy::battle::pickAiTarget(context, targetRandom), 2);

    context.participants[0].stats.potential = 0;
    SequenceRandom poisonRandom({});
    HOJY_CHECK_EQ(
        hojy::battle::pickAiPoisonTarget(context, poisonRandom), 1);
}

void testAiRuntimeSnapshotStaysAtBattleEntryBeforeEquipmentMerge() {
    hojy::mem::CharacterData entry{};
    entry.attack = 31;
    entry.medic = 32;
    entry.poison = 33;
    entry.depoison = 34;
    entry.antipoison = 35;
    entry.throwing = 36;

    const auto snapshot = hojy::battle::snapshotAiStats(entry);

    /* Simulate the effective combat copy after equipment bonuses are added. */
    auto effective = entry;
    effective.attack += 101;
    effective.medic += 102;
    effective.poison += 103;
    effective.depoison += 104;
    effective.antipoison += 105;
    effective.throwing += 106;
    const auto equipmentBonus = hojy::battle::captureAiEquipmentBonuses(
        snapshot, effective);

    /* A later battle mutation must survive while the static equipment delta
     * remains excluded from AI planning. */
    effective.medic += 7;
    effective.poison += 8;
    const auto current = hojy::battle::resolveAiRuntimeStats(
        snapshot, equipmentBonus, effective);

    HOJY_CHECK_EQ(snapshot.attack, 31);
    HOJY_CHECK_EQ(snapshot.medic, 32);
    HOJY_CHECK_EQ(snapshot.poison, 33);
    HOJY_CHECK_EQ(snapshot.depoison, 34);
    HOJY_CHECK_EQ(snapshot.antipoison, 35);
    HOJY_CHECK_EQ(snapshot.throwing, 36);
    HOJY_CHECK_EQ(current.attack, 31);
    HOJY_CHECK_EQ(current.medic, 39);
    HOJY_CHECK_EQ(current.poison, 41);
    HOJY_CHECK_EQ(current.depoison, 34);
    HOJY_CHECK_EQ(current.antipoison, 35);
    HOJY_CHECK_EQ(current.throwing, 36);
}

}

int main() {
    try {
        testInvalidContextsReturnSafeDefaultsWithoutRandom();
        testHealthRecoveryMapsSelfItemAndRequestBranches();
        testDepoisonAndMpItemsKeepStrictGatesAndInputOrder();
        testPendingRequestAndFollowupActionsMapWithoutDuplicatingRules();
        testTargetAdaptersPreserveTemperamentDistanceAndPoisonFilters();
        testAiRuntimeSnapshotStaysAtBattleEntryBeforeEquipmentMerge();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
