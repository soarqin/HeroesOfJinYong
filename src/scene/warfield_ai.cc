#include "warfield.hh"

#include "battle_presentation_snapshot_builder.hh"
#include "battle/ai_policy.hh"
#include "battle/ai_strategy.hh"
#include "battle/combat_rules.hh"
#include "battle/movement.hh"
#include "battle/turn_order.hh"
#include "world/action.hh"
#include "world/bag.hh"
#include "world/savedata.hh"
#include "content/constants.hh"

#include <algorithm>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace hojy::scene {
void Warfield::autoAction() {
    if (pendingAutoAction_) {
        battle::runPendingAction(pendingAutoAction_);
        return;
    }
    auto *ch = currentActor_;
    if (!ch) { setStage(Idle); return; }
    const auto resumeAutoAttack = resumeAutoAttack_;
    resumeAutoAttack_ = false;
    const auto currentAiStats = [](const CharInfo &actor) {
        return battle::resolveAiRuntimeStats(
            actor.aiEntryStats, actor.aiEquipmentBonusStats, actor.info);
    };
    const auto actorAiStats = currentAiStats(*ch);
    const battle::AiResourceState resourceState{
        ch->info.hp, ch->info.maxHp, ch->info.hurt, ch->info.poisoned,
        ch->info.stamina, ch->info.mp, ch->info.maxMp,
        actorAiStats.medic, actorAiStats.depoison,
    };
    const auto actorIndex = static_cast<int>(ch - chars_.data());
    std::vector<battle::AiAllyState> allies;
    allies.reserve(chars_.size());
    for (const auto &ally: chars_) {
        const auto validPosition = ally.x >= 0 && ally.x < mapWidth_
            && ally.y >= 0 && ally.y < mapHeight_;
        const auto allyAiStats = currentAiStats(ally);
        allies.push_back(battle::AiAllyState{
            ally.side, ally.id >= 0 && validPosition,
            ally.info.hp > 0 && validPosition,
            ally.info.hp, ally.info.maxHp, ally.info.hurt,
            ally.info.poisoned, allyAiStats.medic, allyAiStats.depoison,
            ally.actionCode, allyAiStats.attack,
        });
    }
    const auto allyPower = battle::summarizeAllyPower(ch->side, allies);
    std::int16_t resourceItemId = -1;
    int resourceActId = 0;
    std::optional<int> resourceTargetIndex;
    std::optional<std::pair<int, int>> resourceSupportPosition;
    std::vector<std::pair<int, int>> resourceMovingPath;
    auto findResourceItem = [this, ch, &resourceItemId](::hojy::world::state::PropType type, int delta) {
        resourceItemId = ch->side == 1
            ? ::hojy::world::state::tryUseNpcItem(&ch->info, type, static_cast<std::int16_t>(delta))
            : ::hojy::world::state::tryUseBagItem(
                battleBag_, &ch->info, type, static_cast<std::int16_t>(delta));
        return resourceItemId >= 0;
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
    auto &resourceRandom = battleRandom_;
    auto prepareSupport = [this, ch, actorIndex, actorAiStats, &allies, &resourceRandom,
                           &resourceActId, &resourceTargetIndex,
                           &resourceSupportPosition, &resourceMovingPath,
                           &buildCastRangeCells, &terrainDistance]
                          (battle::AiResourceAction action) {
        std::optional<int> targetIndex;
        int ability = 0;
        if (action == battle::AiResourceAction::MedicSupport) {
            ability = actorAiStats.medic;
            resourceActId = -1;
            targetIndex = battle::chooseMedicSupportTarget(
                actorIndex, ability, allies, resourceRandom);
        } else {
            ability = actorAiStats.depoison;
            resourceActId = -2;
            targetIndex = battle::chooseDepoisonSupportTarget(
                actorIndex, ability, allies, resourceRandom);
        }
        if (!targetIndex) { return battle::AiResourceAction::None; }

        resourceTargetIndex = targetIndex;
        const auto *target = &chars_[*targetIndex];
        std::map<std::pair<int, int>, SelectableCell> movementCells;
        getSelectableArea(ch, movementCells, ch->steps, 0);
        const std::pair<int, int> targetPosition{target->x, target->y};
        const std::pair<int, int> actorPosition{ch->x, ch->y};
        const auto castRangeCells = buildCastRangeCells(
            targetPosition, actorPosition, battle::calcTechniqueRange(ability));
        const auto currentCanCast = battle::canCastFromPosition(
            0, battle::calcTechniqueRange(ability),
            terrainDistance(actorPosition, targetPosition),
            actorPosition, targetPosition);
        const auto choice = battle::chooseCastMovementPosition(
            movementCells, castRangeCells, actorPosition, targetPosition,
            battle::calcTechniqueRange(ability),
            battle::CastMovementMode::Approach, currentCanCast,
            terrainDistance);
        resourceSupportPosition = choice.position;
        if (resourceSupportPosition) {
            auto *cell = &movementCells[*resourceSupportPosition];
            while (cell) {
                resourceMovingPath.emplace_back(cell->x, cell->y);
                cell = cell->moveParent;
            }
        }
        return action;
    };
    const auto resourceAction = resumeAutoAttack
        ? battle::AiResourceAction::None
        : battle::chooseAiResourceAction(
            resourceState, resourceRandom,
             [ch, actorIndex, actorAiStats, &allies, &findResourceItem, &resourceActId,
             &resourceTargetIndex, &prepareSupport](battle::AiResourceAction action) {
                switch (action) {
                case battle::AiResourceAction::RecoverHp:
                    if (battle::canSelfMedic(
                            actorAiStats.medic, ch->info.stamina, ch->info.hurt)) {
                        resourceActId = -1;
                        return action;
                    }
                    if (findResourceItem(::hojy::world::state::PropType::Hp, ch->info.maxHp - ch->info.hp)) {
                        return action;
                    }
                    resourceTargetIndex = battle::chooseMedicProvider(
                        actorIndex, ch->info.hurt, allies);
                    return resourceTargetIndex
                        ? battle::AiResourceAction::RequestMedic
                        : battle::AiResourceAction::None;
                case battle::AiResourceAction::SelfDepoison:
                    if (battle::canSelfDepoison(
                            actorAiStats.depoison, ch->info.stamina, ch->info.poisoned)) {
                        resourceActId = -2;
                        return action;
                    }
                    if (findResourceItem(::hojy::world::state::PropType::Poisoned, ch->info.poisoned)) {
                        return action;
                    }
                    resourceTargetIndex = battle::chooseDepoisonProvider(
                        actorIndex, ch->info.poisoned, allies);
                    return resourceTargetIndex
                        ? battle::AiResourceAction::RequestDepoison
                        : battle::AiResourceAction::None;
                case battle::AiResourceAction::RecoverMp:
                    return findResourceItem(::hojy::world::state::PropType::Mp, ch->info.maxMp - ch->info.mp)
                        ? action : battle::AiResourceAction::None;
                case battle::AiResourceAction::MedicSupport:
                case battle::AiResourceAction::DepoisonSupport:
                    return prepareSupport(action);
                default:
                    return battle::AiResourceAction::None;
                }
            });
    const auto supportWithoutPosition =
        (resourceAction == battle::AiResourceAction::MedicSupport
         || resourceAction == battle::AiResourceAction::DepoisonSupport)
        && !resourceSupportPosition;
    const auto requestSupport =
        resourceAction == battle::AiResourceAction::RequestMedic
        || resourceAction == battle::AiResourceAction::RequestDepoison;
    if (!resumeAutoAttack) {
        ch->actionCode = battle::originalActionCode(
            resourceAction, resourceItemId >= 0);
    }
    if (requestSupport) {
        if (resourceTargetIndex) {
            const auto *provider = &chars_[*resourceTargetIndex];
            std::map<std::pair<int, int>, SelectableCell> movementCells;
            getSelectableArea(ch, movementCells, ch->steps, 0);
            const auto providerPosition = std::make_pair(provider->x, provider->y);
            const auto approachPosition = battle::chooseApproachPosition(
                movementCells, providerPosition,
                [this](std::pair<int, int> from, std::pair<int, int> target) {
                    return battle::shortestPathDistance(
                        mapWidth_, mapHeight_, from, target,
                        [this](int x, int y) {
                            return cellInfo_[y * mapWidth_ + x].blocked;
                        },
                        [this, target](int x, int y) {
                            return std::make_pair(x, y) != target
                                && cellInfo_[y * mapWidth_ + x].charInfo != nullptr;
                        });
                });
            if (approachPosition
                && *approachPosition != std::make_pair<int, int>(ch->x, ch->y)) {
                auto *cell = &movementCells[*approachPosition];
                movingPath_.clear();
                while (cell) {
                    movingPath_.emplace_back(cell->x, cell->y);
                    cell = cell->moveParent;
                }
                resumeAutoAttack_ = battle::shouldResumeAutoAttack(true);
                setStage(Moving);
                return;
            }
        }
        // No approach movement was scheduled.  The current actor continues
        // immediately, and the continuation flag must not leak to the next
        // queued actor.
        resumeAutoAttack_ = battle::shouldResumeAutoAttack(false);
    }
    if (supportWithoutPosition
        && battle::chooseUnreachableSupportFallback(
               actorAiStats.attack, allyPower.total, allyPower.count)
           == battle::AiSupportFallback::Rest) {
        pendingAutoAction_ = [this, ch]() { doRest(ch); };
        battle::runPendingAction(pendingAutoAction_);
        return;
    }
    if (resourceAction != battle::AiResourceAction::None
        && !supportWithoutPosition && !requestSupport) {
        if (resourceAction == battle::AiResourceAction::Rest) {
            pendingAutoAction_ = [this, ch]() { doRest(ch); };
        } else if (resourceItemId >= 0) {
            pendingAutoAction_ = [this, ch, resourceItemId]() {
                if (currentActor_ != ch) { return; }
                if (ch->info.hp <= 0) {
                    endTurn(ch);
                    return;
                }
                const auto itemSlot = ch->side == 1
                    ? ::hojy::world::state::findNpcItemSlot(
                          &ch->info, resourceItemId)
                    : static_cast<std::int16_t>(-1);
                std::map<::hojy::world::state::PropType, std::int16_t> changes;
                const auto usedItem = ch->side == 1
                    ? ::hojy::world::state::useNpcItem(&ch->info, resourceItemId, changes)
                    : ::hojy::world::state::useItem(
                        battleBag_, &ch->info, resourceItemId, changes);
                if (!usedItem) {
                    doRest(ch);
                    return;
                }
                if (const auto actor = participantIndex(ch)) {
                    const auto source = ch->side == 1
                        ? battle::InventorySource::NpcCarry
                        : battle::InventorySource::PartyBag;
                    if (!recordBattleAction(battle::BattleAction{
                            *actor,
                            battle::ItemAction{resourceItemId, source, itemSlot},
                        })) {
                        return;
                    }
                }
                setStage(PoppingUp);
                setPresentationStage(BattlePresentationStage::ItemResult);
                pendingItemResultActorId_ = ch->id;
                pendingItemResultItemId_ = resourceItemId;
                std::vector<BattleItemChange> changeValues;
                changeValues.reserve(changes.size());
                for (const auto &[property, value]: changes) {
                    changeValues.push_back({static_cast<std::int16_t>(property), value});
                }
                const auto actorId = ch->id;
                const auto sessionToken = presentationSessionToken();
                const auto actionGeneration = presentationGeneration_;
                auto messages = buildBattleItemResultMessages(
                    resourceItemId, changes);
                postCommand([sessionToken, actorId, actionGeneration,
                             itemId = resourceItemId,
                             changes = std::move(changeValues),
                             messages = std::move(messages)](
                                SceneCommandContext &context) mutable {
                    BattleItemResultRequest request{
                        sessionToken, actorId, itemId, std::move(changes)};
                    request.actionGeneration = actionGeneration;
                    request.expectedStage = BattlePresentationStage::ItemResult;
                    request.messages = std::move(messages);
                    context.showBattleItemResult(std::move(request));
                });
            };
        } else {
            auto *target = resourceTargetIndex
                ? &chars_[*resourceTargetIndex] : ch;
            const auto targetIndex = resourceTargetIndex.value_or(-1);
            const auto resourceRange = (resourceAction
                == battle::AiResourceAction::MedicSupport)
                ? battle::calcTechniqueRange(actorAiStats.medic)
                : battle::calcTechniqueRange(actorAiStats.depoison);
            const auto castCheck = canCastAtCurrentPosition;
            pendingAutoAction_ = [this, ch, target, targetIndex,
                                  resourceActId, resourceAction,
                                  resourceRange, castCheck, allyPower,
                                  actorAiStats]() {
                if (currentActor_ != ch) { return; }
                if (ch->info.hp <= 0 || target->info.hp <= 0
                    || target->x < 0 || target->x >= mapWidth_
                    || target->y < 0 || target->y >= mapHeight_) {
                    endTurn(ch);
                    return;
                }
                if ((resourceAction == battle::AiResourceAction::MedicSupport
                     || resourceAction == battle::AiResourceAction::DepoisonSupport)
                    && !castCheck(targetIndex, 0, resourceRange)) {
                    if (battle::chooseUnreachableSupportFallback(
                            actorAiStats.attack, allyPower.total, allyPower.count)
                        == battle::AiSupportFallback::Rest) {
                        doRest(ch);
                    } else {
                        /* Keep action code 4/5 while the actor falls back to
                         * the ordinary random-skill path on the next frame. */
                        resumeAutoAttack_ = true;
                    }
                    return;
                }
                actIndex_ = -1;
                actId_ = resourceActId;
                actLevel_ = 0;
                attackTimesLeft_ = 1;
                cursorX_ = target->x;
                cursorY_ = target->y;
                startActAction();
            };
        }
        if (resourceItemId >= 0) {
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
            if (retreatPosition
                && *retreatPosition != std::make_pair<int, int>(ch->x, ch->y)) {
                movingPath_.clear();
                auto *cell = &movementCells[*retreatPosition];
                while (cell) {
                    movingPath_.emplace_back(cell->x, cell->y);
                    cell = cell->moveParent;
                }
                setStage(Moving);
                return;
            }
        }
        if (resourceSupportPosition
            && (*resourceSupportPosition != std::make_pair<int, int>(ch->x, ch->y))) {
            movingPath_ = std::move(resourceMovingPath);
            setStage(Moving);
            return;
        }
        battle::runPendingAction(pendingAutoAction_);
        return;
    }
    autoActionSkill(ch, actorIndex, actorAiStats, resourceState, allies, allyPower,
                    resumeAutoAttack, requestSupport, supportWithoutPosition,
                    resourceAction, resourceRandom);

}
}
