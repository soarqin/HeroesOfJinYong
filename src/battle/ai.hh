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
 * Compatibility-level battle AI data contracts reconstructed from the
 * original DOS build.  Runtime scene decisions live in ai_policy and
 * ai_strategy; addresses refer to the approved Z.DAT load image recorded in
 * docs/reverse/battle-evidence.md.
 */

#include "world/character.hh"

#include <vector>

namespace hojy::battle {

class RandomSource;

enum class AiAction {
    Rest,       /* original 0 and 7 */
    Attack,     /* original 1 and 2 */
    Poison,     /* original 3 */
    Depoison,   /* original 4 */
    Medic,      /* original 5 */
    UseItem,    /* original 6 */
    Throw,      /* original 10 */
    Flee,       /* original 11 */
};

/* Unresolved self-recovery records a persistent request for a later ally.
 * The values are the original action codes. */
enum class AiRequest {
    None = 0,
    Medic = 8,
    Depoison = 9,
};

struct AiStats {
    int hp = 0, maxHp = 0, mp = 0, maxMp = 0;
    int stamina = 0, hurt = 0, poisoned = 0;
    int attack = 0, medic = 0, poison = 0, depoison = 0;
    int antipoison = 0, throwing = 0;
    int integrity = 0, potential = 0;
    /* Smallest reqMp among learnt skills, or -1 when nothing is learnt. */
    int minSkillReqMp = -1;
};

/*
 * Capture the battle-runtime properties before the scene applies effective
 * equipment bonuses to its combat copy.  The original AI reads these runtime
 * fields directly; planning must not accidentally use the post-equipment
 * presentation/calculation values.
 */
AiStats snapshotAiStats(const ::hojy::world::state::CharacterData &info) noexcept;

/* Record only the static AI properties contributed by the effective
 * equipment copy.  Keeping this delta lets battle-time mutations remain
 * visible without feeding equipment bonuses back into AI planning. */
AiStats captureAiEquipmentBonuses(const AiStats &entry,
                                  const ::hojy::world::state::CharacterData &effective) noexcept;

/* Reconstruct current runtime AI properties from the entry snapshot, the
 * static equipment delta, and the mutable battle copy. */
AiStats resolveAiRuntimeStats(const AiStats &entry,
                              const AiStats &equipmentBonus,
                              const ::hojy::world::state::CharacterData &effective) noexcept;

struct AiParticipant {
    AiStats stats;
    int side = 0;
    bool active = true;            /* still standing on the field */
    AiRequest request = AiRequest::None;
    /* Grid distance from the actor, negative when unreachable. */
    int distance = -1;
};

struct AiItem {
    int slot = -1;                 /* caller-defined identifier */
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
    int target = -1;               /* participant index, or -1 */
    int itemSlot = -1;             /* AiItem::slot for item actions */
    AiRequest request = AiRequest::None;  /* request left by the actor */
};

/* Compatibility facade for the original public AI snapshot API.  Runtime
 * scene code uses ai_policy/ai_strategy directly; this adapter intentionally
 * contains no duplicate decision rules. */
AiDecision decideAiAction(const AiContext &context, RandomSource &random);
/* Z.DAT:0x3505B attack-target compatibility entry point. */
int pickAiTarget(const AiContext &context, RandomSource &random);
/* Z.DAT:0x355FF poison-target compatibility entry point. */
int pickAiPoisonTarget(const AiContext &context, RandomSource &random);

}
