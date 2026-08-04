#include "ai_strategy.hh"

#include <algorithm>
#include <cstdlib>
#include <limits>

namespace hojy::battle {

namespace {

AiFollowupDecision makeDecision(AiFollowupAction action, int targetIndex = -1,
                                int selectionIndex = -1) {
    return AiFollowupDecision{action, targetIndex, selectionIndex};
}

long long absoluteValue(int value) {
    const auto wide = static_cast<long long>(value);
    return wide < 0 ? -wide : wide;
}

bool supportGatePasses(const AiStrategyActor &actor,
                       const std::vector<AiStrategyCharacter> &characters) {
    long long allyTotal = 0;
    long long enemyTotal = 0;
    int allyCount = 0;
    int enemyCount = 0;

    for (const auto &character: characters) {
        /* Z.DAT:sub_33599 totals every battle slot by side.  A defeated
         * participant contributes zero HP but its base attack remains part of
         * the side-strength comparison; validity only gates action targets. */
        const auto power = static_cast<long long>(character.attack)
            + character.hp;
        if (character.side == actor.side) {
            allyTotal += power;
            ++allyCount;
        } else {
            enemyTotal += power;
            ++enemyCount;
        }
    }

    if (allyCount <= 0 || enemyCount <= 0) { return false; }

    // Preserve the original integer-division order: enemyTotal / count / 2.
    return enemyTotal / enemyCount / 2
            > static_cast<long long>(actor.attack) + actor.hp
        && allyTotal > 2 * enemyTotal;
}

int choosePoisonedTarget(int actorIndex, const AiStrategyActor &actor,
                         const std::vector<AiStrategyCharacter> &characters) {
    int selected = -1;
    int highestPoison = 0;
    for (std::size_t i = 0; i < characters.size(); ++i) {
        const auto &target = characters[i];
        if (static_cast<int>(i) == actorIndex || !target.valid || !target.alive
            || target.side != actor.side || target.poison <= 0) {
            continue;
        }
        if (target.poison > highestPoison) {
            highestPoison = target.poison;
            selected = static_cast<int>(i);
        }
    }
    return selected;
}

int chooseMedicTarget(int actorIndex, const AiStrategyActor &actor,
                      const std::vector<AiStrategyCharacter> &characters) {
    int selected = -1;
    long long largestGap = 0;
    for (std::size_t i = 0; i < characters.size(); ++i) {
        const auto &target = characters[i];
        if (static_cast<int>(i) == actorIndex || !target.valid || !target.alive
            || target.side != actor.side || target.hp >= target.maxHp) {
            continue;
        }
        const auto gap = static_cast<long long>(target.maxHp) - target.hp;
        if (gap > largestGap) {
            largestGap = gap;
            selected = static_cast<int>(i);
        }
    }
    return selected;
}

template<typename Predicate, typename Better>
std::optional<int> chooseEnemy(
    int actorIndex,
    const AiStrategyActor &actor,
    const std::vector<AiStrategyCharacter> &characters,
    Predicate predicate,
    Better better) {
    std::optional<int> selected;
    for (std::size_t i = 0; i < characters.size(); ++i) {
        const auto &candidate = characters[i];
        if (static_cast<int>(i) == actorIndex || !candidate.valid
            || !candidate.alive || candidate.side == actor.side
            || !predicate(candidate)) {
            continue;
        }
        if (!selected || better(candidate, characters[*selected])) {
            selected = static_cast<int>(i);
        }
    }
    return selected;
}

std::optional<int> chooseHighestAttack(
    int actorIndex,
    const AiStrategyActor &actor,
    const std::vector<AiStrategyCharacter> &characters) {
    std::optional<int> selected;
    int highestAttack = 0;
    for (std::size_t i = 0; i < characters.size(); ++i) {
        const auto &candidate = characters[i];
        if (static_cast<int>(i) == actorIndex || !candidate.valid
            || !candidate.alive || candidate.side == actor.side) {
            continue;
        }
        if (candidate.attack > highestAttack) {
            highestAttack = candidate.attack;
            selected = static_cast<int>(i);
        }
    }
    return selected;
}

std::optional<int> chooseLowestAttack(
    int actorIndex,
    const AiStrategyActor &actor,
    const std::vector<AiStrategyCharacter> &characters) {
    std::optional<int> selected;
    int lowestAttack = 1000;
    for (std::size_t i = 0; i < characters.size(); ++i) {
        const auto &candidate = characters[i];
        if (static_cast<int>(i) == actorIndex || !candidate.valid
            || !candidate.alive || candidate.side == actor.side) {
            continue;
        }
        if (candidate.attack < lowestAttack) {
            lowestAttack = candidate.attack;
            selected = static_cast<int>(i);
        }
    }
    return selected;
}

std::optional<int> chooseAbilityTarget(
    int actorIndex,
    const AiStrategyActor &actor,
    const std::vector<AiStrategyCharacter> &characters) {
    bool allyHasPoisonTechnique = false;
    for (const auto &character: characters) {
        // The original scan intentionally does not filter dead slots.
        if (character.side == actor.side && character.poisonTechnique > 20) {
            allyHasPoisonTechnique = true;
            break;
        }
    }
    if (!allyHasPoisonTechnique) {
        return chooseLowestAttack(actorIndex, actor, characters);
    }

    int highestAbility = 0;
    std::optional<int> selected;
    bool sufficientlyStrongDepoison = false;
    for (std::size_t i = 0; i < characters.size(); ++i) {
        const auto &candidate = characters[i];
        if (static_cast<int>(i) == actorIndex || !candidate.valid
            || !candidate.alive || candidate.side == actor.side) {
            continue;
        }
        if (candidate.depoison > highestAbility) {
            highestAbility = candidate.depoison;
            selected = static_cast<int>(i);
            if (candidate.depoison >= 20) {
                sufficientlyStrongDepoison = true;
            }
        }
    }

    bool sufficientlyStrongMedic = false;
    if (!sufficientlyStrongDepoison) {
        // Keep highestAbility as the comparison baseline.  The original
        // routine does not reset it before its medical scan.
        for (std::size_t i = 0; i < characters.size(); ++i) {
            const auto &candidate = characters[i];
            if (static_cast<int>(i) == actorIndex || !candidate.valid
                || !candidate.alive || candidate.side == actor.side) {
                continue;
            }
            if (candidate.medic > highestAbility) {
                highestAbility = candidate.medic;
                selected = static_cast<int>(i);
                if (candidate.medic >= 20) {
                    sufficientlyStrongMedic = true;
                }
            }
        }
    }

    // A selected ability target is only accepted when the medical scan set
    // its success flag.  A strong depoison target still falls through to the
    // original lowest-attack fallback (a preserved DOS logic defect).
    return sufficientlyStrongMedic
        ? selected
        : chooseLowestAttack(actorIndex, actor, characters);
}

bool chooseNpcThrow(const AiStrategyActor &actor,
                    const std::vector<AiThrowingOption> &throwingItems,
    RandomSource &random, int &selectionIndex) {
    for (const auto &item: throwingItems) {
        if (item.itemType == 1 || item.itemType == 2) { continue; }
        if (item.addHp < 0 && absoluteValue(item.addHp) > actor.attack) {
            if (random.next(10) < 6) {
                selectionIndex = item.selectionIndex;
                return true;
            }
        }
        if (item.addPoisoned > 0 && item.addPoisoned > actor.attack) {
            if (random.next(10) < 3) {
                selectionIndex = item.selectionIndex;
                return true;
            }
        }
    }
    return false;
}

bool choosePlayerThrow(const AiStrategyActor &actor,
                       const std::vector<AiThrowingOption> &throwingItems,
                       RandomSource &random, int &selectionIndex) {
    const auto threshold = 3LL * static_cast<long long>(actor.attack) / 2;
    for (const auto &item: throwingItems) {
        if (item.itemType == 1 || item.itemType == 2) { continue; }
        if (item.addHp < 0 && absoluteValue(item.addHp) > threshold) {
            if (random.next(actor.throwing) > 20) {
                selectionIndex = item.selectionIndex;
                return true;
            }
        }
        if (item.addPoisoned > 0 && item.addPoisoned > threshold) {
            if (random.next(10) < 3) {
                selectionIndex = item.selectionIndex;
                return true;
            }
        }
    }
    return false;
}

}

AiFollowupDecision chooseAiFollowupAction(
    int actorIndex,
    const AiStrategyActor &actor,
    const std::vector<AiStrategyCharacter> &characters,
    const std::vector<AiThrowingOption> &throwingItems,
    const std::vector<AiSkillOption> &skills,
    RandomSource &random) {
    if (supportGatePasses(actor, characters)) {
        if (actor.medic < 20 || actor.stamina < 50) {
            if (actor.depoison >= 20 && actor.stamina >= 50) {
                const auto target = choosePoisonedTarget(actorIndex, actor, characters);
                if (target >= 0) {
                    return makeDecision(AiFollowupAction::DepoisonSupport, target);
                }
            }
        } else {
            const auto target = chooseMedicTarget(actorIndex, actor, characters);
            if (target >= 0) {
                return makeDecision(AiFollowupAction::MedicSupport, target);
            }
        }
    }

    const auto r50 = random.next(50);
    if (static_cast<long long>(actor.poison) - actor.attack > r50) {
        const auto r150 = random.next(150);
        if (r150 < actor.poison) {
            return makeDecision(AiFollowupAction::Poison);
        }
    }

    int selectionIndex = -1;
    const auto throwing = actor.side != 0
        ? chooseNpcThrow(actor, throwingItems, random, selectionIndex)
        : choosePlayerThrow(actor, throwingItems, random, selectionIndex);
    if (throwing) {
        return makeDecision(AiFollowupAction::Throw, -1, selectionIndex);
    }

    if (actor.stamina <= 10) {
        return makeDecision(AiFollowupAction::Rest);
    }

    int positiveSkillCount = 0;
    int minimumReqMp = std::numeric_limits<int>::max();
    for (const auto &skill: skills) {
        if (skill.skillId <= 0) { continue; }
        ++positiveSkillCount;
        if (skill.reqMp < minimumReqMp) { minimumReqMp = skill.reqMp; }
    }
    if (positiveSkillCount == 0 || actor.mp < minimumReqMp) {
        return makeDecision(AiFollowupAction::Rest);
    }
    return makeDecision(AiFollowupAction::Skill);
}

std::optional<int> chooseNearestAiTarget(
    int actorIndex,
    int actorSide,
    const std::vector<AiStrategyCharacter> &characters,
    const AiPathDistance &pathDistance,
    int excludedIndex) {
    std::optional<int> selected;
    // sub_35372 starts from 1000 and replaces only on a strict decrease.
    auto selectedDistance = 1000;
    for (std::size_t i = 0; i < characters.size(); ++i) {
        const auto &candidate = characters[i];
        if (static_cast<int>(i) == actorIndex
            || static_cast<int>(i) == excludedIndex
            || !candidate.valid || !candidate.alive
            || candidate.side == actorSide) {
            continue;
        }
        const auto distance = pathDistance
            ? pathDistance(static_cast<int>(i)) : -1;
        if (distance < 0 || distance >= selectedDistance) { continue; }
        selected = static_cast<int>(i);
        selectedDistance = distance;
    }
    return selected;
}

std::optional<int> chooseAiTarget(
    int actorIndex,
    const AiStrategyActor &actor,
    const std::vector<AiStrategyCharacter> &characters,
    RandomSource &random,
    const AiPathDistance &pathDistance) {
    // The DOS helpers can leave the previous target slot unchanged when a
    // triggered branch finds no candidate.  Treat an empty result as a failed
    // branch and continue the documented cascade instead of reusing stale
    // battle state.
    if (actor.integrity >= 75 && random.next(10) < 7) {
        if (const auto target = chooseHighestAttack(
                actorIndex, actor, characters); target) {
            return target;
        }
    }
    if (actor.integrity <= 25 && random.next(10) < 7) {
        if (const auto target = chooseLowestAttack(
                actorIndex, actor, characters); target) {
            return target;
        }
    }
    if (actor.potential >= 70 && random.next(10) < 7) {
        if (const auto target = chooseAbilityTarget(
                actorIndex, actor, characters); target) {
            return target;
        }
    }

    return chooseNearestAiTarget(
        actorIndex, actor.side, characters, pathDistance);
}

std::optional<int> choosePoisonTarget(
    int actorIndex,
    const AiStrategyActor &actor,
    const std::vector<AiStrategyCharacter> &characters,
    RandomSource &random,
    const AiPathDistance &pathDistance) {
    auto eligible = [&actor](const AiStrategyCharacter &candidate) {
        return candidate.poison < 95 && candidate.antipoison < actor.poison;
    };

    if (actor.potential > 60 && random.next(10) < 7) {
        const auto strongest = chooseEnemy(
            actorIndex, actor, characters,
            [&eligible](const AiStrategyCharacter &candidate) {
                return eligible(candidate) && candidate.attack > 0;
            },
            [](const AiStrategyCharacter &candidate,
               const AiStrategyCharacter &selected) {
                return candidate.attack > selected.attack;
            });
        if (strongest) { return strongest; }
    }

    std::optional<int> selected;
    auto selectedDistance = 1000;
    for (std::size_t i = 0; i < characters.size(); ++i) {
        const auto &candidate = characters[i];
        if (static_cast<int>(i) == actorIndex || !candidate.valid
            || !candidate.alive || candidate.side == actor.side
            || !eligible(candidate)) {
            continue;
        }
        const auto distance = pathDistance ? pathDistance(static_cast<int>(i))
                                           : std::numeric_limits<int>::max();
        if (distance < 0) { continue; }
        if (distance < selectedDistance) {
            selected = static_cast<int>(i);
            selectedDistance = distance;
        }
    }
    return selected;
}

std::optional<int> chooseOriginalSkillSlot(
    const std::vector<AiSkillOption> &skills,
    RandomSource &random) {
    int positiveSkillCount = 0;
    for (const auto &skill: skills) {
        if (skill.skillId > 0) { ++positiveSkillCount; }
    }
    if (positiveSkillCount <= 0) { return std::nullopt; }

    // Z.DAT counts positive IDs and then maps the random ordinal back to the
    // corresponding real skill slot.  Accept both the original sparse array
    // and an already-compacted adapter so a missing slot can never be selected.
    const auto ordinal = random.next(positiveSkillCount);
    int positiveIndex = 0;
    for (std::size_t index = 0; index < skills.size(); ++index) {
        const auto &skill = skills[index];
        if (skill.skillId <= 0) { continue; }
        if (positiveIndex == ordinal) {
            return skill.slot >= 0 ? skill.slot : static_cast<int>(index);
        }
        ++positiveIndex;
    }
    return std::nullopt;
}

std::optional<std::pair<int, int>> chooseRetreatPosition(
    const SelectableCells &movementCells,
    int exactSteps,
    const std::vector<std::pair<int, int>> &enemyPositions) {
    if (enemyPositions.empty()) { return std::nullopt; }

    std::optional<std::pair<int, int>> selected;
    long long selectedScore = 0;
    for (const auto &[position, cell]: movementCells) {
        if (cell.moves != exactSteps) { continue; }
        long long score = 0;
        for (const auto &enemy: enemyPositions) {
            score += absoluteValue(position.first - enemy.first)
                   + absoluteValue(position.second - enemy.second);
        }
        // The original scans x first, then y and replaces only on strict >.
        if (score > selectedScore) {
            selected = position;
            selectedScore = score;
        }
    }
    return selected;
}

}
