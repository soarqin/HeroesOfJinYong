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
 * Battle AI decision rules, reconstructed from the original DOS build.
 * The entry point is `Z.DAT:0x33599`, which walks eight stages and produces one
 * of twelve action codes; the codes that share a handler are merged here.
 * Addresses refer to the approved `Z.DAT` load image, see
 * docs/reverse/battle-evidence.md.
 */

#include <vector>

namespace hojy::battle {

class RandomSource;

enum class AiAction {
    Rest,       /* original 0 and 7 */
    Attack,     /* original 1 and 2: a random learnt skill on the picked target */
    Poison,     /* original 3 */
    Depoison,   /* original 4 */
    Medic,      /* original 5 */
    UseItem,    /* original 6 */
    Throw,      /* original 10 */
    Flee,       /* original 11 */
};

/*
 * A character that could not help itself records what it needs, and the next
 * ally with the matching proficiency treats that as an unconditional request
 * (Z.DAT:0x34278 and Z.DAT:0x3445C). The values are the original action codes.
 */
enum class AiRequest {
    None = 0,
    Medic = 8,
    Depoison = 9,
};

struct AiStats {
    int hp = 0, maxHp = 0, mp = 0, maxMp = 0;
    int stamina = 0, hurt = 0, poisoned = 0;
    int attack = 0, medic = 0, poison = 0, depoison = 0, antipoison = 0, throwing = 0;
    int integrity = 0, potential = 0;
    /* Smallest `reqMp` among the learnt skills, or -1 when nothing is learnt. */
    int minSkillReqMp = -1;
};

struct AiParticipant {
    AiStats stats;
    int side = 0;
    bool active = true;            /* still standing on the field */
    AiRequest request = AiRequest::None;
    /* Attack-range grid distance from the acting character, negative when unreachable. */
    int distance = -1;
};

struct AiItem {
    int slot = -1;                 /* caller defined identifier, echoed in the decision */
    int addHp = 0;
    int addMp = 0;
    int addPoisoned = 0;
};

struct AiContext {
    std::vector<AiParticipant> participants;
    std::vector<AiItem> items;     /* shared bag for side 0, carried items otherwise */
    int self = 0;
};

struct AiDecision {
    AiAction action = AiAction::Rest;
    int target = -1;               /* participant index, -1 when the action has no target */
    int itemSlot = -1;             /* AiItem::slot for AiAction::UseItem and AiAction::Throw */
    AiRequest request = AiRequest::None;  /* what the acting character now waits for */
};

/* `AI-ACTION` Z.DAT:0x33599 plus the stage handlers it calls. */
AiDecision decideAiAction(const AiContext &context, RandomSource &random);

/*
 * `AI-TARGET` Z.DAT:0x3505B: an attack target chosen from the character's
 * temperament. Returns -1 when no living enemy is left.
 */
int pickAiTarget(const AiContext &context, RandomSource &random);

/*
 * `AI-TARGET-POISON` Z.DAT:0x355FF: a poison target, restricted to enemies that
 * the poison can still affect. Returns -1 when there is none.
 */
int pickAiPoisonTarget(const AiContext &context, RandomSource &random);

}
