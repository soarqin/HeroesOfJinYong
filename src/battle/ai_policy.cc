#include "ai_policy.hh"

namespace hojy::battle {

namespace {

bool shouldAttemptResourceRecovery(int value, int maximum,
                                   int halfChance, int thirdChance,
                                   int quarterChance, int fifthChance,
                                   RandomSource &random) {
    if (value < maximum / 2 && random.next(10) < halfChance) { return true; }
    if (value < maximum / 3 && random.next(10) < thirdChance) { return true; }
    if (value < maximum / 4 && random.next(10) < quarterChance) { return true; }
    return value < maximum / 5 && random.next(10) < fifthChance;
}

bool shouldAttemptSupport(int ability, int stamina, RandomSource &random) {
    if (stamina <= 50) { return false; }
    if (ability >= 20 && random.next(10) < 4) { return true; }
    if (ability >= 40 && random.next(10) < 6) { return true; }
    if (ability >= 60 && random.next(10) < 8) { return true; }
    return ability >= 80;
}

bool needsMedicSupport(const AiAllyState &target, RandomSource &random) {
    if (target.actionCode == 8 || target.hp < 20 || target.hurt > 40) {
        return true;
    }
    if (target.hp < target.maxHp / 2 && random.next(10) < 7) { return true; }
    if (target.hp < target.maxHp / 3 && random.next(10) < 8) { return true; }
    if (target.hp < target.maxHp / 4 && random.next(10) < 9) { return true; }
    return target.hp < target.maxHp / 5;
}

bool needsDepoisonSupport(const AiAllyState &target, RandomSource &random) {
    if (target.actionCode == 9) { return true; }
    if (target.poisoned > 10 && random.next(10) < 4) { return true; }
    if (target.poisoned > 20 && random.next(10) < 6) { return true; }
    if (target.poisoned > 30 && random.next(10) < 8) { return true; }
    return target.poisoned > 40;
}

template<typename CanSupport, typename NeedsSupport>
std::optional<int> chooseSupportTarget(
    int actorIndex, const std::vector<AiAllyState> &allies,
    CanSupport canSupport, NeedsSupport needsSupport) {
    if (actorIndex < 0 || actorIndex >= static_cast<int>(allies.size())) {
        return std::nullopt;
    }
    const auto actorSide = allies[actorIndex].side;
    for (std::size_t i = 0; i < allies.size(); ++i) {
        const auto &target = allies[i];
        if (static_cast<int>(i) == actorIndex || !target.valid || !target.alive
            || target.side != actorSide || !canSupport(target)) {
            continue;
        }
        if (needsSupport(target)) { return static_cast<int>(i); }
    }
    return std::nullopt;
}

template<typename GetAbility>
std::optional<int> chooseSupportProvider(
    int actorIndex, int requiredState,
    const std::vector<AiAllyState> &allies, GetAbility getAbility) {
    if (actorIndex < 0 || actorIndex >= static_cast<int>(allies.size())) {
        return std::nullopt;
    }
    const auto actorSide = allies[actorIndex].side;
    for (std::size_t i = 0; i < allies.size(); ++i) {
        const auto &provider = allies[i];
        const auto ability = getAbility(provider);
        if (static_cast<int>(i) == actorIndex || !provider.valid || !provider.alive
            || provider.side != actorSide || ability <= 20
            || ability <= requiredState - 30) {
            continue;
        }
        return static_cast<int>(i);
    }
    return std::nullopt;
}

}

bool shouldRestForStamina(const AiResourceState &state) {
    return state.stamina < 10;
}

bool shouldRetreatForHealth(const AiResourceState &state, RandomSource &random) {
    if (random.next(10) >= 5) { return false; }
    if (state.hp < 20) { return true; }
    if (state.hp < state.maxHp / 4 && random.next(10) < 6) { return true; }
    return state.hp < state.maxHp / 5 && random.next(10) < 8;
}

bool shouldAttemptHealthRecovery(const AiResourceState &state, RandomSource &random) {
    if (state.hp < 20 || state.hurt > 50) { return true; }
    return shouldAttemptResourceRecovery(
        state.hp, state.maxHp, 3, 5, 7, 9, random);
}

bool shouldAttemptSelfDepoison(const AiResourceState &state, RandomSource &random) {
    return random.next(10) < state.poisoned / 10;
}

bool canSelfMedic(int medic, int stamina, int hurt) noexcept {
    /* Z.DAT:0x33C4D rejects medic < 20, stamina < 50, or medic <= hurt - 30. */
    return medic >= 20 && stamina >= 50 && medic > hurt - 30;
}

bool canSelfDepoison(int depoison, int stamina, int poisoned) noexcept {
    /* Z.DAT:0x33E93 uses strict comparisons for both ability and stamina. */
    return depoison > 20 && stamina > 50 && depoison > poisoned - 30;
}

bool shouldAttemptMpRecovery(const AiResourceState &state, RandomSource &random) {
    return shouldAttemptResourceRecovery(
        state.mp, state.maxMp, 2, 4, 6, 8, random);
}

bool shouldAttemptMedicSupport(const AiResourceState &state, RandomSource &random) {
    return shouldAttemptSupport(state.medic, state.stamina, random);
}

bool shouldAttemptDepoisonSupport(const AiResourceState &state, RandomSource &random) {
    return shouldAttemptSupport(state.depoison, state.stamina, random);
}

std::optional<int> chooseMedicSupportTarget(
    int actorIndex, int medic, const std::vector<AiAllyState> &allies,
    RandomSource &random) {
    return chooseSupportTarget(
        actorIndex, allies,
        [medic](const AiAllyState &target) {
            return medic > target.hurt - 30;
        },
        [&random](const AiAllyState &target) {
            return needsMedicSupport(target, random);
        });
}

std::optional<int> chooseDepoisonSupportTarget(
    int actorIndex, int depoison, const std::vector<AiAllyState> &allies,
    RandomSource &random) {
    return chooseSupportTarget(
        actorIndex, allies,
        [depoison](const AiAllyState &target) {
            return depoison > target.poisoned - 30;
        },
        [&random](const AiAllyState &target) {
            return needsDepoisonSupport(target, random);
        });
}

std::optional<int> chooseMedicProvider(
    int actorIndex, int actorHurt, const std::vector<AiAllyState> &allies) {
    return chooseSupportProvider(
        actorIndex, actorHurt, allies,
        [](const AiAllyState &provider) { return provider.medic; });
}

std::optional<int> chooseDepoisonProvider(
    int actorIndex, int actorPoisoned, const std::vector<AiAllyState> &allies) {
    return chooseSupportProvider(
        actorIndex, actorPoisoned, allies,
        [](const AiAllyState &provider) { return provider.depoison; });
}

AiSupportFallback chooseUnreachableSupportFallback(
    int attack, int allyPowerTotal, int allyCount) {
    if (allyCount <= 0) { return AiSupportFallback::Attack; }
    const auto doubledAttack = 2LL * attack;
    const auto doubledAverage = 2LL * allyPowerTotal / allyCount;
    return doubledAttack <= doubledAverage
        ? AiSupportFallback::Rest : AiSupportFallback::Attack;
}

AiPowerSummary summarizeAllyPower(
    int side, const std::vector<AiAllyState> &allies) {
    AiPowerSummary summary;
    for (const auto &ally: allies) {
        if (ally.side != side) { continue; }
        summary.total += ally.attack + ally.hp;
        ++summary.count;
    }
    return summary;
}

int originalActionCode(AiResourceAction action, bool usesItem) {
    if (usesItem) { return 6; }
    switch (action) {
    case AiResourceAction::Rest:
        return 7;
    case AiResourceAction::RecoverHp:
    case AiResourceAction::MedicSupport:
        return 5;
    case AiResourceAction::SelfDepoison:
    case AiResourceAction::DepoisonSupport:
        return 4;
    case AiResourceAction::RequestMedic:
        return 8;
    case AiResourceAction::RequestDepoison:
        return 9;
    default:
        return 0;
    }
}

AiResourceAction chooseAiResourceAction(const AiResourceState &state,
                                        RandomSource &random,
                                        const AiResourceResolver &resolve) {
    auto fallback = shouldRestForStamina(state)
        ? AiResourceAction::Rest
        : AiResourceAction::None;

    if (shouldAttemptHealthRecovery(state, random)) {
        const auto resolved = resolve(AiResourceAction::RecoverHp);
        if (resolved != AiResourceAction::None) { return resolved; }
        fallback = AiResourceAction::None;
    }
    if (fallback != AiResourceAction::None) { return fallback; }

    if (shouldAttemptSelfDepoison(state, random)) {
        const auto resolved = resolve(AiResourceAction::SelfDepoison);
        if (resolved != AiResourceAction::None) { return resolved; }
    }
    if (shouldAttemptMpRecovery(state, random)) {
        const auto resolved = resolve(AiResourceAction::RecoverMp);
        if (resolved != AiResourceAction::None) { return resolved; }
    }
    if (shouldAttemptMedicSupport(state, random)) {
        const auto resolved = resolve(AiResourceAction::MedicSupport);
        if (resolved != AiResourceAction::None) { return resolved; }
    }
    if (shouldAttemptDepoisonSupport(state, random)) {
        const auto resolved = resolve(AiResourceAction::DepoisonSupport);
        if (resolved != AiResourceAction::None) { return resolved; }
    }
    return AiResourceAction::None;
}

AiResourceAction chooseAiResourceAction(const AiResourceState &state,
                                        const AiResourceOptions &options,
                                        RandomSource &random) {
    return chooseAiResourceAction(state, random, [&options](AiResourceAction action) {
        switch (action) {
        case AiResourceAction::RecoverHp:
            return options.healthRecovery ? action : AiResourceAction::None;
        case AiResourceAction::SelfDepoison:
            return options.selfDepoison ? action : AiResourceAction::None;
        case AiResourceAction::RecoverMp:
            return options.mpRecovery ? action : AiResourceAction::None;
        case AiResourceAction::MedicSupport:
            return options.medicSupport ? action : AiResourceAction::None;
        case AiResourceAction::DepoisonSupport:
            return options.depoisonSupport ? action : AiResourceAction::None;
        default:
            return AiResourceAction::None;
        }
    });
}

}
