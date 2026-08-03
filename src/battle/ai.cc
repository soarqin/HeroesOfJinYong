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

#include "formulas.hh"
#include "random.hh"

#include <cstdlib>

namespace hojy::battle {

namespace {

struct SideTotals {
    int own = 0, ownCount = 0;
    int enemy = 0, enemyCount = 0;
};

/* Z.DAT:0x335D8 sums `attack + hp` per side over every participant. */
SideTotals sideTotals(const AiContext &context) {
    SideTotals totals;
    const int mySide = context.participants[context.self].side;
    for (const auto &participant: context.participants) {
        const int power = participant.stats.attack + participant.stats.hp;
        if (participant.side == mySide) {
            totals.own += power;
            ++totals.ownCount;
        } else {
            totals.enemy += power;
            ++totals.enemyCount;
        }
    }
    return totals;
}

bool isEnemy(const AiContext &context, int index) {
    const auto &participant = context.participants[index];
    return participant.side != context.participants[context.self].side && participant.active;
}

bool isAlly(const AiContext &context, int index) {
    const auto &participant = context.participants[index];
    return index != context.self
        && participant.side == context.participants[context.self].side
        && participant.active;
}

/* Z.DAT:0x3513A: the most dangerous enemy. */
int targetByHighestAttack(const AiContext &context) {
    int best = 0, found = -1;
    for (int i = 0; i < int(context.participants.size()); ++i) {
        if (!isEnemy(context, i)) { continue; }
        const int attack = context.participants[i].stats.attack;
        if (attack > best) { best = attack; found = i; }
    }
    return found;
}

/* Z.DAT:0x351A7: the easiest enemy. */
int targetByLowestAttack(const AiContext &context) {
    int best = 1000, found = -1;
    for (int i = 0; i < int(context.participants.size()); ++i) {
        if (!isEnemy(context, i)) { continue; }
        const int attack = context.participants[i].stats.attack;
        if (attack < best) { best = attack; found = i; }
    }
    return found;
}

/* Z.DAT:0x35372: the enemy that takes the fewest steps to reach. */
int targetByShortestPath(const AiContext &context) {
    int best = 1000, found = -1;
    for (int i = 0; i < int(context.participants.size()); ++i) {
        if (!isEnemy(context, i)) { continue; }
        const int distance = context.participants[i].distance;
        if (distance < 0 || distance >= best) { continue; }
        best = distance;
        found = i;
    }
    return found;
}

/* Highest value of one proficiency among the reachable enemies. */
int bestEnemyBy(const AiContext &context, int AiStats::*field, int &value) {
    value = 0;
    int found = -1;
    for (int i = 0; i < int(context.participants.size()); ++i) {
        if (!isEnemy(context, i)) { continue; }
        const int score = context.participants[i].stats.*field;
        if (score > value) { value = score; found = i; }
    }
    return found;
}

/*
 * Z.DAT:0x35217: knock out the enemy support first, the healer if the own side
 * does not field poison and the curer if it does.
 *
 * The original tracks the two sweeps with separate flags but only honours the
 * medic one: when the depoison sweep succeeds it still falls through to the
 * lowest-attack selector, which overwrites the stored target. That makes the
 * curer branch dead code, so this build keeps the target it just chose.
 */
int targetBySupportRole(const AiContext &context) {
    const int mySide = context.participants[context.self].side;
    bool sidePoisons = false;
    for (const auto &participant: context.participants) {
        if (participant.side == mySide && participant.stats.poison > 20) {
            sidePoisons = true;
            break;
        }
    }
    int value = 0;
    if (sidePoisons) {
        const int curer = bestEnemyBy(context, &AiStats::depoison, value);
        if (curer >= 0 && value >= 20) { return curer; }
    }
    const int healer = bestEnemyBy(context, &AiStats::medic, value);
    if (healer >= 0 && value >= 20) { return healer; }
    return targetByLowestAttack(context);
}

bool poisonCanBite(const AiContext &context, int index) {
    const auto &target = context.participants[index].stats;
    const auto &me = context.participants[context.self].stats;
    return target.poisoned < 95 && target.antipoison < me.poison;
}

/* Z.DAT:0x35667: among the enemies the poison still bites, the strongest one. */
int poisonTargetByHighestAttack(const AiContext &context) {
    int best = 0, found = -1;
    for (int i = 0; i < int(context.participants.size()); ++i) {
        if (!isEnemy(context, i) || !poisonCanBite(context, i)) { continue; }
        const int attack = context.participants[i].stats.attack;
        if (attack > best) { best = attack; found = i; }
    }
    return found;
}

/*
 * Z.DAT:0x3570F: the nearest of those enemies. The original reads the grid at
 * the previously stored target instead of the candidate, so every candidate
 * compares the same distance and the first one always wins. That is a plain
 * wrong-variable defect, so this build measures the candidate.
 */
int poisonTargetByShortestPath(const AiContext &context) {
    int best = 1000, found = -1;
    for (int i = 0; i < int(context.participants.size()); ++i) {
        if (!isEnemy(context, i) || !poisonCanBite(context, i)) { continue; }
        const int distance = context.participants[i].distance;
        if (distance < 0 || distance >= best) { continue; }
        best = distance;
        found = i;
    }
    return found;
}

bool findItem(const AiContext &context, AiDecision &decision, AiAction action,
              bool (*matches)(const AiItem &)) {
    for (const auto &item: context.items) {
        if (!matches(item)) { continue; }
        decision.action = action;
        decision.target = context.self;
        decision.itemSlot = item.slot;
        return true;
    }
    return false;
}

/* Z.DAT:0x33C4D */
bool tryRestoreHp(const AiContext &context, AiDecision &decision) {
    const auto &me = context.participants[context.self].stats;
    if (me.medic >= 20 && me.stamina >= 50 && me.medic > me.hurt - 30) {
        decision.action = AiAction::Medic;
        decision.target = context.self;
        return true;
    }
    if (findItem(context, decision, AiAction::UseItem,
                 [](const AiItem &item) { return item.addHp > 0; })) {
        return true;
    }
    for (int i = 0; i < int(context.participants.size()); ++i) {
        if (!isAlly(context, i)) { continue; }
        const auto &ally = context.participants[i].stats;
        if (ally.medic <= 20 || ally.medic <= me.hurt - 30) { continue; }
        /* Ask for help and keep fighting this turn. */
        decision.action = AiAction::Attack;
        decision.request = AiRequest::Medic;
        return true;
    }
    return false;
}

/*
 * Z.DAT:0x33E93. The original scans the shared bag for `addPoison < 0`, which is
 * the poison proficiency modifier rather than the poisoned value, so no antidote
 * ever matches on the player side. This build uses `addPoisoned` for both sides.
 */
bool tryCurePoison(const AiContext &context, AiDecision &decision) {
    const auto &me = context.participants[context.self].stats;
    if (me.depoison >= 20 && me.stamina >= 50 && me.depoison > me.poisoned - 30) {
        decision.action = AiAction::Depoison;
        decision.target = context.self;
        return true;
    }
    if (findItem(context, decision, AiAction::UseItem,
                 [](const AiItem &item) { return item.addPoisoned < 0; })) {
        return true;
    }
    for (int i = 0; i < int(context.participants.size()); ++i) {
        if (!isAlly(context, i)) { continue; }
        const auto &ally = context.participants[i].stats;
        if (ally.depoison <= 20 || ally.depoison <= me.poisoned - 30) { continue; }
        decision.action = AiAction::Attack;
        decision.request = AiRequest::Depoison;
        return true;
    }
    return false;
}

/* Z.DAT:0x341F6 */
bool tryMedicAlly(const AiContext &context, AiDecision &decision, RandomSource &random) {
    const auto &me = context.participants[context.self].stats;
    auto gate = [&random](int threshold) { return originalRandom(random, 10) < threshold; };
    for (int i = 0; i < int(context.participants.size()); ++i) {
        if (!isAlly(context, i)) { continue; }
        const auto &ally = context.participants[i];
        if (me.medic <= ally.stats.hurt - 30) { continue; }
        const bool wanted = ally.request == AiRequest::Medic
            || ally.stats.hp < 20 || ally.stats.hurt > 40
            || (ally.stats.hp < ally.stats.maxHp / 2 && gate(7))
            || (ally.stats.hp < ally.stats.maxHp / 3 && gate(8))
            || (ally.stats.hp < ally.stats.maxHp / 4 && gate(9))
            || (ally.stats.hp < ally.stats.maxHp / 5);
        if (!wanted) { continue; }
        decision.action = AiAction::Medic;
        decision.target = i;
        return true;
    }
    return false;
}

/* Z.DAT:0x343DA */
bool tryDepoisonAlly(const AiContext &context, AiDecision &decision, RandomSource &random) {
    const auto &me = context.participants[context.self].stats;
    auto gate = [&random](int threshold) { return originalRandom(random, 10) < threshold; };
    for (int i = 0; i < int(context.participants.size()); ++i) {
        if (!isAlly(context, i)) { continue; }
        const auto &ally = context.participants[i];
        if (me.depoison <= ally.stats.poisoned - 30) { continue; }
        const bool wanted = ally.request == AiRequest::Depoison
            || (ally.stats.poisoned > 10 && gate(4))
            || (ally.stats.poisoned > 20 && gate(6))
            || (ally.stats.poisoned > 30 && gate(8))
            || ally.stats.poisoned > 40;
        if (!wanted) { continue; }
        decision.action = AiAction::Depoison;
        decision.target = i;
        return true;
    }
    return false;
}

/* Z.DAT:0x34550 */
AiDecision decideAttack(const AiContext &context, RandomSource &random) {
    AiDecision decision;
    const auto &me = context.participants[context.self].stats;
    const auto totals = sideTotals(context);

    /* Weak myself but a dominant team: spend the turn supporting instead. */
    const bool supportInstead = totals.enemyCount > 0
        && (totals.enemy / totals.enemyCount) / 2 > me.hp + me.attack
        && totals.own > totals.enemy * 2;
    if (supportInstead) {
        if (me.medic >= 20 && me.stamina >= 50) {
            int best = 0, target = -1;
            for (int i = 0; i < int(context.participants.size()); ++i) {
                if (!isAlly(context, i)) { continue; }
                const auto &ally = context.participants[i].stats;
                if (ally.hp >= ally.maxHp) { continue; }
                if (ally.maxHp - ally.hp > best) { best = ally.maxHp - ally.hp; target = i; }
            }
            if (target >= 0) {
                decision.action = AiAction::Medic;
                decision.target = target;
                return decision;
            }
        } else if (me.depoison >= 20 && me.stamina >= 50) {
            int best = 0, target = -1;
            for (int i = 0; i < int(context.participants.size()); ++i) {
                if (!isAlly(context, i)) { continue; }
                const auto &ally = context.participants[i].stats;
                if (ally.poisoned <= 0) { continue; }
                if (ally.poisoned > best) { best = ally.poisoned; target = i; }
            }
            if (target >= 0) {
                decision.action = AiAction::Depoison;
                decision.target = target;
                return decision;
            }
        }
    }

    /* Poison skill. */
    if (me.poison - me.attack > originalRandom(random, 50)
        && originalRandom(random, 150) < me.poison) {
        decision.action = AiAction::Poison;
        return decision;
    }

    /*
     * Throwing. The player side demands a stronger item and rolls against the
     * throwing proficiency; the enemy side only compares with the raw attack.
     */
    const bool sharedBag = context.participants[context.self].side == 0;
    const int damageBar = sharedBag ? me.attack * 3 / 2 : me.attack;
    for (const auto &item: context.items) {
        if (item.addHp < 0 && std::abs(item.addHp) > damageBar) {
            const bool roll = sharedBag ? originalRandom(random, me.throwing) > 20
                                        : originalRandom(random, 10) < 6;
            if (roll) {
                decision.action = AiAction::Throw;
                decision.itemSlot = item.slot;
                return decision;
            }
        }
        if (item.addPoisoned > 0 && std::abs(item.addPoisoned) > damageBar
            && originalRandom(random, 10) < 3) {
            decision.action = AiAction::Throw;
            decision.itemSlot = item.slot;
            return decision;
        }
    }

    /* Plain skill attack. */
    if (me.stamina > 10 && me.minSkillReqMp >= 0 && me.mp >= me.minSkillReqMp) {
        decision.action = AiAction::Attack;
        return decision;
    }
    return decision;
}

}

int pickAiTarget(const AiContext &context, RandomSource &random) {
    const auto &me = context.participants[context.self].stats;
    int found = -1;
    if (me.integrity >= 75 && originalRandom(random, 10) < 7) {
        found = targetByHighestAttack(context);
    }
    if (found < 0 && me.integrity <= 25 && originalRandom(random, 10) < 7) {
        found = targetByLowestAttack(context);
    }
    if (found < 0 && me.potential >= 70 && originalRandom(random, 10) < 7) {
        found = targetBySupportRole(context);
    }
    if (found < 0) {
        found = targetByShortestPath(context);
    }
    return found;
}

int pickAiPoisonTarget(const AiContext &context, RandomSource &random) {
    const auto &me = context.participants[context.self].stats;
    int found = -1;
    if (me.potential > 60 && originalRandom(random, 10) < 7) {
        found = poisonTargetByHighestAttack(context);
    }
    if (found < 0) {
        found = poisonTargetByShortestPath(context);
    }
    return found;
}

AiDecision decideAiAction(const AiContext &context, RandomSource &random) {
    AiDecision decision;
    if (context.self < 0 || context.self >= int(context.participants.size())) { return decision; }
    const auto &me = context.participants[context.self].stats;
    if (me.stamina < 10) { return decision; }

    auto gate = [&random](int threshold) { return originalRandom(random, 10) < threshold; };

    const bool wantHp = me.hp < 20 || me.hurt > 50
        || (me.hp < me.maxHp / 2 && gate(3))
        || (me.hp < me.maxHp / 3 && gate(5))
        || (me.hp < me.maxHp / 4 && gate(7))
        || (me.hp < me.maxHp / 5 && gate(9));
    if (wantHp && tryRestoreHp(context, decision)) { return decision; }

    if (originalRandom(random, 10) < me.poisoned / 10 && tryCurePoison(context, decision)) {
        return decision;
    }

    const bool wantMp = (me.mp < me.maxMp / 2 && gate(2))
        || (me.mp < me.maxMp / 3 && gate(4))
        || (me.mp < me.maxMp / 4 && gate(6))
        || (me.mp < me.maxMp / 5 && gate(8));
    if (wantMp && findItem(context, decision, AiAction::UseItem,
                           [](const AiItem &item) { return item.addMp > 0; })) {
        return decision;
    }

    if (me.stamina > 50
        && ((me.medic >= 20 && gate(4)) || (me.medic >= 40 && gate(6))
            || (me.medic >= 60 && gate(8)) || me.medic >= 80)
        && tryMedicAlly(context, decision, random)) {
        return decision;
    }

    if (me.stamina > 50
        && ((me.depoison >= 20 && gate(4)) || (me.depoison >= 40 && gate(6))
            || (me.depoison >= 60 && gate(8)) || me.depoison >= 80)
        && tryDepoisonAlly(context, decision, random)) {
        return decision;
    }

    if (gate(5)
        && (me.hp < 20 || (me.hp < me.maxHp / 4 && gate(6))
            || (me.hp < me.maxHp / 5 && gate(8)))) {
        decision.action = AiAction::Flee;
        return decision;
    }

    return decideAttack(context, random);
}

}
