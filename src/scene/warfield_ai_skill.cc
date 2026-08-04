#include "warfield.hh"

#include "battle/ai_policy.hh"
#include "battle/ai_strategy.hh"
#include "battle/combat_rules.hh"
#include "battle/game_random.hh"
#include "battle/movement.hh"
#include "battle/turn_order.hh"
#include "content/constants.hh"
#include "itemview.hh"
#include "world/action.hh"
#include "world/bag.hh"
#include "world/savedata.hh"

#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace hojy::scene {
void Warfield::autoActionSkill(
    CharInfo *ch, int actorIndex,
    const battle::AiStats &actorAiStats,
    const battle::AiResourceState &resourceState,
    const std::vector<battle::AiAllyState> &allies,
    const battle::AiPowerSummary &allyPower,
    bool resumeAutoAttack, bool requestSupport,
    bool supportWithoutPosition,
    battle::AiResourceAction resourceAction,
    battle::RandomSource &resourceRandom) {
    const auto currentAiStats = [](const CharInfo &actor) {
        return battle::resolveAiRuntimeStats(
            actor.aiEntryStats, actor.aiEquipmentBonusStats, actor.info);
    };
    using Position = std::pair<int, int>;
    const auto onMap = [this](Position position) {
        return position.first >= 0 && position.first < mapWidth_
            && position.second >= 0 && position.second < mapHeight_;
    };
    const auto terrainDistance = [this](Position from, Position target) {
        return battle::terrainPathDistance(
            mapWidth_, mapHeight_, from, target,
            [this](int x, int y) {
                return cellInfo_[y * mapWidth_ + x].blocked;
            });
    };
    const auto canCastAtCurrentPosition = [this, ch, onMap, terrainDistance](
                                               int targetIndex,
                                               int attackAreaType,
                                               int range) {
        if (targetIndex < 0
            || targetIndex >= static_cast<int>(chars_.size())
            || !onMap({ch->x, ch->y})) {
            return false;
        }
        const auto &target = chars_[targetIndex];
        if (target.info.hp <= 0 || !onMap({target.x, target.y})) {
            return false;
        }
        const Position actorPosition{ch->x, ch->y};
        const Position targetPosition{target.x, target.y};
        return battle::canCastFromPosition(
            attackAreaType, range,
            terrainDistance(actorPosition, targetPosition),
            actorPosition, targetPosition);
    };
    const auto buildCastRangeCells = [this, onMap](
                                           Position targetPosition,
                                           Position actorPosition,
                                           int range) {
        std::map<Position, SelectableCell> castRangeCells;
        if (!onMap(targetPosition) || !onMap(actorPosition)) {
            return castRangeCells;
        }
        battle::getCastRangeArea(
            mapWidth_, mapHeight_, targetPosition, std::max(0, range),
            castRangeCells,
            [this](int x, int y) {
                return cellInfo_[y * mapWidth_ + x].blocked;
            },
            [this, targetPosition, actorPosition](int x, int y) {
                const Position position{x, y};
                return position != targetPosition && position != actorPosition
                    && cellInfo_[y * mapWidth_ + x].charInfo != nullptr;
            });
        return castRangeCells;
    };
    std::vector<battle::AiStrategyCharacter> strategyCharacters;
    strategyCharacters.reserve(chars_.size());
    for (const auto &candidate: chars_) {
        const auto validPosition = candidate.x >= 0 && candidate.x < mapWidth_
            && candidate.y >= 0 && candidate.y < mapHeight_;
        const auto candidateAiStats = currentAiStats(candidate);
        strategyCharacters.push_back(battle::AiStrategyCharacter{
            candidate.side, candidate.id >= 0 && validPosition,
            candidate.info.hp > 0 && validPosition,
            candidate.x, candidate.y, candidate.info.hp, candidate.info.maxHp,
            candidateAiStats.attack, candidateAiStats.medic,
            candidate.info.poisoned, candidateAiStats.depoison,
            candidateAiStats.antipoison, candidateAiStats.poison,
        });
    }
    battle::AiStrategyActor strategyActor;
    strategyActor.side = ch->side;
    strategyActor.hp = ch->info.hp;
    strategyActor.attack = actorAiStats.attack;
    strategyActor.stamina = ch->info.stamina;
    strategyActor.mp = ch->info.mp;
    strategyActor.medic = actorAiStats.medic;
    strategyActor.poison = actorAiStats.poison;
    strategyActor.depoison = actorAiStats.depoison;
    strategyActor.throwing = actorAiStats.throwing;
    strategyActor.integrity = actorAiStats.integrity;
    strategyActor.potential = actorAiStats.potential;

    std::vector<battle::AiSkillOption> strategySkills;
    strategySkills.reserve(::hojy::content::LearnSkillCount);
    for (int i = 0; i < ::hojy::content::LearnSkillCount; ++i) {
        const auto skillId = ch->info.skillId[i];
        if (skillId <= 0) { continue; }
        const auto *skill = ::hojy::world::state::gSaveData.skillInfo[skillId];
        if (!skill) { continue; }
        strategySkills.push_back(battle::AiSkillOption{
            i, skillId, skill->reqMp,
        });
    }

    std::vector<battle::AiThrowingOption> throwingItems;
    if (ch->side != 0) {
        for (int i = 0; i < ::hojy::content::CarryItemCount; ++i) {
            const auto itemId = ch->info.item[i];
            if (itemId < 0 || ch->info.itemCount[i] <= 0) { continue; }
            const auto *item = ::hojy::world::state::gSaveData.itemInfo[itemId];
            if (!item) { continue; }
            throwingItems.push_back(battle::AiThrowingOption{
                i, itemId, item->addHp, item->addPoisoned, item->itemType,
            });
        }
    } else {
        int bagIndex = 0;
        for (const auto &[itemId, count]: battleBag_.orderedItems()) {
            if (itemId < 0 || count <= 0) { continue; }
            const auto *item = ::hojy::world::state::gSaveData.itemInfo[itemId];
            if (!item) { continue; }
            throwingItems.push_back(battle::AiThrowingOption{
                bagIndex++, itemId, item->addHp, item->addPoisoned, item->itemType,
            });
        }
    }

    const auto pathDistance = [this, ch, &strategyCharacters](int targetIndex) {
        if (targetIndex < 0
            || targetIndex >= static_cast<int>(strategyCharacters.size())) {
            return -1;
        }
        const auto &target = strategyCharacters[targetIndex];
        if (!target.valid || !target.alive) { return -1; }
        const std::pair<int, int> targetPosition{target.x, target.y};
        return battle::terrainPathDistance(
            mapWidth_, mapHeight_, {ch->x, ch->y}, targetPosition,
            [this](int x, int y) {
                return cellInfo_[y * mapWidth_ + x].blocked;
            });
    };

    auto runAtPosition = [this, ch](
                             const std::map<std::pair<int, int>, SelectableCell> &cells,
                             const std::pair<int, int> &position,
                             std::function<void()> action) {
        pendingAutoAction_ = [this, ch, action = std::move(action)]() mutable {
            if (currentActor_ != ch) { return; }
            if (ch->info.hp <= 0) {
                endTurn(ch);
                return;
            }
            action();
        };
        if (position != std::make_pair<int, int>(ch->x, ch->y)) {
            const auto found = cells.find(position);
            if (found == cells.end()) {
                pendingAutoAction_ = nullptr;
                if (currentActor_ == ch) { endTurn(ch); }
                return false;
            }
            movingPath_.clear();
            auto *cell = &found->second;
            while (cell) {
                movingPath_.emplace_back(cell->x, cell->y);
                cell = cell->moveParent;
            }
            stage_ = Moving;
            return true;
        }
        battle::runPendingAction(pendingAutoAction_);
        return true;
    };

    struct CastPositionPlan {
        std::map<Position, SelectableCell> movementCells;
        std::optional<Position> position;
        bool expectedInRange = false;
    };
    auto chooseCastPosition = [this, ch, buildCastRangeCells,
                               canCastAtCurrentPosition, terrainDistance](
                                  int targetIndex, int range,
                                  int attackAreaType,
                                  battle::CastMovementMode mode) {
        CastPositionPlan plan;
        getSelectableArea(ch, plan.movementCells, ch->steps, 0);
        if (targetIndex < 0
            || targetIndex >= static_cast<int>(chars_.size())) {
            return plan;
        }
        const auto &target = chars_[targetIndex];
        const Position targetPosition{target.x, target.y};
        const Position actorPosition{ch->x, ch->y};
        const auto castRangeCells = buildCastRangeCells(
            targetPosition, actorPosition, range);
        const auto choice = battle::chooseCastMovementPosition(
            plan.movementCells, castRangeCells, actorPosition, targetPosition,
            range, mode,
            canCastAtCurrentPosition(targetIndex, attackAreaType, range),
            terrainDistance);
        plan.position = choice.position;
        plan.expectedInRange = choice.expectedInRange;
        return plan;
    };

    const auto forceSkill = resumeAutoAttack || requestSupport || supportWithoutPosition;
    bool preserveSupportFallback = false;
    if (!forceSkill && resourceAction == battle::AiResourceAction::None
        && battle::shouldRetreatForHealth(resourceState, resourceRandom)) {
        std::map<std::pair<int, int>, SelectableCell> movementCells;
        getSelectableArea(ch, movementCells, ch->steps, 0);
        std::vector<std::pair<int, int>> enemyPositions;
        for (const auto &candidate: chars_) {
            if (candidate.id >= 0 && candidate.side != ch->side
                && candidate.x >= 0 && candidate.x < mapWidth_
                && candidate.y >= 0 && candidate.y < mapHeight_) {
                enemyPositions.emplace_back(candidate.x, candidate.y);
            }
        }
        const auto retreatPosition = battle::chooseRetreatPosition(
            movementCells, ch->steps, enemyPositions);
        if (retreatPosition) {
            runAtPosition(movementCells, *retreatPosition,
                          [this]() { doRest(); });
        } else {
            doRest();
        }
        return;
    }

    auto followup = forceSkill
        ? battle::AiFollowupDecision{battle::AiFollowupAction::Skill, -1, -1}
        : battle::chooseAiFollowupAction(
            actorIndex, strategyActor, strategyCharacters,
            throwingItems, strategySkills, resourceRandom);

    if (followup.action == battle::AiFollowupAction::Rest) {
        ch->actionCode = 7;
        doRest();
        return;
    }

    if (followup.action == battle::AiFollowupAction::MedicSupport
        || followup.action == battle::AiFollowupAction::DepoisonSupport) {
        const auto medic = followup.action == battle::AiFollowupAction::MedicSupport;
        const auto range = battle::calcTechniqueRange(
            medic ? actorAiStats.medic : actorAiStats.depoison);
        ch->actionCode = medic ? 5 : 4;
        auto plan = chooseCastPosition(
            followup.targetIndex, range, 0,
            battle::CastMovementMode::Approach);
        if (plan.position) {
            const auto targetIndex = followup.targetIndex;
            const auto castCheck = canCastAtCurrentPosition;
            auto *target = targetIndex >= 0
                && targetIndex < static_cast<int>(chars_.size())
                ? &chars_[targetIndex] : nullptr;
            runAtPosition(
                plan.movementCells, *plan.position,
                [this, ch, target, targetIndex, medic, range, castCheck,
                 allyPower, actorAiStats]() {
                    if (!target || target->info.hp <= 0
                        || !castCheck(targetIndex, 0, range)) {
                        if (battle::chooseUnreachableSupportFallback(
                                actorAiStats.attack, allyPower.total, allyPower.count)
                            == battle::AiSupportFallback::Rest) {
                            doRest(ch);
                        } else {
                            /* Preserve action code 4/5 and let the next
                             * invocation enter the ordinary skill path. */
                            resumeAutoAttack_ = true;
                        }
                        return;
                    }
                    actIndex_ = -1;
                    actId_ = medic ? -1 : -2;
                    actLevel_ = 0;
                    actItemSlot_ = -1;
                    attackTimesLeft_ = 1;
                    cursorX_ = target->x;
                    cursorY_ = target->y;
                    startActAction();
                });
            return;
        }
        if (battle::chooseUnreachableSupportFallback(
                actorAiStats.attack, allyPower.total, allyPower.count)
            == battle::AiSupportFallback::Rest) {
            doRest();
            return;
        }
        preserveSupportFallback = true;
        followup.action = battle::AiFollowupAction::Skill;
    }

    if (followup.action == battle::AiFollowupAction::Poison) {
        const auto targetIndex = battle::choosePoisonTarget(
            actorIndex, strategyActor, strategyCharacters,
            resourceRandom, pathDistance);
        if (!targetIndex) {
            /* Z.DAT:sub_3540E jumps directly to random-skill selection when
             * no eligible poison target exists. */
            followup.action = battle::AiFollowupAction::Skill;
        } else {
            const auto range = battle::calcTechniqueRange(actorAiStats.poison);
            ch->actionCode = 3;
            auto plan = chooseCastPosition(
                *targetIndex, range, 0,
                ch->steps > 0 ? battle::CastMovementMode::Reposition
                              : battle::CastMovementMode::Approach);
            if (plan.position) {
                const auto selectedTargetIndex = *targetIndex;
                const auto castCheck = canCastAtCurrentPosition;
                auto *target = &chars_[selectedTargetIndex];
                runAtPosition(
                    plan.movementCells, *plan.position,
                    [this, ch, target, selectedTargetIndex, range, castCheck,
                     allyPower, actorAiStats]() {
                        if (!target || target->info.hp <= 0
                            || target->side == ch->side
                            || !castCheck(selectedTargetIndex, 0, range)) {
                            /* sub_3540E uses the same team-power split when
                             * repositioning still cannot reach the target. */
                            if (battle::chooseUnreachableSupportFallback(
                                    actorAiStats.attack, allyPower.total,
                                    allyPower.count)
                                == battle::AiSupportFallback::Rest) {
                                doRest(ch);
                            } else {
                                resumeAutoAttack_ = true;
                            }
                            return;
                        }
                        actIndex_ = -1;
                        actId_ = -3;
                        actLevel_ = 0;
                        actItemSlot_ = -1;
                        attackTimesLeft_ = 1;
                        cursorX_ = target->x;
                        cursorY_ = target->y;
                        startActAction();
                    });
                return;
            }
            if (battle::chooseUnreachableSupportFallback(
                    actorAiStats.attack, allyPower.total, allyPower.count)
                == battle::AiSupportFallback::Rest) {
                doRest();
                return;
            }
            followup.action = battle::AiFollowupAction::Skill;
        }
    }

    if (followup.action == battle::AiFollowupAction::Throw) {
        const auto targetIndex = battle::chooseAiTarget(
            actorIndex, strategyActor, strategyCharacters,
            resourceRandom, pathDistance);
        const auto item = std::find_if(
            throwingItems.begin(), throwingItems.end(),
            [&followup](const battle::AiThrowingOption &candidate) {
                return candidate.selectionIndex == followup.selectionIndex;
            });
        if (targetIndex && item != throwingItems.end()) {
            const auto range = battle::calcTechniqueRange(actorAiStats.throwing);
            ch->actionCode = 10;
            auto plan = chooseCastPosition(
                *targetIndex, range, 0,
                battle::CastMovementMode::Approach);
            if (plan.position) {
                const auto selectedTargetIndex = *targetIndex;
                const auto castCheck = canCastAtCurrentPosition;
                auto *target = &chars_[selectedTargetIndex];
                const auto itemId = item->itemId;
                const auto itemSlot = ch->side != 0 ? item->selectionIndex : -1;
                runAtPosition(
                    plan.movementCells, *plan.position,
                    [this, ch, target, selectedTargetIndex, range, castCheck,
                     itemId, itemSlot]() {
                        if (!target || target->info.hp <= 0
                            || target->side == ch->side
                            || !castCheck(selectedTargetIndex, 0, range)) {
                            /* sub_3582B falls through to random skills after
                             * a failed post-move throw check. */
                            resumeAutoAttack_ = true;
                            return;
                        }
                        actIndex_ = itemId;
                        actId_ = -4;
                        actLevel_ = 0;
                        actItemSlot_ = itemSlot;
                        attackTimesLeft_ = 1;
                        cursorX_ = target->x;
                        cursorY_ = target->y;
                        startActAction();
                    });
                return;
            }
        }
        followup.action = battle::AiFollowupAction::Skill;
    }

    const auto skillSlot = battle::chooseOriginalSkillSlot(
        strategySkills, resourceRandom);
    if (!skillSlot || *skillSlot < 0 || *skillSlot >= ::hojy::content::LearnSkillCount) {
        doRest();
        return;
    }
    const auto skillId = ch->info.skillId[*skillSlot];
    const auto *skill = skillId > 0 ? ::hojy::world::state::gSaveData.skillInfo[skillId] : nullptr;
    if (!skill) {
        doRest();
        return;
    }
    const auto storedSkillLevel = ch->info.skillLevel[*skillSlot];
    const auto skillLevels = battle::resolveAiSkillLevels(
        skill->reqMp, storedSkillLevel, ch->info.mp);
    /*
     * Z.DAT:sub_34C47 chooses the candidate position from the stored
     * proficiency level.  MP-based level forcing happens later, when the
     * selected action is executed (sub_37734).  Resolving it here changes
     * target selection for low-MP actors because selRange[level] can differ.
     */
    const auto skillRange = skill->selRange[skillLevels.planning];

    struct SkillPlan {
        std::map<Position, SelectableCell> movementCells;
        std::optional<Position> position;
        bool expectedInRange = false;
    };
    auto planSkillPosition = [this, ch, skill, skillRange,
                              buildCastRangeCells, canCastAtCurrentPosition,
                              terrainDistance](int targetIndex) {
        SkillPlan plan;
        getSelectableArea(ch, plan.movementCells, ch->steps, 0);
        if (targetIndex < 0
            || targetIndex >= static_cast<int>(chars_.size())) {
            return plan;
        }
        const auto &target = chars_[targetIndex];
        const Position targetPosition{target.x, target.y};
        const Position actorPosition{ch->x, ch->y};
        const auto castRangeCells = buildCastRangeCells(
            targetPosition, actorPosition, skillRange);
        const auto mode = (skill->attackAreaType == 1
                           || skill->attackAreaType == 2)
            ? battle::CastMovementMode::Aligned
            : battle::CastMovementMode::Approach;
        const auto choice = battle::chooseCastMovementPosition(
            plan.movementCells, castRangeCells, actorPosition, targetPosition,
            skillRange, mode,
            canCastAtCurrentPosition(
                targetIndex, skill->attackAreaType, skillRange),
            terrainDistance);
        plan.position = choice.position;
        plan.expectedInRange = choice.expectedInRange;
        return plan;
    };

    const auto nearestCurrentTarget = [this, ch, actorIndex, onMap,
                                       terrainDistance]() {
        std::optional<int> selected;
        auto selectedDistance = std::numeric_limits<int>::max();
        for (std::size_t i = 0; i < chars_.size(); ++i) {
            const auto &candidate = chars_[i];
            if (static_cast<int>(i) == actorIndex
                || candidate.id < 0 || candidate.side == ch->side
                || candidate.info.hp <= 0
                || !onMap({candidate.x, candidate.y})) {
                continue;
            }
            const auto distance = terrainDistance(
                {ch->x, ch->y}, {candidate.x, candidate.y});
            if (distance < 0 || distance >= selectedDistance) { continue; }
            selected = static_cast<int>(i);
            selectedDistance = distance;
        }
        return selected;
    };

    const int selectedSkillSlot = *skillSlot;
    const auto castSkillAtCurrentPosition = [this, ch, skill, skillId,
                                              selectedSkillSlot,
                                              storedSkillLevel](int targetIndex) {
        if (targetIndex < 0 || targetIndex >= static_cast<int>(chars_.size())) {
            doRest(ch);
            return;
        }
        const auto &target = chars_[targetIndex];
        const auto directional = skill->attackAreaType == 1;
        if (directional) {
            const auto dx = target.x - ch->x;
            const auto dy = target.y - ch->y;
            if (dy < 0) ch->direction = DirUp;
            else if (dx > 0) ch->direction = DirRight;
            else if (dx < 0) ch->direction = DirLeft;
            else ch->direction = DirDown;
            cursorX_ = ch->x;
            cursorY_ = ch->y;
        } else {
            cursorX_ = target.x;
            cursorY_ = target.y;
        }
        actIndex_ = selectedSkillSlot;
        actId_ = skillId;
        /* Resolve the actual cast level only after movement is complete. */
        actLevel_ = battle::resolveAiSkillLevels(
            skill->reqMp, storedSkillLevel, ch->info.mp).execution;
        actItemSlot_ = -1;
        if (actLevel_ < 0) {
            doRest(ch);
            return;
        }
        attackTimesLeft_ = battle::attackCount(ch->info.doubleAttack);
        startActAction();
    };

    auto targetIndex = battle::chooseAiTarget(
        actorIndex, strategyActor, strategyCharacters,
        resourceRandom, pathDistance);
    SkillPlan skillPlan;
    if (targetIndex) {
        skillPlan = planSkillPosition(*targetIndex);
    }
    ch->actionCode = battle::actionCodeForSkill(
        ch->actionCode, forceSkill || preserveSupportFallback);
    if (skillPlan.position) {
        const auto selectedTargetIndex = *targetIndex;
        const auto castCheck = canCastAtCurrentPosition;
        const auto nearestTarget = nearestCurrentTarget;
        runAtPosition(
            skillPlan.movementCells, *skillPlan.position,
            [this, ch, selectedTargetIndex, skillRange, skill,
             castCheck, nearestTarget, castSkillAtCurrentPosition]() {
                if (castCheck(selectedTargetIndex,
                              skill->attackAreaType, skillRange)) {
                    castSkillAtCurrentPosition(selectedTargetIndex);
                    return;
                }
                const auto fallback = nearestTarget();
                if (fallback && castCheck(
                        *fallback, skill->attackAreaType, skillRange)) {
                    castSkillAtCurrentPosition(*fallback);
                } else {
                    doRest(ch);
                }
            });
        return;
    }

    const auto fallbackTarget = nearestCurrentTarget();
    if (fallbackTarget && canCastAtCurrentPosition(
            *fallbackTarget, skill->attackAreaType, skillRange)) {
        castSkillAtCurrentPosition(*fallbackTarget);
        return;
    }
    doRest();
}

}
