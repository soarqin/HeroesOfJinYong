#include "battle/ai_policy.hh"
#include "battle/random.hh"
#include "test_support.hh"

#include <iostream>
#include <vector>

namespace {

hojy::battle::AiResourceState makeState() {
    return hojy::battle::AiResourceState{
        100, 100, 0, 0, 100, 100, 100, 0, 0,
    };
}

hojy::battle::AiAllyState makeAlly(int side, int hp = 100, int maxHp = 100,
                                  int hurt = 0, int poisoned = 0,
                                  int actionCode = 0, int medic = 0,
                                  int depoison = 0, int attack = 0) {
    auto ally = hojy::battle::AiAllyState{
        side, true, true, hp, maxHp, hurt, poisoned,
        medic, depoison, actionCode,
    };
    ally.attack = attack;
    return ally;
}

void testHealthRecoveryUsesOriginalThresholdOrder() {
    auto state = makeState();
    state.hp = 24;
    hojy::battle::SequenceRandom random({9, 9, 6});

    HOJY_CHECK_EQ(hojy::battle::shouldAttemptHealthRecovery(state, random), true);
    HOJY_CHECK_EQ(random.callCount(), 3U);
    HOJY_CHECK_EQ(random.calls()[0].maximum, 9);
    HOJY_CHECK_EQ(random.calls()[1].maximum, 9);
    HOJY_CHECK_EQ(random.calls()[2].maximum, 9);
}

void testCriticalHealthAndHurtDoNotConsumeRandom() {
    auto state = makeState();
    state.hp = 19;
    hojy::battle::SequenceRandom hpRandom({});
    HOJY_CHECK_EQ(hojy::battle::shouldAttemptHealthRecovery(state, hpRandom), true);
    HOJY_CHECK_EQ(hpRandom.callCount(), 0U);

    state = makeState();
    state.hurt = 51;
    hojy::battle::SequenceRandom hurtRandom({});
    HOJY_CHECK_EQ(hojy::battle::shouldAttemptHealthRecovery(state, hurtRandom), true);
    HOJY_CHECK_EQ(hurtRandom.callCount(), 0U);
}

void testMpRecoveryUsesAllApplicableThresholds() {
    auto state = makeState();
    state.mp = 19;
    hojy::battle::SequenceRandom random({9, 9, 9, 7});

    HOJY_CHECK_EQ(hojy::battle::shouldAttemptMpRecovery(state, random), true);
    HOJY_CHECK_EQ(random.callCount(), 4U);
}

void testSelfDepoisonGateConsumesOneRandomValue() {
    auto state = makeState();
    state.poisoned = 39;
    hojy::battle::SequenceRandom successRandom({2});
    HOJY_CHECK_EQ(hojy::battle::shouldAttemptSelfDepoison(state, successRandom), true);
    HOJY_CHECK_EQ(successRandom.callCount(), 1U);

    hojy::battle::SequenceRandom failRandom({3});
    HOJY_CHECK_EQ(hojy::battle::shouldAttemptSelfDepoison(state, failRandom), false);
    HOJY_CHECK_EQ(failRandom.callCount(), 1U);
}

void testSelfMedicAndDepoisonUseAsymmetricOriginalGates() {
    HOJY_CHECK_EQ(hojy::battle::canSelfMedic(20, 50, 49), true);
    HOJY_CHECK_EQ(hojy::battle::canSelfMedic(20, 49, 50), false);
    HOJY_CHECK_EQ(hojy::battle::canSelfMedic(19, 100, 0), false);
    HOJY_CHECK_EQ(hojy::battle::canSelfMedic(20, 50, 49), true);
    HOJY_CHECK_EQ(hojy::battle::canSelfMedic(20, 50, 51), false);

    HOJY_CHECK_EQ(hojy::battle::canSelfDepoison(21, 51, 50), true);
    HOJY_CHECK_EQ(hojy::battle::canSelfDepoison(20, 51, 51), false);
    HOJY_CHECK_EQ(hojy::battle::canSelfDepoison(21, 50, 51), false);
    HOJY_CHECK_EQ(hojy::battle::canSelfDepoison(21, 51, 52), false);
}

void testMedicSupportUsesCapabilityThresholdOrder() {
    auto state = makeState();
    state.medic = 60;
    hojy::battle::SequenceRandom random({9, 9, 7});
    HOJY_CHECK_EQ(hojy::battle::shouldAttemptMedicSupport(state, random), true);
    HOJY_CHECK_EQ(random.callCount(), 3U);

    state.medic = 80;
    hojy::battle::SequenceRandom guaranteedRandom({9, 9, 9});
    HOJY_CHECK_EQ(hojy::battle::shouldAttemptMedicSupport(state, guaranteedRandom), true);
    HOJY_CHECK_EQ(guaranteedRandom.callCount(), 3U);

    state.stamina = 50;
    hojy::battle::SequenceRandom staminaRandom({});
    HOJY_CHECK_EQ(hojy::battle::shouldAttemptMedicSupport(state, staminaRandom), false);
    HOJY_CHECK_EQ(staminaRandom.callCount(), 0U);
}

void testDepoisonSupportUsesSameCapabilityThresholds() {
    auto state = makeState();
    state.depoison = 40;
    hojy::battle::SequenceRandom random({9, 5});
    HOJY_CHECK_EQ(hojy::battle::shouldAttemptDepoisonSupport(state, random), true);
    HOJY_CHECK_EQ(random.callCount(), 2U);
}

void testLowStaminaIsADeferredFallback() {
    auto state = makeState();
    state.stamina = 9;
    HOJY_CHECK_EQ(hojy::battle::shouldRestForStamina(state), true);
    state.stamina = 10;
    HOJY_CHECK_EQ(hojy::battle::shouldRestForStamina(state), false);
}

void testRetreatHealthGateUsesOriginalShortCircuitOrder() {
    auto state = makeState();
    state.hp = 19;
    hojy::battle::SequenceRandom critical({4});
    HOJY_CHECK_EQ(
        hojy::battle::shouldRetreatForHealth(state, critical), true);
    HOJY_CHECK_EQ(critical.callCount(), 1U);

    state.maxHp = 200;
    state.hp = 30;
    hojy::battle::SequenceRandom quarterMiss({4, 6, 7});
    HOJY_CHECK_EQ(
        hojy::battle::shouldRetreatForHealth(state, quarterMiss), true);
    HOJY_CHECK_EQ(quarterMiss.callCount(), 3U);

    state.hp = 20;
    hojy::battle::SequenceRandom outerMiss({5});
    HOJY_CHECK_EQ(
        hojy::battle::shouldRetreatForHealth(state, outerMiss), false);
    HOJY_CHECK_EQ(outerMiss.callCount(), 1U);
}

void testResourceSelectionPreservesOriginalBranchOrder() {
    auto state = makeState();
    state.stamina = 9;
    hojy::battle::SequenceRandom restRandom({});
    HOJY_CHECK_EQ(
        hojy::battle::chooseAiResourceAction(state, {}, restRandom),
        hojy::battle::AiResourceAction::Rest);
    HOJY_CHECK_EQ(restRandom.callCount(), 0U);

    state.hp = 19;
    state.mp = 10;
    hojy::battle::AiResourceOptions options;
    options.mpRecovery = true;
    hojy::battle::SequenceRandom recoveryRandom({9, 1});
    HOJY_CHECK_EQ(
        hojy::battle::chooseAiResourceAction(state, options, recoveryRandom),
        hojy::battle::AiResourceAction::RecoverMp);
    HOJY_CHECK_EQ(recoveryRandom.callCount(), 2U);
}

void testResourceResolversRunLazilyInBranchOrder() {
    auto state = makeState();
    state.stamina = 9;
    state.hp = 19;
    state.mp = 10;
    std::vector<hojy::battle::AiResourceAction> attempts;
    hojy::battle::SequenceRandom random({9, 1});

    const auto selected = hojy::battle::chooseAiResourceAction(
        state, random, [&attempts](hojy::battle::AiResourceAction action) {
            attempts.push_back(action);
            return action == hojy::battle::AiResourceAction::RecoverMp
                ? action : hojy::battle::AiResourceAction::None;
        });

    HOJY_CHECK_EQ(selected, hojy::battle::AiResourceAction::RecoverMp);
    HOJY_CHECK_EQ(attempts.size(), 2U);
    HOJY_CHECK_EQ(attempts[0], hojy::battle::AiResourceAction::RecoverHp);
    HOJY_CHECK_EQ(attempts[1], hojy::battle::AiResourceAction::RecoverMp);
    HOJY_CHECK_EQ(random.callCount(), 2U);
}

void testResourceResolverCanSelectRequestAction() {
    auto state = makeState();
    state.hp = 19;
    hojy::battle::SequenceRandom random({});

    const auto selected = hojy::battle::chooseAiResourceAction(
        state, random, [](hojy::battle::AiResourceAction action) {
            return action == hojy::battle::AiResourceAction::RecoverHp
                ? hojy::battle::AiResourceAction::RequestMedic
                : hojy::battle::AiResourceAction::None;
        });

    HOJY_CHECK_EQ(selected, hojy::battle::AiResourceAction::RequestMedic);
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

void testMedicProviderUsesSlotOrderAndStrictAbilityThresholds() {
    auto invalid = makeAlly(0, 100, 100, 0, 0, 0, 100);
    invalid.valid = false;
    std::vector<hojy::battle::AiAllyState> allies{
        makeAlly(0, 100, 100, 70),
        invalid,
        makeAlly(1, 100, 100, 0, 0, 0, 100),
        makeAlly(0, 100, 100, 0, 0, 0, 40),
        makeAlly(0, 100, 100, 0, 0, 0, 41),
        makeAlly(0, 100, 100, 0, 0, 0, 80),
    };

    const auto provider = hojy::battle::chooseMedicProvider(0, 70, allies);

    HOJY_CHECK_EQ(provider.has_value(), true);
    HOJY_CHECK_EQ(*provider, 4);
}

void testDepoisonProviderUsesSlotOrderAndStrictAbilityThresholds() {
    auto dead = makeAlly(0, 100, 100, 0, 0, 0, 0, 100);
    dead.alive = false;
    std::vector<hojy::battle::AiAllyState> allies{
        makeAlly(0, 100, 100, 0, 70),
        dead,
        makeAlly(0, 100, 100, 0, 0, 0, 0, 40),
        makeAlly(0, 100, 100, 0, 0, 0, 0, 41),
    };

    const auto provider = hojy::battle::chooseDepoisonProvider(0, 70, allies);

    HOJY_CHECK_EQ(provider.has_value(), true);
    HOJY_CHECK_EQ(*provider, 3);
}

void testUnreachableSupportFallbackUsesOriginalPowerComparison() {
    HOJY_CHECK_EQ(
        hojy::battle::chooseUnreachableSupportFallback(50, 101, 2),
        hojy::battle::AiSupportFallback::Rest);
    HOJY_CHECK_EQ(
        hojy::battle::chooseUnreachableSupportFallback(51, 101, 2),
        hojy::battle::AiSupportFallback::Attack);
    HOJY_CHECK_EQ(
        hojy::battle::chooseUnreachableSupportFallback(150, 300, 2),
        hojy::battle::AiSupportFallback::Rest);
}

void testAllyPowerSummaryUsesBaseAttackAndCurrentHealth() {
    const std::vector<hojy::battle::AiAllyState> allies{
        makeAlly(0, 25, 100, 0, 0, 0, 0, 0, 10),
        makeAlly(0, 30, 100, 0, 0, 0, 0, 0, 20),
        makeAlly(1, 90, 100, 0, 0, 0, 0, 0, 999),
    };

    const auto summary = hojy::battle::summarizeAllyPower(0, allies);

    HOJY_CHECK_EQ(summary.total, 85);
    HOJY_CHECK_EQ(summary.count, 2);
}

void testResourceActionMapsToOriginalActionCode() {
    HOJY_CHECK_EQ(hojy::battle::originalActionCode(
                      hojy::battle::AiResourceAction::None, false), 0);
    HOJY_CHECK_EQ(hojy::battle::originalActionCode(
                      hojy::battle::AiResourceAction::Rest, false), 7);
    HOJY_CHECK_EQ(hojy::battle::originalActionCode(
                      hojy::battle::AiResourceAction::RequestMedic, false), 8);
    HOJY_CHECK_EQ(hojy::battle::originalActionCode(
                      hojy::battle::AiResourceAction::RequestDepoison, false), 9);
    HOJY_CHECK_EQ(hojy::battle::originalActionCode(
                      hojy::battle::AiResourceAction::RecoverHp, false), 5);
    HOJY_CHECK_EQ(hojy::battle::originalActionCode(
                      hojy::battle::AiResourceAction::MedicSupport, false), 5);
    HOJY_CHECK_EQ(hojy::battle::originalActionCode(
                      hojy::battle::AiResourceAction::SelfDepoison, false), 4);
    HOJY_CHECK_EQ(hojy::battle::originalActionCode(
                      hojy::battle::AiResourceAction::DepoisonSupport, false), 4);
    HOJY_CHECK_EQ(hojy::battle::originalActionCode(
                      hojy::battle::AiResourceAction::RecoverHp, true), 6);
}

void testMedicTargetUsesSlotOrderAndRequestPriority() {
    auto invalid = makeAlly(0, 100, 100, 0, 0, 8);
    invalid.valid = false;
    auto enemy = makeAlly(1, 100, 100, 0, 0, 8);
    auto dead = makeAlly(0, 100, 100, 0, 0, 8);
    dead.alive = false;
    std::vector<hojy::battle::AiAllyState> allies{
        makeAlly(0), invalid, enemy, dead,
        makeAlly(0, 100, 100, 0, 0, 8),
        makeAlly(0, 10),
    };
    hojy::battle::SequenceRandom random({});

    const auto target = hojy::battle::chooseMedicSupportTarget(
        0, 40, allies, random);

    HOJY_CHECK_EQ(target.has_value(), true);
    HOJY_CHECK_EQ(*target, 4);
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

void testMedicTargetUsesAllApplicableThresholdsInSlotOrder() {
    std::vector<hojy::battle::AiAllyState> allies{
        makeAlly(0),
        makeAlly(0, 24),
        makeAlly(0, 49),
    };
    hojy::battle::SequenceRandom random({9, 9, 9, 6});

    const auto target = hojy::battle::chooseMedicSupportTarget(
        0, 40, allies, random);

    HOJY_CHECK_EQ(target.has_value(), true);
    HOJY_CHECK_EQ(*target, 2);
    HOJY_CHECK_EQ(random.callCount(), 4U);
}

void testMedicTargetRejectsInsufficientAbilityWithoutRandom() {
    std::vector<hojy::battle::AiAllyState> allies{
        makeAlly(0),
        makeAlly(0, 10, 100, 50),
    };
    hojy::battle::SequenceRandom random({});

    const auto target = hojy::battle::chooseMedicSupportTarget(
        0, 20, allies, random);

    HOJY_CHECK_EQ(target.has_value(), false);
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

void testDepoisonTargetUsesRequestAndSlotOrder() {
    std::vector<hojy::battle::AiAllyState> allies{
        makeAlly(0),
        makeAlly(1, 100, 100, 0, 0, 9),
        makeAlly(0, 100, 100, 0, 0, 9),
        makeAlly(0, 100, 100, 0, 50),
    };
    hojy::battle::SequenceRandom random({});

    const auto target = hojy::battle::chooseDepoisonSupportTarget(
        0, 40, allies, random);

    HOJY_CHECK_EQ(target.has_value(), true);
    HOJY_CHECK_EQ(*target, 2);
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

void testDepoisonTargetUsesAllApplicableThresholds() {
    std::vector<hojy::battle::AiAllyState> allies{
        makeAlly(0),
        makeAlly(0, 100, 100, 0, 35),
    };
    hojy::battle::SequenceRandom random({9, 9, 7});

    const auto target = hojy::battle::chooseDepoisonSupportTarget(
        0, 40, allies, random);

    HOJY_CHECK_EQ(target.has_value(), true);
    HOJY_CHECK_EQ(*target, 1);
    HOJY_CHECK_EQ(random.callCount(), 3U);
}

void testDepoisonTargetGuaranteedThresholdFollowsRandomChecks() {
    std::vector<hojy::battle::AiAllyState> allies{
        makeAlly(0),
        makeAlly(0, 100, 100, 0, 41),
    };
    hojy::battle::SequenceRandom random({9, 9, 9});

    const auto target = hojy::battle::chooseDepoisonSupportTarget(
        0, 40, allies, random);

    HOJY_CHECK_EQ(target.has_value(), true);
    HOJY_CHECK_EQ(*target, 1);
    HOJY_CHECK_EQ(random.callCount(), 3U);
}

void testDepoisonTargetRejectsInsufficientAbilityWithoutRandom() {
    std::vector<hojy::battle::AiAllyState> allies{
        makeAlly(0),
        makeAlly(0, 100, 100, 0, 50),
    };
    hojy::battle::SequenceRandom random({});

    const auto target = hojy::battle::chooseDepoisonSupportTarget(
        0, 20, allies, random);

    HOJY_CHECK_EQ(target.has_value(), false);
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

}

int main() {
    try {
        testHealthRecoveryUsesOriginalThresholdOrder();
        testCriticalHealthAndHurtDoNotConsumeRandom();
        testMpRecoveryUsesAllApplicableThresholds();
        testSelfDepoisonGateConsumesOneRandomValue();
        testSelfMedicAndDepoisonUseAsymmetricOriginalGates();
        testMedicSupportUsesCapabilityThresholdOrder();
        testDepoisonSupportUsesSameCapabilityThresholds();
        testLowStaminaIsADeferredFallback();
        testRetreatHealthGateUsesOriginalShortCircuitOrder();
        testResourceSelectionPreservesOriginalBranchOrder();
        testResourceResolversRunLazilyInBranchOrder();
        testResourceResolverCanSelectRequestAction();
        testMedicProviderUsesSlotOrderAndStrictAbilityThresholds();
        testDepoisonProviderUsesSlotOrderAndStrictAbilityThresholds();
        testUnreachableSupportFallbackUsesOriginalPowerComparison();
        testAllyPowerSummaryUsesBaseAttackAndCurrentHealth();
        testResourceActionMapsToOriginalActionCode();
        testMedicTargetUsesSlotOrderAndRequestPriority();
        testMedicTargetUsesAllApplicableThresholdsInSlotOrder();
        testMedicTargetRejectsInsufficientAbilityWithoutRandom();
        testDepoisonTargetUsesRequestAndSlotOrder();
        testDepoisonTargetUsesAllApplicableThresholds();
        testDepoisonTargetGuaranteedThresholdFollowsRandomChecks();
        testDepoisonTargetRejectsInsufficientAbilityWithoutRandom();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
