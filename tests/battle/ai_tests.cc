/*
 * Heroes of Jin Yong.
 * A reimplementation of the DOS game `The legend of Jin Yong Heroes`.
 * Copyright (C) 2021, Soar Qin<soarchin@gmail.com>
 */

/*
 * Contract tests for the battle ai. Every case names the evidence address in the
 * approved `Z.DAT` image; see docs/reverse/battle-evidence.md.
 */

#include "battle/ai.hh"
#include "battle/random.hh"
#include "test_support.hh"

#include <iostream>
#include <vector>

using namespace hojy::battle;

namespace {

AiStats healthy() {
    AiStats stats;
    stats.hp = 100; stats.maxHp = 100;
    stats.mp = 100; stats.maxMp = 100;
    stats.stamina = 100;
    stats.attack = 30;
    stats.minSkillReqMp = 0;
    return stats;
}

/* Pads a roll sequence so later cascade stages always have a value to draw. */
std::vector<int> rolls(std::vector<int> head, int filler = 9) {
    head.resize(head.size() + 64, filler);
    return head;
}

AiContext duel() {
    AiContext context;
    AiParticipant self;
    self.stats = healthy();
    self.side = 0;
    self.distance = 0;
    AiParticipant foe;
    foe.stats = healthy();
    foe.side = 1;
    foe.distance = 3;
    context.participants = {self, foe};
    context.self = 0;
    return context;
}

/* Z.DAT:0x336A0: low stamina always rests, whatever else is wrong. */
void restsOnLowStamina() {
    auto context = duel();
    context.participants[0].stats.stamina = 9;
    context.participants[0].stats.hp = 1;
    SequenceRandom random({});
    auto decision = decideAiAction(context, random);
    HOJY_CHECK_EQ(int(decision.action), int(AiAction::Rest));
    HOJY_CHECK_EQ(random.callCount(), 0U);
}

/* Z.DAT:0x336C2 and Z.DAT:0x33C4D */
void healsItselfWhenBadlyHurt() {
    {
        /* hp below 20 skips the random gates entirely and reaches for an item. */
        auto context = duel();
        context.participants[0].stats.hp = 19;
        context.items = {AiItem{7, 50, 0, 0}};
        SequenceRandom random({});
        auto decision = decideAiAction(context, random);
        HOJY_CHECK_EQ(int(decision.action), int(AiAction::UseItem));
        HOJY_CHECK_EQ(decision.itemSlot, 7);
        HOJY_CHECK_EQ(decision.target, 0);
        HOJY_CHECK_EQ(random.callCount(), 0U);
    }
    {
        /* Own medic proficiency comes before any item. */
        auto context = duel();
        context.participants[0].stats.hp = 19;
        context.participants[0].stats.medic = 40;
        context.items = {AiItem{7, 50, 0, 0}};
        SequenceRandom random({});
        auto decision = decideAiAction(context, random);
        HOJY_CHECK_EQ(int(decision.action), int(AiAction::Medic));
        HOJY_CHECK_EQ(decision.target, 0);
    }
    {
        /* Without a cure of its own it asks an ally and keeps fighting. */
        auto context = duel();
        context.participants[0].stats.hp = 19;
        AiParticipant ally;
        ally.stats = healthy();
        ally.stats.medic = 60;
        ally.side = 0;
        context.participants.push_back(ally);
        SequenceRandom random({});
        auto decision = decideAiAction(context, random);
        HOJY_CHECK_EQ(int(decision.action), int(AiAction::Attack));
        HOJY_CHECK_EQ(int(decision.request), int(AiRequest::Medic));
    }
    {
        /* A light wound only triggers on the weighted roll. */
        auto context = duel();
        context.participants[0].stats.hp = 49;   /* below maxHp / 2 */
        context.items = {AiItem{7, 50, 0, 0}};
        SequenceRandom failing(rolls({9}));     /* rnd(10) == 9, not below 3 */
        auto decision = decideAiAction(context, failing);
        if (decision.action == AiAction::UseItem) {
            throw std::runtime_error("a failed roll must not consume the item");
        }
        SequenceRandom passing(rolls({0}));
        decision = decideAiAction(context, passing);
        HOJY_CHECK_EQ(int(decision.action), int(AiAction::UseItem));
    }
}

/* Z.DAT:0x3380C: the poison stage always draws, and compares with poisoned / 10. */
void curesPoisonProportionally() {
    auto context = duel();
    context.participants[0].stats.poisoned = 60;
    context.items = {AiItem{3, 0, 0, -40}};
    SequenceRandom random({5});
    auto decision = decideAiAction(context, random);
    HOJY_CHECK_EQ(int(decision.action), int(AiAction::UseItem));
    HOJY_CHECK_EQ(decision.itemSlot, 3);
    HOJY_CHECK_EQ(random.callCount(), 1U);

    SequenceRandom tooLucky(rolls({6}));
    decision = decideAiAction(context, tooLucky);
    if (decision.action == AiAction::UseItem) {
        throw std::runtime_error("rnd(10) == 6 must not cure a poison value of 60");
    }
}

/* Z.DAT:0x3396B and Z.DAT:0x341F6 */
void healsAllies() {
    auto context = duel();
    context.participants[0].stats.medic = 80;   /* the last tier needs no roll */
    AiParticipant ally;
    ally.stats = healthy();
    ally.stats.hp = 10;
    ally.side = 0;
    context.participants.push_back(ally);
    SequenceRandom random(rolls({}));
    auto decision = decideAiAction(context, random);
    HOJY_CHECK_EQ(int(decision.action), int(AiAction::Medic));
    HOJY_CHECK_EQ(decision.target, 2);
}

/* A pending request bypasses the ally condition (Z.DAT:0x34278). */
void answersPendingRequests() {
    auto context = duel();
    context.participants[0].stats.medic = 80;
    AiParticipant ally;
    ally.stats = healthy();                     /* not hurt at all */
    ally.side = 0;
    ally.request = AiRequest::Medic;
    context.participants.push_back(ally);
    SequenceRandom random(rolls({}));
    auto decision = decideAiAction(context, random);
    HOJY_CHECK_EQ(int(decision.action), int(AiAction::Medic));
    HOJY_CHECK_EQ(decision.target, 2);
}

/* Z.DAT:0x347B2: poison needs to beat the attack and pass a second roll. */
void usesPoisonSkill() {
    auto context = duel();
    context.participants[0].stats.poison = 90;
    context.participants[0].stats.attack = 10;
    SequenceRandom random(rolls({0, 9, 0, 0}));
    auto decision = decideAiAction(context, random);
    HOJY_CHECK_EQ(int(decision.action), int(AiAction::Poison));

    /* A rnd(150) draw at or above the poison proficiency rejects it. */
    SequenceRandom unlucky(rolls({0, 9, 0, 100}));
    decision = decideAiAction(context, unlucky);
    if (decision.action == AiAction::Poison) {
        throw std::runtime_error("a high rnd(50) must reject the poison skill");
    }
}

/* Z.DAT:0x34823: the shared bag needs a stronger dart than a carried one. */
void throwsStrongItems() {
    {
        auto context = duel();
        context.participants[0].stats.throwing = 90;
        context.items = {AiItem{11, -60, 0, 0}};   /* 60 > attack 30 * 3 / 2 */
        SequenceRandom random(rolls({0, 9, 0, 30}));   /* cascade, then rnd(90) == 30 */
        auto decision = decideAiAction(context, random);
        HOJY_CHECK_EQ(int(decision.action), int(AiAction::Throw));
        HOJY_CHECK_EQ(decision.itemSlot, 11);
    }
    {
        /* 40 does not clear the player side bar of 45. */
        auto context = duel();
        context.participants[0].stats.throwing = 90;
        context.items = {AiItem{11, -40, 0, 0}};
        SequenceRandom random(rolls({0, 9, 0}));
        auto decision = decideAiAction(context, random);
        HOJY_CHECK_EQ(int(decision.action), int(AiAction::Attack));
    }
    {
        /* The enemy side only has to beat the raw attack. */
        auto context = duel();
        context.self = 1;
        context.participants[1].stats.throwing = 90;
        context.items = {AiItem{11, -40, 0, 0}};
        SequenceRandom random(rolls({0, 9, 0, 0}));
        auto decision = decideAiAction(context, random);
        HOJY_CHECK_EQ(int(decision.action), int(AiAction::Throw));
    }
}

/* Z.DAT:0x34A28: no usable skill at all falls through to resting. */
void restsWithoutUsableSkill() {
    auto context = duel();
    context.participants[0].stats.minSkillReqMp = 200;   /* more than the 100 mp held */
    context.participants[0].stats.poison = 0;
    SequenceRandom random(rolls({0, 9, 0}));
    auto decision = decideAiAction(context, random);
    HOJY_CHECK_EQ(int(decision.action), int(AiAction::Rest));
}

/* Z.DAT:0x3513A, 0x351A7, 0x35372 */
void picksTargetByTemperament() {
    AiContext context;
    AiParticipant self;
    self.stats = healthy();
    self.side = 0;
    AiParticipant weak, strong, near;
    weak.stats = healthy(); weak.stats.attack = 10; weak.side = 1; weak.distance = 9;
    strong.stats = healthy(); strong.stats.attack = 90; strong.side = 1; strong.distance = 7;
    near.stats = healthy(); near.stats.attack = 50; near.side = 1; near.distance = 1;
    context.participants = {self, weak, strong, near};
    context.self = 0;

    {
        /* An upright character goes for the most dangerous enemy. */
        context.participants[0].stats.integrity = 90;
        SequenceRandom random({0});
        HOJY_CHECK_EQ(pickAiTarget(context, random), 2);
    }
    {
        /* A ruthless one picks off the weakest. */
        context.participants[0].stats.integrity = 10;
        SequenceRandom random({0});
        HOJY_CHECK_EQ(pickAiTarget(context, random), 1);
    }
    {
        /* Otherwise the nearest enemy. */
        context.participants[0].stats.integrity = 50;
        context.participants[0].stats.potential = 0;
        SequenceRandom random({});
        HOJY_CHECK_EQ(pickAiTarget(context, random), 3);
    }
    {
        /* A failed temperament roll also falls back to the nearest. */
        context.participants[0].stats.integrity = 90;
        SequenceRandom random({9});
        HOJY_CHECK_EQ(pickAiTarget(context, random), 3);
    }
}

/* Z.DAT:0x35217: talent aims at the enemy support. */
void picksSupportTarget() {
    AiContext context;
    AiParticipant self;
    self.stats = healthy();
    self.stats.potential = 90;
    self.stats.integrity = 50;    /* keep the temperament branches out of the way */
    self.side = 0;
    AiParticipant healer, brute;
    healer.stats = healthy(); healer.stats.medic = 60; healer.stats.attack = 20;
    healer.side = 1; healer.distance = 8;
    brute.stats = healthy(); brute.stats.attack = 99; brute.side = 1; brute.distance = 1;
    context.participants = {self, healer, brute};
    context.self = 0;
    SequenceRandom random({0});
    HOJY_CHECK_EQ(pickAiTarget(context, random), 1);

    /* With poison on the own side the enemy curer comes first. */
    context.participants[0].stats.poison = 80;
    context.participants[2].stats.depoison = 50;
    SequenceRandom poisoner({0});
    HOJY_CHECK_EQ(pickAiTarget(context, poisoner), 2);
}

/* Z.DAT:0x355FF: poison targets must still be affected by the poison. */
void picksPoisonTarget() {
    AiContext context;
    AiParticipant self;
    self.stats = healthy();
    self.stats.poison = 60;
    self.side = 0;
    AiParticipant immune, saturated, valid;
    immune.stats = healthy(); immune.stats.antipoison = 80; immune.side = 1; immune.distance = 1;
    saturated.stats = healthy(); saturated.stats.poisoned = 95; saturated.side = 1; saturated.distance = 2;
    valid.stats = healthy(); valid.side = 1; valid.distance = 5;
    context.participants = {self, immune, saturated, valid};
    context.self = 0;
    SequenceRandom random({});
    HOJY_CHECK_EQ(pickAiPoisonTarget(context, random), 3);

    /* Nothing to poison at all. */
    context.participants.pop_back();
    SequenceRandom none({});
    HOJY_CHECK_EQ(pickAiPoisonTarget(context, none), -1);
}

/* Z.DAT:0x33ADE: the flee stage rolls twice before giving up on the fight. */
void fleesWhenDoomed() {
    auto context = duel();
    context.participants[0].stats.hp = 19;
    context.participants[0].stats.maxHp = 200;
    /* hp gate passes but nothing can heal, then the flee roll succeeds. */
    SequenceRandom random(rolls({0, 0}));
    auto decision = decideAiAction(context, random);
    HOJY_CHECK_EQ(int(decision.action), int(AiAction::Flee));
}

}

int main() {
    try {
        restsOnLowStamina();
        healsItselfWhenBadlyHurt();
        curesPoisonProportionally();
        healsAllies();
        answersPendingRequests();
        usesPoisonSkill();
        throwsStrongItems();
        restsWithoutUsableSkill();
        picksTargetByTemperament();
        picksSupportTarget();
        picksPoisonTarget();
        fleesWhenDoomed();
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
    return 0;
}
