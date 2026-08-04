/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ai.hh"

#include "ai_policy.hh"
#include "ai_strategy.hh"
#include "resource_items.hh"

#include <optional>

namespace hojy::battle {

namespace {

bool validContext(const AiContext &context) {
    return context.self >= 0
        && context.self < static_cast<int>(context.participants.size());
}

AiResourceState resourceState(const AiStats &stats) {
    return AiResourceState{
        stats.hp, stats.maxHp, stats.hurt, stats.poisoned,
        stats.stamina, stats.mp, stats.maxMp, stats.medic, stats.depoison,
    };
}

std::vector<AiAllyState> allyStates(const AiContext &context) {
    std::vector<AiAllyState> states;
    states.reserve(context.participants.size());
    for (const auto &participant: context.participants) {
        states.push_back(AiAllyState{
            participant.side,
            true,
            participant.active,
            participant.stats.hp,
            participant.stats.maxHp,
            participant.stats.hurt,
            participant.stats.poisoned,
            participant.stats.medic,
            participant.stats.depoison,
            static_cast<int>(participant.request),
            participant.stats.attack,
        });
    }
    return states;
}

std::vector<ResourceItemOption> resourceItems(const AiContext &context) {
    std::vector<ResourceItemOption> items;
    items.reserve(context.items.size());
    for (const auto &item: context.items) {
        items.push_back(ResourceItemOption{
            item.slot, item.addHp, item.addMp, 0, item.addPoisoned,
        });
    }
    return items;
}

AiStrategyActor strategyActor(const AiStats &stats, int side) {
    AiStrategyActor actor;
    actor.side = side;
    actor.hp = stats.hp;
    actor.attack = stats.attack;
    actor.stamina = stats.stamina;
    actor.mp = stats.mp;
    actor.medic = stats.medic;
    actor.poison = stats.poison;
    actor.depoison = stats.depoison;
    actor.throwing = stats.throwing;
    actor.integrity = stats.integrity;
    actor.potential = stats.potential;
    return actor;
}

std::vector<AiStrategyCharacter> strategyCharacters(
    const AiContext &context) {
    std::vector<AiStrategyCharacter> characters;
    characters.reserve(context.participants.size());
    for (const auto &participant: context.participants) {
        const auto &stats = participant.stats;
        characters.push_back(AiStrategyCharacter{
            participant.side,
            true,
            participant.active,
            0,
            0,
            stats.hp,
            stats.maxHp,
            stats.attack,
            stats.medic,
            stats.poisoned,
            stats.depoison,
            stats.antipoison,
            stats.poison,
        });
    }
    return characters;
}

std::vector<AiThrowingOption> throwingItems(const AiContext &context) {
    std::vector<AiThrowingOption> items;
    items.reserve(context.items.size());
    for (const auto &item: context.items) {
        items.push_back(AiThrowingOption{
            item.slot, item.slot, item.addHp, item.addPoisoned, 3,
        });
    }
    return items;
}

std::vector<AiSkillOption> skillOptions(const AiStats &stats) {
    if (stats.minSkillReqMp < 0) { return {}; }
    return {{0, 1, stats.minSkillReqMp}};
}

AiDecision mapResourceDecision(AiResourceAction action, int self,
                               int target, int itemSlot) {
    AiDecision decision;
    switch (action) {
    case AiResourceAction::Rest:
        break;
    case AiResourceAction::RecoverHp:
        decision.action = itemSlot >= 0 ? AiAction::UseItem : AiAction::Medic;
        decision.target = self;
        decision.itemSlot = itemSlot;
        break;
    case AiResourceAction::SelfDepoison:
        decision.action = itemSlot >= 0 ? AiAction::UseItem : AiAction::Depoison;
        decision.target = self;
        decision.itemSlot = itemSlot;
        break;
    case AiResourceAction::RecoverMp:
        decision.action = AiAction::UseItem;
        decision.target = self;
        decision.itemSlot = itemSlot;
        break;
    case AiResourceAction::RequestMedic:
        decision.action = AiAction::Attack;
        decision.request = AiRequest::Medic;
        break;
    case AiResourceAction::RequestDepoison:
        decision.action = AiAction::Attack;
        decision.request = AiRequest::Depoison;
        break;
    case AiResourceAction::MedicSupport:
        decision.action = AiAction::Medic;
        decision.target = target;
        break;
    case AiResourceAction::DepoisonSupport:
        decision.action = AiAction::Depoison;
        decision.target = target;
        break;
    case AiResourceAction::None:
        break;
    }
    return decision;
}

AiDecision mapFollowupDecision(const AiFollowupDecision &followup) {
    AiDecision decision;
    switch (followup.action) {
    case AiFollowupAction::Rest:
        break;
    case AiFollowupAction::MedicSupport:
        decision.action = AiAction::Medic;
        decision.target = followup.targetIndex;
        break;
    case AiFollowupAction::DepoisonSupport:
        decision.action = AiAction::Depoison;
        decision.target = followup.targetIndex;
        break;
    case AiFollowupAction::Poison:
        decision.action = AiAction::Poison;
        break;
    case AiFollowupAction::Throw:
        decision.action = AiAction::Throw;
        decision.itemSlot = followup.selectionIndex;
        break;
    case AiFollowupAction::Skill:
        decision.action = AiAction::Attack;
        break;
    }
    return decision;
}

AiPathDistance contextDistance(const AiContext &context) {
    return [&context](int targetIndex) {
        if (targetIndex < 0
            || targetIndex >= static_cast<int>(context.participants.size())) {
            return -1;
        }
        return context.participants[targetIndex].distance;
    };
}

}

AiStats snapshotAiStats(const ::hojy::world::state::CharacterData &info) noexcept {
    AiStats stats;
    stats.hp = info.hp;
    stats.maxHp = info.maxHp;
    stats.mp = info.mp;
    stats.maxMp = info.maxMp;
    stats.stamina = info.stamina;
    stats.hurt = info.hurt;
    stats.poisoned = info.poisoned;
    stats.attack = info.attack;
    stats.medic = info.medic;
    stats.poison = info.poison;
    stats.depoison = info.depoison;
    stats.antipoison = info.antipoison;
    stats.throwing = info.throwing;
    stats.integrity = info.integrity;
    stats.potential = info.potential;
    return stats;
}

AiStats captureAiEquipmentBonuses(const AiStats &entry,
                                  const ::hojy::world::state::CharacterData &effective) noexcept {
    const auto current = snapshotAiStats(effective);
    AiStats bonus;
    bonus.attack = current.attack - entry.attack;
    bonus.medic = current.medic - entry.medic;
    bonus.poison = current.poison - entry.poison;
    bonus.depoison = current.depoison - entry.depoison;
    bonus.antipoison = current.antipoison - entry.antipoison;
    bonus.throwing = current.throwing - entry.throwing;
    bonus.integrity = current.integrity - entry.integrity;
    bonus.potential = current.potential - entry.potential;
    return bonus;
}

AiStats resolveAiRuntimeStats(const AiStats &entry,
                              const AiStats &equipmentBonus,
                              const ::hojy::world::state::CharacterData &effective) noexcept {
    auto current = snapshotAiStats(effective);
    const auto resolve = [](int entryValue, int equipmentValue,
                            int currentValue) {
        return entryValue + (currentValue - (entryValue + equipmentValue));
    };
    current.attack = resolve(entry.attack, equipmentBonus.attack, current.attack);
    current.medic = resolve(entry.medic, equipmentBonus.medic, current.medic);
    current.poison = resolve(entry.poison, equipmentBonus.poison, current.poison);
    current.depoison = resolve(entry.depoison, equipmentBonus.depoison, current.depoison);
    current.antipoison = resolve(
        entry.antipoison, equipmentBonus.antipoison, current.antipoison);
    current.throwing = resolve(
        entry.throwing, equipmentBonus.throwing, current.throwing);
    current.integrity = resolve(
        entry.integrity, equipmentBonus.integrity, current.integrity);
    current.potential = resolve(
        entry.potential, equipmentBonus.potential, current.potential);
    return current;
}

AiDecision decideAiAction(const AiContext &context, RandomSource &random) {
    if (!validContext(context)) { return {}; }

    const auto &self = context.participants[context.self];
    const auto state = resourceState(self.stats);
    const auto allies = allyStates(context);
    const auto items = resourceItems(context);
    int target = -1;
    int itemSlot = -1;

    const auto resourceAction = chooseAiResourceAction(
        state, random,
        [&](AiResourceAction action) {
            switch (action) {
            case AiResourceAction::RecoverHp:
                if (canSelfMedic(self.stats.medic, self.stats.stamina,
                                 self.stats.hurt)) {
                    return action;
                }
                if (const auto item = chooseFirstResourceItem(
                        items, ResourceItemKind::Hp)) {
                    itemSlot = *item;
                    return action;
                }
                if (const auto provider = chooseMedicProvider(
                        context.self, self.stats.hurt, allies)) {
                    target = *provider;
                    return AiResourceAction::RequestMedic;
                }
                return AiResourceAction::None;
            case AiResourceAction::SelfDepoison:
                if (canSelfDepoison(self.stats.depoison, self.stats.stamina,
                                    self.stats.poisoned)) {
                    return action;
                }
                if (const auto item = chooseFirstResourceItem(
                        items, ResourceItemKind::Poisoned)) {
                    itemSlot = *item;
                    return action;
                }
                if (const auto provider = chooseDepoisonProvider(
                        context.self, self.stats.poisoned, allies)) {
                    target = *provider;
                    return AiResourceAction::RequestDepoison;
                }
                return AiResourceAction::None;
            case AiResourceAction::RecoverMp:
                if (const auto item = chooseFirstResourceItem(
                        items, ResourceItemKind::Mp)) {
                    itemSlot = *item;
                    return action;
                }
                return AiResourceAction::None;
            case AiResourceAction::MedicSupport:
                if (const auto selected = chooseMedicSupportTarget(
                        context.self, self.stats.medic, allies, random)) {
                    target = *selected;
                    return action;
                }
                return AiResourceAction::None;
            case AiResourceAction::DepoisonSupport:
                if (const auto selected = chooseDepoisonSupportTarget(
                        context.self, self.stats.depoison, allies, random)) {
                    target = *selected;
                    return action;
                }
                return AiResourceAction::None;
            default:
                return AiResourceAction::None;
            }
        });
    if (resourceAction != AiResourceAction::None) {
        return mapResourceDecision(
            resourceAction, context.self, target, itemSlot);
    }

    if (shouldRetreatForHealth(state, random)) {
        AiDecision decision;
        decision.action = AiAction::Flee;
        return decision;
    }

    const auto actor = strategyActor(self.stats, self.side);
    const auto characters = strategyCharacters(context);
    return mapFollowupDecision(chooseAiFollowupAction(
        context.self, actor, characters, throwingItems(context),
        skillOptions(self.stats), random));
}

int pickAiTarget(const AiContext &context, RandomSource &random) {
    if (!validContext(context)) { return -1; }
    const auto &self = context.participants[context.self];
    const auto target = chooseAiTarget(
        context.self, strategyActor(self.stats, self.side),
        strategyCharacters(context), random, contextDistance(context));
    return target.value_or(-1);
}

int pickAiPoisonTarget(const AiContext &context, RandomSource &random) {
    if (!validContext(context)) { return -1; }
    const auto &self = context.participants[context.self];
    const auto target = choosePoisonTarget(
        context.self, strategyActor(self.stats, self.side),
        strategyCharacters(context), random, contextDistance(context));
    return target.value_or(-1);
}

}
