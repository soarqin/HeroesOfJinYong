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

#pragma once

/*
 * Pure battle number rules, reconstructed from the original DOS build.
 * Evidence ids and addresses are listed in docs/reverse/battle-evidence.md;
 * every address below refers to the approved `Z.DAT` load image.
 */

namespace hojy::battle {

class RandomSource;

/*
 * The original random helper (`Z.DAT:0x3D612`) returns 0 without touching the
 * generator when the bound is not in 2..30000, so the number of generator
 * calls depends on the bound. Callers must go through this wrapper to keep the
 * random call sequence identical to the original.
 */
int originalRandom(RandomSource &random, int bound);

/* `NUM-SKILL-MPCOST` Z.DAT:0x3850E: reqMp * ((level + 1) / 2). */
int skillMpCost(int reqMp, int level) noexcept;

/*
 * `NUM-SKILL-LEVEL` Z.DAT:0x39217: the largest level not above `level` whose
 * mp cost is affordable. Returns -1 when the skill may not be selected at all,
 * matching the original battle menu gate (`Z.DAT:0x32F4E`, mp >= reqMp).
 */
int resolveSkillLevel(int reqMp, int level, int currMp) noexcept;

struct DamageInput {
    int attack = 0;          /* real attack, equipment and knowledge included */
    int defence = 0;         /* real defence, equipment and knowledge included */
    int attackerStamina = 0;
    int targetHurt = 0;
    int distance = 0;
};

/*
 * `NUM-DAMAGE-DECAY` Z.DAT:0x3944C. The original applies the decay for every
 * distance, so distance 0 yields a 103% multiplier and distance 1 yields 100%.
 */
int applyDistanceDecay(int damage, int distance) noexcept;

/* `NUM-DAMAGE` Z.DAT:0x39391-0x3948F. Consumes 2 or 4 random values. */
int calcDamage(const DamageInput &input, RandomSource &random);

/* Same control flow as calcDamage with the random terms removed. */
int predictDamage(const DamageInput &input) noexcept;

/*
 * `NUM-POISON-ONHIT` Z.DAT:0x39529. `storedSkillLevel` is the raw
 * `skillLevel[index]` value: the original uses `storedSkillLevel / 100 + 1`
 * here and ignores the mp-limited level used for damage.
 */
int poisonOnHit(int poisonAmp, int storedSkillLevel, int addPoison, int antipoison) noexcept;

/* `NUM-POISON-ACT` Z.DAT:0x39A45: (poison - antipoison) / 4, clamped to 0..99. */
int poisonAmount(int poison, int antipoison) noexcept;

/* `NUM-DEPOISON-ACT` Z.DAT:0x39DA3. Consumes 2 random values. */
int depoisonAmount(int depoison, int targetPoisoned, RandomSource &random);

/*
 * `NUM-MEDIC-ACT` Z.DAT:0x3A145. Consumes 1 random value in every branch.
 * The `hurt > medic + 20` rejection happens after the random draw, so callers
 * must apply it themselves instead of returning early.
 */
int medicHeal(int medic, int targetHurt, RandomSource &random);

struct RestGain {
    int stamina = 0;
    int hp = 0;
    int mp = 0;
};

/*
 * `NUM-REST` Z.DAT:0x3A8A4. `moved` is true when the actor already spent
 * movement steps this turn. Hp and mp only recover once the new stamina
 * reaches 30.
 */
RestGain restGain(int stamina, bool moved, RandomSource &random);

/*
 * `NUM-ROUND-DRAIN` Z.DAT:0x3C563: applied to every participant at the end of
 * a round, not at the start of each turn.
 */
int roundEndDrain(int hurt, int poisoned) noexcept;

/*
 * `NUM-LEVELUP-FACTOR` Z.DAT:0x3B79A: the growth roll widens with potential.
 * Consumes 1 random value.
 */
int levelUpFactor(int potential, RandomSource &random);

struct LevelUpGain {
    int maxHp = 0;
    int maxMp = 0;
    int stat = 0;    /* applied to attack, speed and defence alike */
};

/*
 * `NUM-LEVELUP` Z.DAT:0x3B7E5. The original resolves every level gained in one
 * step and scales the growth by that count, so it must not be called in a loop.
 * Consumes 1 random value.
 */
LevelUpGain levelUpGain(int gainedLevels, int hpAddOnLevelUp, int factor, RandomSource &random);

/*
 * `WAR-TRAIN` Z.DAT:0x3BB21 and `WAR-CRAFT` Z.DAT:0x3C2E1 share this tier:
 * `7 - potential / 15`, without any clamp. Higher potential means a smaller
 * multiplier and therefore faster progress.
 */
int potentialTier(int potential) noexcept;

/*
 * `WAR-TRAIN` Z.DAT:0x3BCE7: a skill book adds to a proficiency and then clamps
 * the result into 0..limit, upper bound first.
 */
int applyBookStat(int current, int add, int limit) noexcept;

/*
 * `NUM-THROW` Z.DAT:0x3A537. `itemAddHp` is the raw item value, negative for
 * damaging items, and the result keeps that sign. Consumes 1 random value.
 */
int throwDamage(int itemAddHp, int targetHurt, int throwing, RandomSource &random);

/*
 * `NUM-THROW-POISON` Z.DAT:0x3A73B. Returns the signed change applied to the
 * target's poisoned value. Consumes 2 random values for curing items and none
 * for poisonous ones.
 */
int throwPoison(int itemAddPoisoned, int throwing, int antipoison, RandomSource &random);

}
