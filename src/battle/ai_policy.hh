#pragma once

#include "battle/random.hh"

#include <functional>
#include <optional>
#include <vector>

namespace hojy::battle {

struct AiResourceState {
    int hp = 0;
    int maxHp = 0;
    int hurt = 0;
    int poisoned = 0;
    int stamina = 0;
    int mp = 0;
    int maxMp = 0;
    int medic = 0;
    int depoison = 0;
};

struct AiAllyState {
    int side = 0;
    bool valid = false;
    bool alive = false;
    int hp = 0;
    int maxHp = 0;
    int hurt = 0;
    int poisoned = 0;
    int medic = 0;
    int depoison = 0;
    /* Scene adapter request markers: 8 = medic, 9 = depoison. */
    int actionCode = 0;
    int attack = 0;
};

struct AiPowerSummary {
    int total = 0;
    int count = 0;
};

enum class AiResourceAction {
    None,
    Rest,
    RecoverHp,
    SelfDepoison,
    RecoverMp,
    RequestMedic,
    RequestDepoison,
    MedicSupport,
    DepoisonSupport,
};

enum class AiSupportFallback {
    Rest,
    Attack,
};

struct AiResourceOptions {
    bool healthRecovery = false;
    bool selfDepoison = false;
    bool mpRecovery = false;
    bool medicSupport = false;
    bool depoisonSupport = false;
};

using AiResourceResolver = std::function<AiResourceAction(AiResourceAction)>;

bool shouldRestForStamina(const AiResourceState &state);
bool shouldRetreatForHealth(const AiResourceState &state, RandomSource &random);
bool shouldAttemptHealthRecovery(const AiResourceState &state, RandomSource &random);
bool shouldAttemptSelfDepoison(const AiResourceState &state, RandomSource &random);
bool canSelfMedic(int medic, int stamina, int hurt) noexcept;
bool canSelfDepoison(int depoison, int stamina, int poisoned) noexcept;
bool shouldAttemptMpRecovery(const AiResourceState &state, RandomSource &random);
bool shouldAttemptMedicSupport(const AiResourceState &state, RandomSource &random);
bool shouldAttemptDepoisonSupport(const AiResourceState &state, RandomSource &random);
std::optional<int> chooseMedicSupportTarget(
    int actorIndex, int medic, const std::vector<AiAllyState> &allies,
    RandomSource &random);
std::optional<int> chooseDepoisonSupportTarget(
    int actorIndex, int depoison, const std::vector<AiAllyState> &allies,
    RandomSource &random);
std::optional<int> chooseMedicProvider(
    int actorIndex, int actorHurt, const std::vector<AiAllyState> &allies);
std::optional<int> chooseDepoisonProvider(
    int actorIndex, int actorPoisoned, const std::vector<AiAllyState> &allies);
AiSupportFallback chooseUnreachableSupportFallback(
    int attack, int allyPowerTotal, int allyCount);
AiPowerSummary summarizeAllyPower(
    int side, const std::vector<AiAllyState> &allies);
int originalActionCode(AiResourceAction action, bool usesItem);
AiResourceAction chooseAiResourceAction(const AiResourceState &state,
                                        const AiResourceOptions &options,
                                        RandomSource &random);
AiResourceAction chooseAiResourceAction(const AiResourceState &state,
                                        RandomSource &random,
                                        const AiResourceResolver &resolve);

}
