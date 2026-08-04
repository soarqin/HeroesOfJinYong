#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <variant>
#include <vector>

namespace hojy::battle {

using ParticipantId = std::size_t;
using InventorySnapshot =
    std::vector<std::pair<std::int16_t, std::int16_t>>;

struct BattleCell {
    std::int16_t x = 0;
    std::int16_t y = 0;
};

inline bool operator==(const BattleCell &left, const BattleCell &right) {
    return left.x == right.x && left.y == right.y;
}

struct ActionTarget {
    ParticipantId participant = 0;
    std::int16_t distance = 0;
};

inline bool operator==(const ActionTarget &left, const ActionTarget &right) {
    return left.participant == right.participant
        && left.distance == right.distance;
}

struct MoveAction {
    BattleCell from;
    BattleCell to;
};

inline bool operator==(const MoveAction &left, const MoveAction &right) {
    return left.from == right.from && left.to == right.to;
}

struct SkillAction {
    std::int16_t skillSlot = -1;
    std::int16_t skillId = -1;
    std::int16_t level = 0;
    std::vector<ActionTarget> targets;
};

inline bool operator==(const SkillAction &left, const SkillAction &right) {
    return left.skillSlot == right.skillSlot
        && left.skillId == right.skillId
        && left.level == right.level
        && left.targets == right.targets;
}

enum class Technique {
    Poison,
    Depoison,
    Medic,
};

struct TechniqueAction {
    Technique technique = Technique::Poison;
    ParticipantId target = 0;
};

inline bool operator==(const TechniqueAction &left,
                       const TechniqueAction &right) {
    return left.technique == right.technique && left.target == right.target;
}

enum class InventorySource {
    PartyBag,
    NpcCarry,
};

struct ThrowAction {
    ParticipantId target = 0;
    std::int16_t itemId = -1;
    InventorySource source = InventorySource::PartyBag;
    std::int16_t slot = -1;
};

inline bool operator==(const ThrowAction &left, const ThrowAction &right) {
    return left.target == right.target
        && left.itemId == right.itemId
        && left.source == right.source
        && left.slot == right.slot;
}

struct ItemAction {
    std::int16_t itemId = -1;
    InventorySource source = InventorySource::PartyBag;
    std::int16_t slot = -1;
};

inline bool operator==(const ItemAction &left, const ItemAction &right) {
    return left.itemId == right.itemId && left.source == right.source
        && left.slot == right.slot;
}

struct RestAction {
    bool moved = false;
};

inline bool operator==(const RestAction &left, const RestAction &right) {
    return left.moved == right.moved;
}

struct RoundEndAction {
    bool inactive = false;
};

inline bool operator==(const RoundEndAction &left,
                       const RoundEndAction &right) {
    return left.inactive == right.inactive;
}

// An attempted action that has no state effect but still consumes the
// actor's turn.  Scene adapters use this for rejected/dead targets so the
// action log remains complete without fabricating a combat result.
struct NoOpAction {
    std::int8_t reason = 0;
};

inline bool operator==(const NoOpAction &left, const NoOpAction &right) {
    return left.reason == right.reason;
}

using ActionPayload = std::variant<MoveAction, SkillAction, TechniqueAction,
                                   ThrowAction, ItemAction, RestAction,
                                   RoundEndAction, NoOpAction>;

struct BattleAction {
    ParticipantId actor = 0;
    ActionPayload payload;
};

inline bool operator==(const BattleAction &left, const BattleAction &right) {
    return left.actor == right.actor && left.payload == right.payload;
}

}
