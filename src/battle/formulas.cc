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

#include "formulas.hh"

#include "random.hh"

#include <cstdint>

namespace hojy::battle {

namespace {

/*
 * The original build keeps these values in 16-bit registers and only widens
 * them with `cwde` at the end of an expression, so intermediate results wrap
 * at 16 bits. Reproducing the truncation keeps overflow behaviour identical.
 */
inline int narrow(int value) noexcept {
    return static_cast<std::int16_t>(static_cast<std::uint16_t>(value));
}

}

int originalRandom(RandomSource &random, int bound) {
    if (bound <= 1 || bound > OriginalRandomBoundMax) { return 0; }
    return random.next(bound);
}

int skillMpCost(int reqMp, int level) noexcept {
    if (reqMp <= 0) { return 0; }
    if (level < 0) { level = 0; }
    return reqMp * ((level + 1) / 2);
}

int resolveSkillLevel(int reqMp, int level, int currMp) noexcept {
    if (reqMp <= 0) { return level; }
    if (currMp < reqMp) { return -1; }
    for (int candidate = level; candidate > 0; --candidate) {
        if (currMp >= skillMpCost(reqMp, candidate)) { return candidate; }
    }
    return 0;
}

int applyDistanceDecay(int damage, int distance) noexcept {
    const int base = narrow(damage);
    if (distance > 10) {
        return static_cast<int>(static_cast<std::uint32_t>(base * 2) / 3u);
    }
    const int percent = 100 - (distance - 1) * 3;
    return static_cast<int>(static_cast<std::uint32_t>(base * percent) / 100u);
}

namespace {

int finishDamage(int damage, const DamageInput &input) noexcept {
    if (narrow(damage) < 0) {
        damage = 0;
    } else {
        damage = narrow(damage) + input.attackerStamina / 15;
        damage = narrow(damage) + input.targetHurt / 20;
        damage = applyDistanceDecay(damage, input.distance);
    }
    return narrow(damage) < 1 ? 1 : narrow(damage);
}

}

int calcDamage(const DamageInput &input, RandomSource &random) {
    int damage = (narrow(input.attack) - narrow(input.defence) * 3) * 2 / 3;
    damage += originalRandom(random, 20);
    damage -= originalRandom(random, 20);
    if (narrow(damage) <= 0) {
        damage = narrow(input.attack) / 10;
        damage += originalRandom(random, 4);
        damage -= originalRandom(random, 4);
    }
    return finishDamage(damage, input);
}

int predictDamage(const DamageInput &input) noexcept {
    int damage = (narrow(input.attack) - narrow(input.defence) * 3) * 2 / 3;
    if (narrow(damage) <= 0) {
        damage = narrow(input.attack) / 10;
    }
    return finishDamage(damage, input);
}

int poisonOnHit(int poisonAmp, int storedSkillLevel, int addPoison, int antipoison) noexcept {
    const int poison = (storedSkillLevel / 100 + 1) * addPoison + poisonAmp;
    if (poison <= antipoison || antipoison >= 90) { return 0; }
    return (poison - antipoison) / 15;
}

int poisonAmount(int poison, int antipoison) noexcept {
    int amount = (poison - antipoison) / 4;
    if (amount > 99) { amount = 99; }
    if (amount < 0) { amount = 0; }
    return amount;
}

int depoisonAmount(int depoison, int targetPoisoned, RandomSource &random) {
    int amount = depoison / 3;
    amount += originalRandom(random, 10);
    amount -= originalRandom(random, 10);
    if (amount > 99) { amount = 99; }
    if (amount < 0) { amount = 0; }
    if (targetPoisoned > depoison + 20) { amount = 0; }
    if (amount > targetPoisoned) { amount = targetPoisoned; }
    return amount;
}

int medicHeal(int medic, int targetHurt, RandomSource &random) {
    if (medic < 0) { medic = 0; }
    int heal;
    if (targetHurt <= 25) {
        heal = medic * 4 / 5;
    } else if (targetHurt <= 50) {
        heal = medic * 3 / 4;
    } else if (targetHurt <= 75) {
        heal = medic * 2 / 3;
    } else {
        heal = medic / 2;
    }
    return heal + originalRandom(random, 5);
}

RestGain restGain(int stamina, bool moved, RandomSource &random) {
    RestGain gain;
    gain.stamina = originalRandom(random, 3) + (moved ? 2 : 3);
    int newStamina = stamina + gain.stamina;
    if (newStamina > 100) {
        newStamina = 100;
        gain.stamina = newStamina - stamina;
    }
    if (newStamina < 30) { return gain; }
    const int bound = newStamina / 10 - 2;
    gain.hp = originalRandom(random, bound) + 3;
    gain.mp = originalRandom(random, bound) + 3;
    return gain;
}

int roundEndDrain(int hurt, int poisoned) noexcept {
    return hurt / 20 + poisoned / 10;
}

int applyBookStat(int current, int add, int limit) noexcept {
    int value = narrow(current + add);
    if (value > limit) { value = limit; }
    if (value < 0) { value = 0; }
    return value;
}

int potentialTier(int potential) noexcept {
    return 7 - potential / 15;
}

int levelUpFactor(int potential, RandomSource &random) {
    int bound;
    if (potential < 30) { bound = 2; }
    else if (potential < 50) { bound = 3; }
    else if (potential < 70) { bound = 4; }
    else if (potential < 90) { bound = 5; }
    else { bound = 6; }
    return originalRandom(random, bound) + 1;
}

LevelUpGain levelUpGain(int gainedLevels, int hpAddOnLevelUp, int factor, RandomSource &random) {
    LevelUpGain gain;
    if (gainedLevels <= 0) { return gain; }
    gain.maxHp = gainedLevels * 3 * (hpAddOnLevelUp + originalRandom(random, 3));
    gain.maxMp = gainedLevels * 4 * (9 - factor);
    gain.stat = gainedLevels * factor;
    return gain;
}

int throwDamage(int itemAddHp, int targetHurt, int throwing, RandomSource &random) {
    int base;
    if (targetHurt == 0) {
        base = itemAddHp / 4;
    } else if (targetHurt <= 33) {
        base = itemAddHp / 3;
    } else if (targetHurt <= 66) {
        base = itemAddHp / 2;
    } else {
        base = itemAddHp;
    }
    base -= originalRandom(random, 5);
    return (base - throwing * 2) / 3;
}

int throwPoison(int itemAddPoisoned, int throwing, int antipoison, RandomSource &random) {
    if (itemAddPoisoned > 0) {
        if (antipoison >= 100) { return 0; }
        int poison = (itemAddPoisoned - throwing) / 2 - antipoison;
        if (poison < 0) { poison = 0; }
        return poison / 2;
    }
    int poison = itemAddPoisoned / 2;
    poison += originalRandom(random, 5);
    poison -= originalRandom(random, 5);
    return poison;
}

}
