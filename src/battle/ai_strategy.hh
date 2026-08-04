#pragma once

#include "battle/movement.hh"
#include "battle/random.hh"

#include <functional>
#include <optional>
#include <utility>
#include <vector>

namespace hojy::battle {

enum class AiFollowupAction {
    Rest,
    MedicSupport,
    DepoisonSupport,
    Poison,
    Throw,
    Skill,
};

struct AiStrategyCharacter {
    int side = 0;
    bool valid = false;
    bool alive = false;
    int x = 0;
    int y = 0;
    int hp = 0;
    int maxHp = 0;
    int attack = 0;
    int medic = 0;
    int poison = 0;
    int depoison = 0;
    int antipoison = 0;
    int poisonTechnique = 0;
};

struct AiStrategyActor {
    int side = 0;
    int hp = 0;
    int attack = 0;
    int stamina = 0;
    int mp = 0;
    int medic = 0;
    int poison = 0;
    int depoison = 0;
    int throwing = 0;
    int integrity = 0;
    int potential = 0;
};

struct AiThrowingOption {
    int selectionIndex = -1;
    int itemId = -1;
    int addHp = 0;
    int addPoisoned = 0;
    /* Consumables are item type 3; equipment/books (1/2) are not throwable. */
    int itemType = 3;
};

struct AiSkillOption {
    int slot = -1;
    int skillId = 0;
    int reqMp = 0;
};

struct AiFollowupDecision {
    AiFollowupAction action = AiFollowupAction::Rest;
    int targetIndex = -1;
    int selectionIndex = -1;
};

using AiPathDistance = std::function<int(int targetIndex)>;

/* Deterministic nearest-enemy scan used after a failed first cast attempt. */
std::optional<int> chooseNearestAiTarget(
    int actorIndex,
    int actorSide,
    const std::vector<AiStrategyCharacter> &characters,
    const AiPathDistance &pathDistance,
    int excludedIndex = -1);

AiFollowupDecision chooseAiFollowupAction(
    int actorIndex,
    const AiStrategyActor &actor,
    const std::vector<AiStrategyCharacter> &characters,
    const std::vector<AiThrowingOption> &throwingItems,
    const std::vector<AiSkillOption> &skills,
    RandomSource &random);

std::optional<int> chooseAiTarget(
    int actorIndex,
    const AiStrategyActor &actor,
    const std::vector<AiStrategyCharacter> &characters,
    RandomSource &random,
    const AiPathDistance &pathDistance);

std::optional<int> choosePoisonTarget(
    int actorIndex,
    const AiStrategyActor &actor,
    const std::vector<AiStrategyCharacter> &characters,
    RandomSource &random,
    const AiPathDistance &pathDistance);

std::optional<int> chooseOriginalSkillSlot(
    const std::vector<AiSkillOption> &skills,
    RandomSource &random);

std::optional<std::pair<int, int>> chooseRetreatPosition(
    const SelectableCells &movementCells,
    int exactSteps,
    const std::vector<std::pair<int, int>> &enemyPositions);

}
