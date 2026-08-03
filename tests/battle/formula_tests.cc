/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>
 */

/*
 * Contract tests for the battle number rules. Every case names the evidence
 * address in the approved `Z.DAT` image; see docs/reverse/battle-evidence.md.
 */

#include "battle/formulas.hh"
#include "battle/random.hh"
#include "test_support.hh"

#include <iostream>
#include <vector>

using namespace hojy::battle;

namespace {

/* NUM-SKILL-MPCOST Z.DAT:0x3850E, NUM-SKILL-LEVEL Z.DAT:0x39217 */
void skillLevelAndCost() {
    HOJY_CHECK_EQ(skillMpCost(10, 0), 0);
    HOJY_CHECK_EQ(skillMpCost(10, 1), 10);
    HOJY_CHECK_EQ(skillMpCost(10, 2), 10);
    HOJY_CHECK_EQ(skillMpCost(10, 3), 20);
    HOJY_CHECK_EQ(skillMpCost(10, 9), 50);
    HOJY_CHECK_EQ(skillMpCost(0, 9), 0);

    /* Free skills keep the stored level. */
    HOJY_CHECK_EQ(resolveSkillLevel(0, 7, 0), 7);
    /* Not even one cast worth of mp: the battle menu hides the skill. */
    HOJY_CHECK_EQ(resolveSkillLevel(10, 9, 9), -1);
    /* One step of mp covers levels 1 and 2, so the highest of those wins. */
    HOJY_CHECK_EQ(resolveSkillLevel(10, 9, 10), 2);
    HOJY_CHECK_EQ(resolveSkillLevel(10, 1, 10), 1);
    /* 25 mp affords ceil(level / 2) == 2 steps, so level 4. */
    HOJY_CHECK_EQ(resolveSkillLevel(10, 9, 25), 4);
    HOJY_CHECK_EQ(resolveSkillLevel(10, 9, 50), 9);
    /* The stored level is still the ceiling. */
    HOJY_CHECK_EQ(resolveSkillLevel(10, 3, 999), 3);
}

/* NUM-DAMAGE Z.DAT:0x39391, NUM-DAMAGE-DECAY Z.DAT:0x3944C */
void damage() {
    /* Two draws of rnd(20) when the main branch already yields a hit. */
    {
        SequenceRandom random({7, 2});
        DamageInput in{200, 20, 60, 40, 1};
        /* (200 - 60) * 2 / 3 == 93, +7 -2 == 98, +60/15 == 102, +40/20 == 104 */
        HOJY_CHECK_EQ(calcDamage(in, random), 104);
        HOJY_CHECK_EQ(random.callCount(), 2U);
    }
    /* Four draws when the main branch collapses to the attack/10 fallback. */
    {
        SequenceRandom random({0, 0, 3, 1});
        DamageInput in{100, 100, 0, 0, 1};
        /* (100 - 300) * 2 / 3 <= 0 -> 100/10 == 10, +3 -1 == 12 */
        HOJY_CHECK_EQ(calcDamage(in, random), 12);
        HOJY_CHECK_EQ(random.callCount(), 4U);
    }
    /* A negative fallback result is floored to the minimum of one damage. */
    {
        SequenceRandom random({0, 0, 0, 3});
        DamageInput in{5, 100, 0, 0, 1};
        HOJY_CHECK_EQ(calcDamage(in, random), 1);
    }

    /* Distance 1 is neutral, distance 0 is the original's 103% bonus. */
    HOJY_CHECK_EQ(applyDistanceDecay(100, 1), 100);
    HOJY_CHECK_EQ(applyDistanceDecay(100, 0), 103);
    HOJY_CHECK_EQ(applyDistanceDecay(100, 2), 97);
    HOJY_CHECK_EQ(applyDistanceDecay(100, 10), 73);
    HOJY_CHECK_EQ(applyDistanceDecay(100, 11), 66);
    HOJY_CHECK_EQ(applyDistanceDecay(100, 40), 66);

    /* The decayed result is still floored at one. */
    {
        DamageInput in{16, 0, 0, 0, 10};
        HOJY_CHECK_EQ(predictDamage(in), 7);
    }
    {
        DamageInput in{2, 0, 0, 0, 10};
        HOJY_CHECK_EQ(predictDamage(in), 1);
    }
    /* Prediction follows the same control flow with the random terms removed. */
    {
        SequenceRandom random({0, 0});
        DamageInput in{200, 20, 60, 40, 3};
        HOJY_CHECK_EQ(calcDamage(in, random), predictDamage(in));
    }
}

/* NUM-POISON-ONHIT Z.DAT:0x39529 */
void poisonOnAttack() {
    /* level term is `storedSkillLevel / 100 + 1`, not the mp limited level. */
    HOJY_CHECK_EQ(poisonOnHit(0, 250, 20, 0), 4);
    HOJY_CHECK_EQ(poisonOnHit(30, 0, 20, 0), 3);
    /* antipoison at or above 90 blocks it completely. */
    HOJY_CHECK_EQ(poisonOnHit(90, 900, 20, 90), 0);
    /* poison must strictly exceed antipoison. */
    HOJY_CHECK_EQ(poisonOnHit(0, 0, 20, 20), 0);
    HOJY_CHECK_EQ(poisonOnHit(0, 0, 20, 19), 0);
    HOJY_CHECK_EQ(poisonOnHit(0, 0, 40, 19), 1);
}

/* NUM-POISON-ACT Z.DAT:0x39A45 */
void poisonAction() {
    HOJY_CHECK_EQ(poisonAmount(80, 0), 20);
    HOJY_CHECK_EQ(poisonAmount(80, 40), 10);
    HOJY_CHECK_EQ(poisonAmount(10, 80), 0);
    HOJY_CHECK_EQ(poisonAmount(999, 0), 99);
}

/* NUM-DEPOISON-ACT Z.DAT:0x39DA3 */
void depoisonAction() {
    {
        SequenceRandom random({8, 3});
        /* 60 / 3 == 20, +8 -3 == 25, target holds more than that. */
        HOJY_CHECK_EQ(depoisonAmount(60, 70, random), 25);
        HOJY_CHECK_EQ(random.callCount(), 2U);
    }
    {
        /* Never removes more poison than the target carries. */
        SequenceRandom random({0, 0});
        HOJY_CHECK_EQ(depoisonAmount(60, 5, random), 5);
    }
    {
        /* poisoned > depoison + 20 is out of reach. */
        SequenceRandom random({0, 0});
        HOJY_CHECK_EQ(depoisonAmount(30, 51, random), 0);
    }
}

/* NUM-MEDIC-ACT Z.DAT:0x3A145 */
void medicAction() {
    {
        SequenceRandom random({4});
        HOJY_CHECK_EQ(medicHeal(50, 0, random), 44);   /* 50*4/5 + 4 */
    }
    {
        SequenceRandom random({0});
        HOJY_CHECK_EQ(medicHeal(50, 26, random), 37);  /* 50*3/4 */
    }
    {
        SequenceRandom random({0});
        HOJY_CHECK_EQ(medicHeal(50, 51, random), 33);  /* 50*2/3 */
    }
    {
        SequenceRandom random({0});
        HOJY_CHECK_EQ(medicHeal(50, 76, random), 25);  /* 50/2 */
    }
    {
        /* The random draw happens in every branch, boundaries included. */
        SequenceRandom random({1});
        HOJY_CHECK_EQ(medicHeal(50, 25, random), 41);
        HOJY_CHECK_EQ(random.callCount(), 1U);
    }
}

/* NUM-REST Z.DAT:0x3A8A4 */
void rest() {
    {
        /* Below 30 stamina nothing but stamina recovers, and no hp/mp draw. */
        SequenceRandom random({1});
        auto gain = restGain(10, false, random);
        HOJY_CHECK_EQ(gain.stamina, 4);
        HOJY_CHECK_EQ(gain.hp, 0);
        HOJY_CHECK_EQ(gain.mp, 0);
        HOJY_CHECK_EQ(random.callCount(), 1U);
    }
    {
        /* Having moved costs one point of the stamina bonus. */
        SequenceRandom random({0});
        HOJY_CHECK_EQ(restGain(10, true, random).stamina, 2);
    }
    {
        /* rnd(stamina/10 - 2) is skipped when the bound degenerates. */
        SequenceRandom random({0});
        auto gain = restGain(28, false, random);
        HOJY_CHECK_EQ(gain.stamina, 3);
        HOJY_CHECK_EQ(gain.hp, 3);
        HOJY_CHECK_EQ(gain.mp, 3);
        HOJY_CHECK_EQ(random.callCount(), 1U);
    }
    {
        SequenceRandom random({2, 4, 1});
        auto gain = restGain(90, false, random);
        HOJY_CHECK_EQ(gain.stamina, 5);
        HOJY_CHECK_EQ(gain.hp, 7);
        HOJY_CHECK_EQ(gain.mp, 4);
        HOJY_CHECK_EQ(random.callCount(), 3U);
    }
    {
        /* The stamina gain is reported after the 100 cap. */
        SequenceRandom random({2, 0, 0});
        HOJY_CHECK_EQ(restGain(99, false, random).stamina, 1);
    }
}

/* NUM-ROUND-DRAIN Z.DAT:0x3C563 */
void roundDrain() {
    HOJY_CHECK_EQ(roundEndDrain(0, 0), 0);
    HOJY_CHECK_EQ(roundEndDrain(40, 0), 2);
    HOJY_CHECK_EQ(roundEndDrain(0, 55), 5);
    HOJY_CHECK_EQ(roundEndDrain(99, 99), 13);
}

/* NUM-THROW Z.DAT:0x3A537, NUM-THROW-POISON Z.DAT:0x3A73B */
void throwing() {
    {
        /* addHp keeps the item's negative sign, so the result is negative. */
        SequenceRandom random({0});
        HOJY_CHECK_EQ(throwDamage(-120, 0, 30, random), -30);   /* (-30 - 60) / 3 */
        HOJY_CHECK_EQ(random.callCount(), 1U);
    }
    {
        SequenceRandom random({0});
        HOJY_CHECK_EQ(throwDamage(-120, 34, 30, random), -40);  /* (-60 - 60) / 3 */
    }
    {
        SequenceRandom random({0});
        HOJY_CHECK_EQ(throwDamage(-120, 67, 30, random), -60);  /* (-120 - 60) / 3 */
    }
    {
        /* Higher throwing proficiency deals more damage. */
        SequenceRandom random({0, 0});
        auto weak = throwDamage(-120, 0, 0, random);
        auto strong = throwDamage(-120, 0, 60, random);
        if (!(strong < weak)) { throw std::runtime_error("throwing must scale damage"); }
    }
    {
        /* Poisonous darts add poison and take no random draw. */
        SequenceRandom random({});
        HOJY_CHECK_EQ(throwPoison(100, 20, 10, random), 15);    /* ((100-20)/2 - 10) / 2 */
        HOJY_CHECK_EQ(random.callCount(), 0U);
    }
    {
        SequenceRandom random({});
        HOJY_CHECK_EQ(throwPoison(100, 20, 100, random), 0);
        HOJY_CHECK_EQ(throwPoison(10, 20, 40, random), 0);
    }
    {
        /* Curing darts lower poison and consume two draws. */
        SequenceRandom random({1, 2});
        HOJY_CHECK_EQ(throwPoison(-40, 0, 0, random), -21);
        HOJY_CHECK_EQ(random.callCount(), 2U);
    }
}

/* NUM-LEVELUP-FACTOR Z.DAT:0x3B79A, NUM-LEVELUP Z.DAT:0x3B7E5 */
void levelUp() {
    /* The roll widens in five potential tiers and is always at least one. */
    {
        SequenceRandom random({1, 2, 3, 4, 5});
        HOJY_CHECK_EQ(levelUpFactor(29, random), 2);    /* rnd(2) */
        HOJY_CHECK_EQ(levelUpFactor(49, random), 3);    /* rnd(3) */
        HOJY_CHECK_EQ(levelUpFactor(69, random), 4);    /* rnd(4) */
        HOJY_CHECK_EQ(levelUpFactor(89, random), 5);    /* rnd(5) */
        HOJY_CHECK_EQ(levelUpFactor(90, random), 6);    /* rnd(6) */
    }
    {
        /* Two levels at once scale every gain, and only one hp roll happens. */
        SequenceRandom random({2});
        auto gain = levelUpGain(2, 5, 3, random);
        HOJY_CHECK_EQ(gain.maxHp, 42);                  /* 2 * 3 * (5 + 2) */
        HOJY_CHECK_EQ(gain.maxMp, 48);                  /* 2 * 4 * (9 - 3) */
        HOJY_CHECK_EQ(gain.stat, 6);                    /* 2 * 3 */
        HOJY_CHECK_EQ(random.callCount(), 1U);
    }
    {
        /* A single level is the same rule with a count of one. */
        SequenceRandom random({0});
        auto gain = levelUpGain(1, 4, 1, random);
        HOJY_CHECK_EQ(gain.maxHp, 12);
        HOJY_CHECK_EQ(gain.maxMp, 32);
        HOJY_CHECK_EQ(gain.stat, 1);
    }
    {
        SequenceRandom random({});
        auto gain = levelUpGain(0, 4, 1, random);
        HOJY_CHECK_EQ(gain.maxHp, 0);
        HOJY_CHECK_EQ(random.callCount(), 0U);
    }
}

/* WAR-TRAIN Z.DAT:0x3BCE7 */
void bookStats() {
    HOJY_CHECK_EQ(applyBookStat(30, 10, 100), 40);
    /* The upper bound is applied first, then the lower one. */
    HOJY_CHECK_EQ(applyBookStat(95, 20, 100), 100);
    HOJY_CHECK_EQ(applyBookStat(100, 5, 100), 100);
    /* A negative modifier can take a proficiency down, but never below zero. */
    HOJY_CHECK_EQ(applyBookStat(10, -30, 100), 0);
    HOJY_CHECK_EQ(applyBookStat(0, 0, 100), 0);
}

/* WAR-TRAIN Z.DAT:0x3BB21, WAR-CRAFT Z.DAT:0x3C2E1 */
void potentialTiers() {
    /* Unclamped, so the low end really is 7 and the high end really is 1. */
    HOJY_CHECK_EQ(potentialTier(0), 7);
    HOJY_CHECK_EQ(potentialTier(14), 7);
    HOJY_CHECK_EQ(potentialTier(15), 6);
    HOJY_CHECK_EQ(potentialTier(45), 4);
    HOJY_CHECK_EQ(potentialTier(90), 1);
    HOJY_CHECK_EQ(potentialTier(100), 1);
}

/* The original random helper returns 0 without touching the generator. */
void randomGuard() {
    SequenceRandom random({5});
    HOJY_CHECK_EQ(originalRandom(random, 0), 0);
    HOJY_CHECK_EQ(originalRandom(random, 1), 0);
    HOJY_CHECK_EQ(originalRandom(random, -3), 0);
    HOJY_CHECK_EQ(originalRandom(random, 30001), 0);
    HOJY_CHECK_EQ(random.callCount(), 0U);
    HOJY_CHECK_EQ(originalRandom(random, 10), 5);
    HOJY_CHECK_EQ(random.callCount(), 1U);
}

}

int main() {
    try {
        skillLevelAndCost();
        damage();
        poisonOnAttack();
        poisonAction();
        depoisonAction();
        medicAction();
        rest();
        roundDrain();
        throwing();
        levelUp();
        potentialTiers();
        bookStats();
        randomGuard();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
