#include "battle/action.hh"
#include "battle/engine.hh"
#include "battle/random.hh"
#include "test_support.hh"

#include <iostream>
#include <type_traits>

namespace {

using hojy::battle::ActionTarget;
using hojy::battle::BattleAction;
using hojy::battle::BattleCell;
using hojy::battle::BattleEngine;
using hojy::battle::BattleParticipant;
using hojy::battle::EngineStatus;
using hojy::battle::InventorySource;
using hojy::battle::ItemAction;
using hojy::battle::MoveAction;
using hojy::battle::RecordingRandom;
using hojy::battle::SequenceRandom;
using hojy::battle::SkillAction;
using hojy::battle::Technique;
using hojy::battle::TechniqueAction;

void testBattleEngineRecordsActionsWithoutApplyingStateTwice() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = 20;
    enemy.hp = 10;
    BattleParticipant playerParticipant(player);
    BattleParticipant enemyParticipant(enemy);
    SequenceRandom sequence({5});
    RecordingRandom random(sequence);
    BattleEngine engine;
    HOJY_CHECK_EQ(engine.begin({
        {&playerParticipant, &enemyParticipant}, {false, true}, &random,
        {{100, 2}},
    }), true);

    HOJY_CHECK_EQ(random.next(10), 5);
    enemyParticipant.state().hp = 4;
    const BattleAction action{
        0,
        SkillAction{0, 7, 2, {ActionTarget{1, 1}}},
    };
    HOJY_CHECK_EQ(engine.record(action, {{100, 2}}), true);
    HOJY_CHECK_EQ(enemyParticipant.state().hp, 4);
    const auto snapshot = engine.snapshot();
    HOJY_CHECK_EQ(snapshot.actions, 1U);
    HOJY_CHECK_EQ(snapshot.actionLog.size(), 1U);
    HOJY_CHECK_EQ(snapshot.actionLog[0].randomBegin, 0U);
    HOJY_CHECK_EQ(snapshot.actionLog[0].randomEnd, 1U);
    HOJY_CHECK_EQ(snapshot.actionLog[0].participants[1].hp, 4);
    HOJY_CHECK_EQ(engine.finish(false).committed, false);
    HOJY_CHECK_EQ(enemy.hp, 10);
}

void testBattleEngineAcceptsAlliedUtilityAndOrderedSkillTargets() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData ally{};
    hojy::world::state::CharacterData enemyA{};
    hojy::world::state::CharacterData enemyB{};
    player.hp = ally.hp = enemyA.hp = enemyB.hp = 10;
    BattleParticipant p(player);
    BattleParticipant a(ally);
    BattleParticipant e1(enemyA);
    BattleParticipant e2(enemyB);
    BattleEngine engine;
    HOJY_CHECK_EQ(engine.begin({
        {&p, &a, &e1, &e2}, {false, false, true, true}, nullptr, {},
    }), true);

    HOJY_CHECK_EQ(engine.record(
        BattleAction{0, TechniqueAction{Technique::Medic, 1}}, {}), true);
    HOJY_CHECK_EQ(engine.record(
        BattleAction{0, TechniqueAction{Technique::Depoison, 0}}, {}), true);
    HOJY_CHECK_EQ(engine.record(
        BattleAction{0, TechniqueAction{Technique::Poison, 2}}, {}), true);
    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, SkillAction{1, 9, 3,
                       {ActionTarget{3, 2}, ActionTarget{2, 1}}},
    }, {}), true);
    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, SkillAction{1, 9, 3, {}},
    }, {}), true);
    HOJY_CHECK_EQ(engine.snapshot().actions, 5U);
    HOJY_CHECK_EQ(engine.finish(false).committed, false);
}

void testBattleEngineRecordsMovementItemsRestAndRoundEnd() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = enemy.hp = 10;
    BattleParticipant p(player);
    BattleParticipant e(enemy);
    BattleEngine engine;
    HOJY_CHECK_EQ(engine.begin({{&p, &e}, {false, true}, nullptr, {{42, 2}}}), true);

    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, hojy::battle::MoveAction{BattleCell{1, 1}, BattleCell{2, 1}},
    }, {{42, 2}}), true);
    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, hojy::battle::ThrowAction{1, 42, InventorySource::PartyBag, -1},
    }, {{42, 1}}), true);
    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, hojy::battle::ItemAction{42, InventorySource::PartyBag},
    }, {}), true);
    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, hojy::battle::RestAction{true},
    }, {}), true);
    HOJY_CHECK_EQ(engine.record(BattleAction{
        1, hojy::battle::RoundEndAction{false},
    }, {}), true);
    HOJY_CHECK_EQ(engine.snapshot().actions, 5U);
    engine.finish(false);
}

void testBattleEngineFaultsInvalidActionAndRollsBack() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = 20;
    enemy.hp = 10;
    BattleParticipant p(player);
    BattleParticipant e(enemy);
    BattleEngine engine;
    HOJY_CHECK_EQ(engine.begin({{&p, &e}, {false, true}, nullptr, {}}), true);
    e.state().hp = 5;
    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, SkillAction{0, 7, 1, {ActionTarget{9, 1}}},
    }, {}), false);
    HOJY_CHECK_EQ(engine.status(), EngineStatus::Faulted);
    HOJY_CHECK_EQ(engine.finish(true).committed, false);
    HOJY_CHECK_EQ(enemy.hp, 10);
    HOJY_CHECK_EQ(e.state().hp, 10);
}

void testBattleEngineCommitsFinishedObservedState() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = 20;
    enemy.hp = 10;
    BattleParticipant p(player);
    BattleParticipant e(enemy);
    BattleEngine engine;
    HOJY_CHECK_EQ(engine.begin({{&p, &e}, {false, true}, nullptr, {}}), true);
    e.state().hp = 0;
    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, SkillAction{0, 7, 1, {ActionTarget{1, 1}}},
    }, {}), true);
    HOJY_CHECK_EQ(engine.reconcile(), true);
    HOJY_CHECK_EQ(engine.status(), EngineStatus::Finished);
    const auto result = engine.finish(true);
    HOJY_CHECK_EQ(result.committed, true);
    HOJY_CHECK_EQ(result.won, true);
    HOJY_CHECK_EQ(result.actions, 1U);
    HOJY_CHECK_EQ(enemy.hp, 0);
}

void testBattleEngineDefersFinishUntilTheTurnBoundary() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = 20;
    enemy.hp = 10;
    BattleParticipant p(player);
    BattleParticipant e(enemy);
    BattleEngine engine;
    HOJY_CHECK_EQ(engine.begin({{&p, &e}, {false, true}, nullptr, {}}), true);

    e.state().hp = 0;
    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, SkillAction{0, 7, 1, {ActionTarget{1, 1}}},
    }, {}), true);
    HOJY_CHECK_EQ(engine.status(), EngineStatus::Active);

    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, SkillAction{0, 7, 1, {}},
    }, {}), true);
    HOJY_CHECK_EQ(engine.reconcile(), true);
    HOJY_CHECK_EQ(engine.status(), EngineStatus::Finished);
    HOJY_CHECK_EQ(engine.finish(true).committed, true);
}

void testBattleReplayIsDeterministicAndRejectsCorruptRandomLog() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = 20;
    enemy.hp = 10;
    BattleParticipant p(player);
    BattleParticipant e(enemy);
    SequenceRandom sequence({7, 2});
    RecordingRandom random(sequence);
    BattleEngine engine;
    HOJY_CHECK_EQ(engine.begin({
        {&p, &e}, {false, true}, &random, {{9, 2}},
    }), true);

    HOJY_CHECK_EQ(random.next(10), 7);
    e.state().hp = 4;
    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, SkillAction{0, 7, 1, {ActionTarget{1, 1}}},
    }, {{9, 2}}), true);
    HOJY_CHECK_EQ(random.next(1, 3), 3);
    e.state().hp = 0;
    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, SkillAction{0, 7, 1, {ActionTarget{1, 1}}},
    }, {{9, 2}}), true);

    HOJY_CHECK_EQ(engine.reconcile(), true);
    const auto result = engine.finish(true);
    HOJY_CHECK_EQ(result.committed, true);
    const auto first = BattleEngine::replay(result.replay);
    const auto second = BattleEngine::replay(result.replay);
    HOJY_CHECK_EQ(first.valid, true);
    HOJY_CHECK_EQ(second.valid, true);
    HOJY_CHECK_EQ(first.won, true);
    HOJY_CHECK_EQ(first.actions, 2U);
    HOJY_CHECK_EQ(first.participants[1].hp, 0);
    HOJY_CHECK_EQ(first.inventory.size(), 1U);
    HOJY_CHECK_EQ(first.inventory[0].second, 2);
    HOJY_CHECK_EQ(second.participants[1].hp, first.participants[1].hp);
    HOJY_CHECK_EQ(second.randomCalls.size(), first.randomCalls.size());

    auto corrupt = result.replay;
    ++corrupt.randomCalls[0].result;
    const auto rejected = BattleEngine::replay(corrupt);
    HOJY_CHECK_EQ(rejected.valid, false);
    HOJY_CHECK_EQ(rejected.actions, 0U);
    HOJY_CHECK_EQ(rejected.randomCalls.empty(), true);

    auto corruptMiddle = result.replay;
    ++corruptMiddle.actions[0].participants[0].exp;
    const auto rejectedMiddle = BattleEngine::replay(corruptMiddle);
    HOJY_CHECK_EQ(rejectedMiddle.valid, false);
}

void testBattleEngineRejectsDuplicateParticipantsAndRollsBackOnDestruction() {
    hojy::world::state::CharacterData character{};
    character.hp = 10;
    BattleParticipant participant(character);
    participant.state().hp = 1;
    BattleEngine invalid;
    HOJY_CHECK_EQ(invalid.begin({
        {&participant, &participant}, {false, true}, nullptr, {},
    }), false);
    HOJY_CHECK_EQ(invalid.finish(true).committed, false);
    HOJY_CHECK_EQ(participant.state().hp, 10);

    hojy::world::state::CharacterData enemy{};
    enemy.hp = 10;
    BattleParticipant enemyParticipant(enemy);
    {
        BattleEngine engine;
        HOJY_CHECK_EQ(engine.begin({
            {&participant, &enemyParticipant}, {false, true}, nullptr, {},
        }), true);
        enemyParticipant.state().hp = 1;
        HOJY_CHECK_EQ(engine.record(BattleAction{
            0, SkillAction{0, 7, 1, {ActionTarget{1, 1}}},
        }, {}), true);
    }
    HOJY_CHECK_EQ(enemy.hp, 10);
    HOJY_CHECK_EQ(enemyParticipant.state().hp, 10);
}

void testBattleEngineReconcileCapturesFinalInventory() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = enemy.hp = 10;
    BattleParticipant p(player);
    BattleParticipant e(enemy);
    BattleEngine engine;
    HOJY_CHECK_EQ(engine.begin({
        {&p, &e}, {false, true}, nullptr, {{42, 2}},
    }), true);

    e.state().hp = 0;
    HOJY_CHECK_EQ(engine.reconcile({{42, 1}}), true);
    HOJY_CHECK_EQ(engine.status(), EngineStatus::Finished);
    const auto result = engine.finish(true);
    HOJY_CHECK_EQ(result.replay.initialInventory.size(), 1U);
    HOJY_CHECK_EQ(result.replay.actions.size(), 0U);
    HOJY_CHECK_EQ(result.replay.initialInventory[0].second, 2);
    const auto replay = BattleEngine::replay(result.replay);
    HOJY_CHECK_EQ(replay.valid, true);
    HOJY_CHECK_EQ(replay.inventory.size(), 1U);
    HOJY_CHECK_EQ(replay.inventory[0].second, 1);
}

void testBattleReplayRejectsOneSidedSetup() {
    hojy::world::state::CharacterData player{};
    player.hp = 10;
    const hojy::battle::BattleReplay replay{
        {player}, {false}, {}, {}, {}, false, {}, {},
    };
    HOJY_CHECK_EQ(BattleEngine::replay(replay).valid, false);
}

void testBattleEngineSeparatesTrailingRandomFromTheFinalAction() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = enemy.hp = 10;
    BattleParticipant p(player);
    BattleParticipant e(enemy);
    SequenceRandom sequence({2});
    RecordingRandom random(sequence);
    BattleEngine engine;
    HOJY_CHECK_EQ(engine.begin({{&p, &e}, {false, true}, &random, {}}), true);
    e.state().hp = 4;
    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, SkillAction{0, 7, 1, {ActionTarget{1, 1}}},
    }, {}), true);
    e.state().hp = 0;
    HOJY_CHECK_EQ(engine.reconcile(), true);
    HOJY_CHECK_EQ(random.next(3), 2);
    const auto result = engine.finish(true);
    HOJY_CHECK_EQ(result.replay.actions[0].randomEnd, 0U);
    HOJY_CHECK_EQ(result.replay.settlementRandomBegin, 0U);
    HOJY_CHECK_EQ(BattleEngine::replay(result.replay).valid, true);
}

void testBattleEngineRejectsInvalidEnumsAndInventoryTransitions() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = enemy.hp = 10;
    BattleParticipant p(player);
    BattleParticipant e(enemy);

    BattleEngine invalidTechnique;
    HOJY_CHECK_EQ(invalidTechnique.begin({
        {&p, &e}, {false, true}, nullptr, {{42, 1}},
    }), true);
    HOJY_CHECK_EQ(invalidTechnique.record(BattleAction{
        0, TechniqueAction{static_cast<Technique>(99), 1},
    }, {{42, 1}}), false);

    BattleParticipant p2(player);
    BattleParticipant e2(enemy);
    BattleEngine invalidSource;
    HOJY_CHECK_EQ(invalidSource.begin({
        {&p2, &e2}, {false, true}, nullptr, {{42, 1}},
    }), true);
    HOJY_CHECK_EQ(invalidSource.record(BattleAction{
        0, ItemAction{42, InventorySource::NpcCarry},
    }, {{42, 1}}), false);

    BattleParticipant p3(player);
    BattleParticipant e3(enemy);
    BattleEngine invalidTransition;
    HOJY_CHECK_EQ(invalidTransition.begin({
        {&p3, &e3}, {false, true}, nullptr, {{42, 1}},
    }), true);
    HOJY_CHECK_EQ(invalidTransition.record(BattleAction{
        0, MoveAction{BattleCell{1, 1}, BattleCell{2, 1}},
    }, {}), false);
}

void testBattleEngineValidatesNpcCarrySlotTransition() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = enemy.hp = 10;
    for (int index = 0; index < 4; ++index) {
        player.item[index] = enemy.item[index] = -1;
        player.itemCount[index] = enemy.itemCount[index] = 0;
    }
    enemy.item[0] = 42;
    enemy.itemCount[0] = 1;
    BattleParticipant p(player);
    BattleParticipant e(enemy);
    BattleEngine engine;
    HOJY_CHECK_EQ(engine.begin({{&p, &e}, {false, true}, nullptr, {}}), true);
    e.state().item[0] = -1;
    e.state().itemCount[0] = 0;
    HOJY_CHECK_EQ(engine.record(BattleAction{
        1, ItemAction{42, InventorySource::NpcCarry, 0},
    }, {}), true);
    engine.finish(false);

    BattleParticipant p2(player);
    BattleParticipant e2(enemy);
    BattleEngine invalid;
    HOJY_CHECK_EQ(invalid.begin({{&p2, &e2}, {false, true}, nullptr, {}}), true);
    HOJY_CHECK_EQ(invalid.record(BattleAction{
        1, ItemAction{42, InventorySource::NpcCarry, 1},
    }, {}), false);
}

void testBattleEnginePreservesInventoryOrderInTransitions() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = enemy.hp = 10;
    BattleParticipant p(player);
    BattleParticipant e(enemy);
    BattleEngine engine;
    const hojy::battle::InventorySnapshot initial{{42, 1}, {7, 2}};
    HOJY_CHECK_EQ(engine.begin({{&p, &e}, {false, true}, nullptr, initial}), true);

    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, MoveAction{BattleCell{1, 1}, BattleCell{2, 1}},
    }, {{7, 2}, {42, 1}}), false);
    const auto snapshot = engine.snapshot();
    HOJY_CHECK_EQ(snapshot.inventory, initial);
    HOJY_CHECK_EQ(snapshot.actionLog.empty(), true);
    engine.abort();
}

void testBattleEngineRejectsUnrelatedParticipantMutationAndRestoresState() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = enemy.hp = 10;
    BattleParticipant p(player);
    BattleParticipant e(enemy);
    BattleEngine engine;
    HOJY_CHECK_EQ(engine.begin({{&p, &e}, {false, true}, nullptr, {{42, 2}}}), true);

    p.state().hp = 8;
    e.state().exp = 3;
    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, ItemAction{42, InventorySource::PartyBag, -1},
    }, {{42, 1}}), false);
    HOJY_CHECK_EQ(p.state().hp, 10);
    HOJY_CHECK_EQ(e.state().exp, 0);
    const auto snapshot = engine.snapshot();
    const hojy::battle::InventorySnapshot expected{{42, 2}};
    HOJY_CHECK_EQ(snapshot.inventory, expected);
    HOJY_CHECK_EQ(snapshot.actionLog.empty(), true);
    engine.abort();
    HOJY_CHECK_EQ(player.hp, 10);
    HOJY_CHECK_EQ(enemy.exp, 0);
}

void testBattleReplayBindsInitialStateAndCommitDisposition() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = 20;
    enemy.hp = 10;
    BattleParticipant p(player);
    BattleParticipant e(enemy);
    BattleEngine engine;
    HOJY_CHECK_EQ(engine.begin({{&p, &e}, {false, true}, nullptr, {}}), true);
    e.state().hp = 0;
    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, SkillAction{0, 7, 1, {ActionTarget{1, 1}}},
    }, {}), true);
    HOJY_CHECK_EQ(engine.reconcile(), true);
    const auto result = engine.finish(true);
    HOJY_CHECK_EQ(result.replay.committed, true);
    HOJY_CHECK_EQ(result.replay.initialIntegrity != 0, true);
    HOJY_CHECK_EQ(BattleEngine::replay(result.replay).valid, true);

    auto alteredInitial = result.replay;
    ++alteredInitial.initialParticipants[0].exp;
    HOJY_CHECK_EQ(BattleEngine::replay(alteredInitial).valid, false);

    hojy::world::state::CharacterData alreadyDeadEnemy{};
    alreadyDeadEnemy.hp = 0;
    BattleParticipant p2(player);
    BattleParticipant e2(alreadyDeadEnemy);
    BattleEngine aborted;
    HOJY_CHECK_EQ(aborted.begin({{&p2, &e2}, {false, true}, nullptr, {}}), true);
    const auto abortedResult = aborted.finish(false);
    HOJY_CHECK_EQ(abortedResult.committed, false);
    HOJY_CHECK_EQ(BattleEngine::replay(abortedResult.replay).valid, false);
    auto forgedCommit = abortedResult.replay;
    forgedCommit.committed = true;
    HOJY_CHECK_EQ(BattleEngine::replay(forgedCommit).valid, false);
}

void testBattleReplayRejectsRandomModuloAliases() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = 20;
    enemy.hp = 10;
    BattleParticipant p(player);
    BattleParticipant e(enemy);
    SequenceRandom sequence({7});
    RecordingRandom random(sequence);
    BattleEngine engine;
    HOJY_CHECK_EQ(engine.begin({{&p, &e}, {false, true}, &random, {}}), true);
    HOJY_CHECK_EQ(random.next(10), 7);
    e.state().hp = 0;
    HOJY_CHECK_EQ(engine.record(BattleAction{
        0, SkillAction{0, 7, 1, {ActionTarget{1, 1}}},
    }, {}), true);
    HOJY_CHECK_EQ(engine.reconcile(), true);
    const auto result = engine.finish(true);
    auto altered = result.replay;
    altered.randomCalls[0].rawValue += 10;
    HOJY_CHECK_EQ(BattleEngine::replay(altered).valid, false);
}

void testBattleReplayTracksSettlementRandomCallsWithoutActions() {
    hojy::world::state::CharacterData player{};
    hojy::world::state::CharacterData enemy{};
    player.hp = 20;
    enemy.hp = 0;
    BattleParticipant p(player);
    BattleParticipant e(enemy);
    SequenceRandom sequence({5});
    RecordingRandom random(sequence);
    BattleEngine engine;
    HOJY_CHECK_EQ(engine.begin({{&p, &e}, {false, true}, &random, {}}), true);
    HOJY_CHECK_EQ(random.next(10), 5);
    const auto result = engine.finish(true);
    HOJY_CHECK_EQ(result.committed, true);
    HOJY_CHECK_EQ(result.replay.actions.empty(), true);
    HOJY_CHECK_EQ(result.replay.randomCalls.empty(), false);
    HOJY_CHECK_EQ(result.replay.settlementRandomBegin, 0U);
    HOJY_CHECK_EQ(BattleEngine::replay(result.replay).valid, true);
    auto corruptBoundary = result.replay;
    corruptBoundary.settlementRandomBegin = 1;
    HOJY_CHECK_EQ(BattleEngine::replay(corruptBoundary).valid, false);
}

}

int main() {
    try {
        HOJY_CHECK_EQ(
            std::is_copy_constructible<BattleParticipant>::value, false);
        HOJY_CHECK_EQ(
            std::is_move_constructible<BattleParticipant>::value, false);
        HOJY_CHECK_EQ(std::is_copy_constructible<BattleEngine>::value, false);
        HOJY_CHECK_EQ(std::is_move_constructible<BattleEngine>::value, false);
        testBattleEngineRecordsActionsWithoutApplyingStateTwice();
        testBattleEngineAcceptsAlliedUtilityAndOrderedSkillTargets();
        testBattleEngineRecordsMovementItemsRestAndRoundEnd();
        testBattleEngineFaultsInvalidActionAndRollsBack();
        testBattleEngineCommitsFinishedObservedState();
        testBattleEngineDefersFinishUntilTheTurnBoundary();
        testBattleReplayIsDeterministicAndRejectsCorruptRandomLog();
        testBattleEngineRejectsDuplicateParticipantsAndRollsBackOnDestruction();
        testBattleEngineReconcileCapturesFinalInventory();
        testBattleReplayRejectsOneSidedSetup();
        testBattleEngineSeparatesTrailingRandomFromTheFinalAction();
        testBattleEngineRejectsInvalidEnumsAndInventoryTransitions();
        testBattleEngineValidatesNpcCarrySlotTransition();
        testBattleEnginePreservesInventoryOrderInTransitions();
        testBattleEngineRejectsUnrelatedParticipantMutationAndRestoresState();
        testBattleReplayBindsInitialStateAndCommitDisposition();
        testBattleReplayRejectsRandomModuloAliases();
        testBattleReplayTracksSettlementRandomCallsWithoutActions();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
