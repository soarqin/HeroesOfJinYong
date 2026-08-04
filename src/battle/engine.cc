#include "engine.hh"

#include "content/constants.hh"

#include <algorithm>
#include <array>
#include <cstring>
#include <set>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace hojy::battle {
namespace {

void hashBytes(std::uint64_t &hash, const void *data, std::size_t size) {
    const auto *bytes = static_cast<const unsigned char *>(data);
    for (std::size_t index = 0; index < size; ++index) {
        hash ^= bytes[index];
        hash *= 1099511628211ULL;
    }
}

template<typename T>
void hashValue(std::uint64_t &hash, const T &value) {
    hashBytes(hash, &value, sizeof(value));
}

std::uint64_t actionIntegrity(
        const BattleAction &action,
        const std::vector<bool> &enemy,
        const std::vector<::hojy::world::state::CharacterData> &beforeParticipants,
        const InventorySnapshot &beforeInventory,
        const std::vector<::hojy::world::state::CharacterData> &participants,
        const InventorySnapshot &inventory,
        std::size_t randomBegin,
        std::size_t randomEnd) {
    std::uint64_t hash = 1469598103934665603ULL;
    hashValue(hash, action.actor);
    const auto payloadIndex = action.payload.index();
    hashValue(hash, payloadIndex);
    std::visit([&](const auto &payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, MoveAction>) {
            hashValue(hash, payload.from.x);
            hashValue(hash, payload.from.y);
            hashValue(hash, payload.to.x);
            hashValue(hash, payload.to.y);
        } else if constexpr (std::is_same_v<T, SkillAction>) {
            hashValue(hash, payload.skillSlot);
            hashValue(hash, payload.skillId);
            hashValue(hash, payload.level);
            hashValue(hash, payload.targets.size());
            for (const auto &target: payload.targets) {
                hashValue(hash, target.participant);
                hashValue(hash, target.distance);
            }
        } else if constexpr (std::is_same_v<T, TechniqueAction>) {
            hashValue(hash, payload.technique);
            hashValue(hash, payload.target);
        } else if constexpr (std::is_same_v<T, ThrowAction>) {
            hashValue(hash, payload.target);
            hashValue(hash, payload.itemId);
            hashValue(hash, payload.source);
            hashValue(hash, payload.slot);
        } else if constexpr (std::is_same_v<T, ItemAction>) {
            hashValue(hash, payload.itemId);
            hashValue(hash, payload.source);
            hashValue(hash, payload.slot);
        } else if constexpr (std::is_same_v<T, RestAction>) {
            hashValue(hash, payload.moved);
        } else if constexpr (std::is_same_v<T, RoundEndAction>) {
            hashValue(hash, payload.inactive);
        } else if constexpr (std::is_same_v<T, NoOpAction>) {
            hashValue(hash, payload.reason);
        }
    }, action.payload);
    hashValue(hash, enemy.size());
    for (const auto value: enemy) {
        hashValue(hash, value);
    }
    hashValue(hash, beforeParticipants.size());
    for (const auto &participant: beforeParticipants) {
        hashBytes(hash, &participant, sizeof(participant));
    }
    hashValue(hash, beforeInventory.size());
    for (const auto &[itemId, count]: beforeInventory) {
        hashValue(hash, itemId);
        hashValue(hash, count);
    }
    hashValue(hash, participants.size());
    for (const auto &participant: participants) {
        hashBytes(hash, &participant, sizeof(participant));
    }
    hashValue(hash, inventory.size());
    for (const auto &[itemId, count]: inventory) {
        hashValue(hash, itemId);
        hashValue(hash, count);
    }
    hashValue(hash, randomBegin);
    hashValue(hash, randomEnd);
    return hash;
}

std::uint64_t snapshotIntegrity(
        const std::vector<::hojy::world::state::CharacterData> &participants,
        const std::vector<bool> &enemy,
        const InventorySnapshot &inventory,
        bool won,
        const std::vector<RandomCall> &randomCalls,
        bool committed,
        std::size_t settlementRandomBegin) {
    std::uint64_t hash = 1469598103934665603ULL;
    hashValue(hash, participants.size());
    for (const auto &participant: participants) {
        hashBytes(hash, &participant, sizeof(participant));
    }
    hashValue(hash, inventory.size());
    for (const auto &[itemId, count]: inventory) {
        hashValue(hash, itemId);
        hashValue(hash, count);
    }
    hashValue(hash, enemy.size());
    for (const auto value: enemy) {
        hashValue(hash, value);
    }
    hashValue(hash, won);
    hashValue(hash, committed);
    hashValue(hash, settlementRandomBegin);
    hashValue(hash, randomCalls.size());
    for (const auto &call: randomCalls) {
        hashValue(hash, call.minimum);
        hashValue(hash, call.maximum);
        hashValue(hash, call.rawValue);
        hashValue(hash, call.result);
    }
    return hash;
}

bool alive(const std::vector<::hojy::world::state::CharacterData> &state,
           ParticipantId participant) {
    return participant < state.size() && state[participant].hp > 0;
}

bool validInventorySource(InventorySource source) noexcept {
    switch (source) {
    case InventorySource::PartyBag:
    case InventorySource::NpcCarry:
        return true;
    }
    return false;
}

bool sameInventory(const InventorySnapshot &left,
                   const InventorySnapshot &right) {
    // The vector order is the DOS bag-slot order.  Treating snapshots as a
    // map would allow a replay to silently reorder slots while preserving
    // item counts, which changes subsequent AI selection and save encoding.
    return left == right;
}

bool consumedFromPartyBag(const InventorySnapshot &before,
                          const InventorySnapshot &after,
                          std::int16_t itemId) {
    if (itemId < 0) { return false; }
    auto expected = before;
    const auto ite = std::find_if(
        expected.begin(), expected.end(),
        [itemId](const auto &entry) { return entry.first == itemId; });
    if (ite == expected.end() || ite->second <= 0) { return false; }
    if (--ite->second == 0) {
        expected.erase(ite);
    }
    return expected == after;
}

bool validInventoryTransition(const BattleAction &action,
                              const InventorySnapshot &before,
                              const InventorySnapshot &after) {
    return std::visit([&](const auto &payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, ThrowAction>) {
            return payload.source == InventorySource::PartyBag
                ? consumedFromPartyBag(before, after, payload.itemId)
                : sameInventory(before, after);
        } else if constexpr (std::is_same_v<T, ItemAction>) {
            return payload.source == InventorySource::PartyBag
                ? consumedFromPartyBag(before, after, payload.itemId)
                : sameInventory(before, after);
        } else {
            return sameInventory(before, after);
        }
    }, action.payload);
}

bool npcCarryConsumed(
        const ::hojy::world::state::CharacterData &before,
        const ::hojy::world::state::CharacterData &after,
        std::int16_t slot, std::int16_t itemId) {
    if (slot < 0 || slot >= ::hojy::content::CarryItemCount
        || before.item[slot] != itemId || before.itemCount[slot] <= 0) {
        return false;
    }
    std::array<std::int16_t, ::hojy::content::CarryItemCount> items{};
    std::array<std::int16_t, ::hojy::content::CarryItemCount> counts{};
    for (int index = 0; index < ::hojy::content::CarryItemCount; ++index) {
        items[index] = before.item[index];
        counts[index] = before.itemCount[index];
    }
    if (--counts[slot] <= 0) {
        for (int index = slot;
             index + 1 < ::hojy::content::CarryItemCount; ++index) {
            items[index] = items[index + 1];
            counts[index] = counts[index + 1];
        }
        items.back() = -1;
        counts.back() = 0;
    }
    for (int index = 0; index < ::hojy::content::CarryItemCount; ++index) {
        if (after.item[index] != items[index]
            || after.itemCount[index] != counts[index]) {
            return false;
        }
    }
    return true;
}

bool validParticipantTransition(
        const BattleAction &action,
        const std::vector<::hojy::world::state::CharacterData> &before,
        const std::vector<::hojy::world::state::CharacterData> &after) {
    if (before.size() != after.size()) { return false; }

    const auto participantMayChange = [&](ParticipantId participant) {
        if (participant == action.actor) { return true; }
        return std::visit([participant](const auto &payload) {
            using T = std::decay_t<decltype(payload)>;
            if constexpr (std::is_same_v<T, SkillAction>) {
                return std::any_of(
                    payload.targets.begin(), payload.targets.end(),
                    [participant](const auto &target) {
                        return target.participant == participant;
                    });
            } else if constexpr (std::is_same_v<T, TechniqueAction>) {
                return payload.target == participant;
            } else if constexpr (std::is_same_v<T, ThrowAction>) {
                return payload.target == participant;
            } else {
                return false;
            }
        }, action.payload);
    };

    for (std::size_t index = 0; index < before.size(); ++index) {
        if (!participantMayChange(index)
            && std::memcmp(&before[index], &after[index],
                           sizeof(before[index])) != 0) {
            return false;
        }
    }

    return std::visit([&](const auto &payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, ItemAction>) {
            if (payload.source != InventorySource::NpcCarry) { return true; }
            return action.actor < before.size() && action.actor < after.size()
                && npcCarryConsumed(before[action.actor], after[action.actor],
                                    payload.slot, payload.itemId);
        } else if constexpr (std::is_same_v<T, ThrowAction>) {
            if (payload.source != InventorySource::NpcCarry) { return true; }
            return action.actor < before.size() && action.actor < after.size()
                && npcCarryConsumed(before[action.actor], after[action.actor],
                                    payload.slot, payload.itemId);
        } else if constexpr (std::is_same_v<T, NoOpAction>) {
            if (before.size() != after.size()) { return false; }
            for (std::size_t index = 0; index < before.size(); ++index) {
                if (std::memcmp(&before[index], &after[index],
                                sizeof(before[index])) != 0) {
                    return false;
                }
            }
            return true;
        } else {
            return true;
        }
    }, action.payload);
}

}

BattleEngine::~BattleEngine() {
    if (status_ != EngineStatus::Idle) {
        discardParticipants();
    }
}

void BattleEngine::abort() noexcept {
    if (status_ == EngineStatus::Idle) { return; }
    discardParticipants();
    setup_ = {};
    initialParticipants_.clear();
    initialInventory_.clear();
    inventory_.clear();
    actions_.clear();
    randomCalls_.clear();
    sourceRandomCursor_ = 0;
    status_ = EngineStatus::Idle;
    won_ = false;
}

bool BattleEngine::validateInventory(const InventorySnapshot &inventory) {
    std::set<std::int16_t> itemIds;
    for (const auto &[itemId, count]: inventory) {
        if (itemId < 0 || count <= 0 || !itemIds.insert(itemId).second) {
            return false;
        }
    }
    return true;
}

bool BattleEngine::begin(BattleSetup setup) {
    if (status_ != EngineStatus::Idle) {
        return false;
    }
    const auto fail = [&setup, this]() {
        for (auto *participant: setup.participants) {
            if (participant) { participant->discard(); }
        }
        status_ = EngineStatus::Faulted;
        return false;
    };
    if (setup.participants.empty()
        || setup.enemy.size() != setup.participants.size()
        || !validateInventory(setup.inventory)) {
        return fail();
    }

    bool playerPresent = false;
    bool enemyPresent = false;
    std::vector<BattleParticipant *> uniqueParticipants;
    std::vector<::hojy::world::state::CharacterData> initialParticipants;
    uniqueParticipants.reserve(setup.participants.size());
    initialParticipants.reserve(setup.participants.size());
    for (std::size_t index = 0; index < setup.participants.size(); ++index) {
        auto *participant = setup.participants[index];
        if (!participant
            || std::find(uniqueParticipants.begin(), uniqueParticipants.end(),
                         participant) != uniqueParticipants.end()) {
            return fail();
        }
        uniqueParticipants.push_back(participant);
        initialParticipants.push_back(participant->state());
        if (setup.enemy[index]) {
            enemyPresent = true;
        } else {
            playerPresent = true;
        }
    }
    if (!playerPresent || !enemyPresent) {
        return fail();
    }

    setup_ = std::move(setup);
    initialParticipants_ = std::move(initialParticipants);
    initialInventory_ = setup_.inventory;
    inventory_ = initialInventory_;
    actions_.clear();
    randomCalls_.clear();
    sourceRandomCursor_ = 0;
    if (const auto *calls = sourceRandomCalls()) {
        sourceRandomCursor_ = calls->size();
    }
    status_ = EngineStatus::Active;
    won_ = false;
    updateFinishedState();
    return status_ != EngineStatus::Faulted;
}

bool BattleEngine::validSlot(std::size_t index) const noexcept {
    return index < setup_.participants.size()
        && setup_.participants[index] != nullptr;
}

bool BattleEngine::validateAction(
        const BattleAction &action,
        const std::vector<::hojy::world::state::CharacterData> &state) const {
    return validateAction(action, setup_.enemy, state);
}

bool BattleEngine::validateAction(
        const BattleAction &action,
        const std::vector<bool> &enemy,
        const std::vector<::hojy::world::state::CharacterData> &state) {
    if (enemy.size() != state.size() || action.actor >= state.size()) {
        return false;
    }
    const bool roundEnd = std::holds_alternative<RoundEndAction>(action.payload);
    if (!roundEnd && !alive(state, action.actor)) {
        return false;
    }

    return std::visit([&](const auto &payload) {
        using T = std::decay_t<decltype(payload)>;
        if constexpr (std::is_same_v<T, MoveAction>) {
            return !(payload.from == payload.to);
        } else if constexpr (std::is_same_v<T, SkillAction>) {
            const auto level = payload.level;
            if (payload.skillSlot < 0 || payload.skillId < 0
                || payload.skillSlot >= ::hojy::content::LearnSkillCount
                || level < 0
                || level > ::hojy::content::SkillLevelMaxDiv) {
                return false;
            }
            std::set<ParticipantId> targets;
            for (const auto &target: payload.targets) {
                if (target.distance < 0 || !alive(state, target.participant)
                    || enemy[target.participant] == enemy[action.actor]
                    || !targets.insert(target.participant).second) {
                    return false;
                }
            }
            return true;
        } else if constexpr (std::is_same_v<T, TechniqueAction>) {
            if (!alive(state, payload.target)) {
                return false;
            }
            const bool sameSide = enemy[payload.target] == enemy[action.actor];
            switch (payload.technique) {
            case Technique::Poison:
                return !sameSide;
            case Technique::Depoison:
            case Technique::Medic:
                return sameSide;
            }
            return false;
        } else if constexpr (std::is_same_v<T, ThrowAction>) {
            if (payload.itemId < 0 || !alive(state, payload.target)
                || enemy[payload.target] == enemy[action.actor]
                || !validInventorySource(payload.source)
                || (payload.source == InventorySource::NpcCarry
                    && (payload.slot < 0 || payload.slot >= ::hojy::content::CarryItemCount))
                || (payload.source == InventorySource::PartyBag
                    && payload.slot != -1)
                || ((payload.source == InventorySource::NpcCarry)
                    != enemy[action.actor])) {
                return false;
            }
            return true;
        } else if constexpr (std::is_same_v<T, ItemAction>) {
            return payload.itemId >= 0 && validInventorySource(payload.source)
                && (payload.source == InventorySource::NpcCarry
                    ? payload.slot >= 0
                        && payload.slot < ::hojy::content::CarryItemCount
                    : payload.slot == -1)
                && ((payload.source == InventorySource::NpcCarry)
                    == enemy[action.actor]);
        } else if constexpr (std::is_same_v<T, RestAction>) {
            return true;
        } else if constexpr (std::is_same_v<T, RoundEndAction>) {
            return true;
        } else if constexpr (std::is_same_v<T, NoOpAction>) {
            return true;
        }
        return false;
    }, action.payload);
}

std::vector<::hojy::world::state::CharacterData>
BattleEngine::participantState() const {
    std::vector<::hojy::world::state::CharacterData> result;
    result.reserve(setup_.participants.size());
    for (const auto *participant: setup_.participants) {
        result.push_back(participant ? participant->state()
                                     : ::hojy::world::state::CharacterData{});
    }
    return result;
}

const std::vector<RandomCall> *BattleEngine::sourceRandomCalls() const noexcept {
    if (const auto *recording = dynamic_cast<const RecordingRandom *>(setup_.random)) {
        return &recording->calls();
    }
    if (const auto *sequence = dynamic_cast<const SequenceRandom *>(setup_.random)) {
        return &sequence->calls();
    }
    return nullptr;
}

void BattleEngine::captureRandomCalls() {
    const auto *calls = sourceRandomCalls();
    if (!calls || sourceRandomCursor_ >= calls->size()) {
        return;
    }
    randomCalls_.insert(
        randomCalls_.end(),
        calls->begin() + static_cast<std::ptrdiff_t>(sourceRandomCursor_),
        calls->end());
    sourceRandomCursor_ = calls->size();
}

void BattleEngine::restoreParticipants(
        const std::vector<::hojy::world::state::CharacterData> &state) noexcept {
    if (state.size() != setup_.participants.size()) { return; }
    for (std::size_t index = 0; index < state.size(); ++index) {
        if (setup_.participants[index]) {
            setup_.participants[index]->state() = state[index];
        }
    }
}

bool BattleEngine::record(const BattleAction &action,
                          InventorySnapshot inventory) {
    if (status_ != EngineStatus::Active || !validateInventory(inventory)) {
        status_ = EngineStatus::Faulted;
        return false;
    }
    const auto &before = actions_.empty()
        ? initialParticipants_ : actions_.back().participants;
    const auto &beforeInventory = actions_.empty()
        ? initialInventory_ : actions_.back().inventory;
    if (!validateAction(action, before)) {
        status_ = EngineStatus::Faulted;
        return false;
    }
    if (!validInventoryTransition(action, beforeInventory, inventory)) {
        status_ = EngineStatus::Faulted;
        return false;
    }

    const auto state = participantState();
    if (!validParticipantTransition(action, before, state)) {
        restoreParticipants(before);
        status_ = EngineStatus::Faulted;
        return false;
    }
    const auto randomBegin = randomCalls_.size();
    captureRandomCalls();
    const auto randomEnd = randomCalls_.size();
    inventory_ = std::move(inventory);
    actions_.push_back(ActionRecord{
        action, randomBegin, randomEnd, state, inventory_,
        actionIntegrity(action, setup_.enemy, before, beforeInventory,
                        state, inventory_, randomBegin, randomEnd),
    });
    return true;
}

bool BattleEngine::reconcile() {
    if (status_ != EngineStatus::Active) {
        return false;
    }
    updateFinishedState();
    return true;
}

bool BattleEngine::reconcile(InventorySnapshot inventory) {
    if (status_ != EngineStatus::Active && status_ != EngineStatus::Finished) {
        return false;
    }
    if (!validateInventory(inventory)) {
        status_ = EngineStatus::Faulted;
        return false;
    }
    inventory_ = std::move(inventory);
    if (status_ == EngineStatus::Active) {
        updateFinishedState();
    }
    return true;
}

void BattleEngine::updateFinishedState(
        const std::vector<bool> &enemy,
        const std::vector<::hojy::world::state::CharacterData> &participants,
        bool &finished, bool &won) noexcept {
    bool playerAlive = false;
    bool enemyAlive = false;
    for (std::size_t index = 0; index < participants.size()
                               && index < enemy.size(); ++index) {
        if (participants[index].hp <= 0) {
            continue;
        }
        if (enemy[index]) {
            enemyAlive = true;
        } else {
            playerAlive = true;
        }
    }
    finished = !enemyAlive || !playerAlive;
    won = playerAlive && !enemyAlive;
}

void BattleEngine::updateFinishedState() noexcept {
    bool finished = false;
    bool won = false;
    updateFinishedState(setup_.enemy, participantState(), finished, won);
    if (finished) {
        status_ = EngineStatus::Finished;
        won_ = won;
    }
}

BattleSnapshot BattleEngine::snapshot() const {
    BattleSnapshot result;
    result.status = status_;
    result.won = won_;
    result.actions = actions_.size();
    result.participants = participantState();
    result.inventory = inventory_;
    result.actionLog = actions_;
    result.randomCalls = randomCalls_;
    return result;
}

void BattleEngine::discardParticipants() noexcept {
    for (auto *participant: setup_.participants) {
        if (participant) {
            participant->discard();
        }
    }
}

BattleResult BattleEngine::finish(bool commit) {
    const auto settlementRandomBegin = randomCalls_.size();
    captureRandomCalls();
    const bool canCommit = commit && status_ == EngineStatus::Finished;
    BattleResult result;
    result.committed = canCommit;
    result.won = won_;
    result.actions = actions_.size();
    const auto finalParticipants = participantState();
    const auto finalInventory = inventory_;
    result.replay = BattleReplay{
        initialParticipants_, setup_.enemy, initialInventory_, actions_,
        randomCalls_, won_, finalParticipants, finalInventory,
        snapshotIntegrity(finalParticipants, setup_.enemy, finalInventory,
                          won_, randomCalls_, canCommit,
                          settlementRandomBegin),
        snapshotIntegrity(initialParticipants_, setup_.enemy,
                          initialInventory_, false, {}, false, 0),
        canCommit,
        settlementRandomBegin,
    };
    if (canCommit) {
        for (auto *participant: setup_.participants) {
            if (participant) {
                participant->commit();
            }
        }
    } else {
        discardParticipants();
    }
    setup_ = {};
    initialParticipants_.clear();
    initialInventory_.clear();
    inventory_.clear();
    actions_.clear();
    randomCalls_.clear();
    sourceRandomCursor_ = 0;
    status_ = EngineStatus::Idle;
    won_ = false;
    return result;
}

ReplayResult BattleEngine::replay(const BattleReplay &replayData) {
    ReplayResult result;
    if (replayData.initialParticipants.empty()
        || replayData.enemy.size() != replayData.initialParticipants.size()
        || !validateInventory(replayData.initialInventory)
        || !replayData.committed
        || replayData.initialIntegrity == 0
        || replayData.initialIntegrity != snapshotIntegrity(
            replayData.initialParticipants, replayData.enemy,
            replayData.initialInventory, false, {}, false, 0)) {
        result.error = "invalid replay setup";
        return result;
    }
    bool playerPresent = false;
    bool enemyPresent = false;
    for (const auto value: replayData.enemy) {
        playerPresent = playerPresent || !value;
        enemyPresent = enemyPresent || value;
    }
    if (!playerPresent || !enemyPresent) {
        result.error = "invalid replay sides";
        return result;
    }

    try {
        std::vector<std::int64_t> rawValues;
        rawValues.reserve(replayData.randomCalls.size());
        for (const auto &call: replayData.randomCalls) {
            rawValues.push_back(call.rawValue);
        }
        SequenceRandom random(std::move(rawValues));
        for (const auto &call: replayData.randomCalls) {
            if (random.next(call.minimum, call.maximum) != call.result) {
                result.error = "random replay mismatch";
                return result;
            }
            if (random.calls().empty() || !(random.calls().back() == call)) {
                result.error = "random replay raw value mismatch";
                return result;
            }
        }
    } catch (const std::exception &) {
        result.error = "invalid random replay";
        return result;
    }

    auto state = replayData.initialParticipants;
    auto inventory = replayData.initialInventory;
    std::size_t randomCursor = 0;
    for (const auto &record: replayData.actions) {
        if (record.randomBegin != randomCursor
            || record.randomEnd < record.randomBegin
            || record.randomEnd > replayData.randomCalls.size()
            || record.participants.size() != state.size()
            || !validateInventory(record.inventory)
            || record.integrity == 0
            || record.integrity != actionIntegrity(
                record.action, replayData.enemy, state, inventory,
                record.participants, record.inventory,
                record.randomBegin, record.randomEnd)
            || !validInventoryTransition(record.action, inventory,
                                         record.inventory)
            || !validParticipantTransition(record.action, state,
                                           record.participants)
            || !validateAction(record.action, replayData.enemy, state)) {
            result.error = "invalid action replay record";
            return result;
        }
        randomCursor = record.randomEnd;
        state = record.participants;
        inventory = record.inventory;
    }
    if (replayData.settlementRandomBegin != randomCursor
        || replayData.settlementRandomBegin > replayData.randomCalls.size()) {
        result.error = "invalid settlement random boundary";
        return result;
    }

    if (replayData.finalParticipants.size() != state.size()
        || !validateInventory(replayData.finalInventory)
        || replayData.finalIntegrity == 0
        || replayData.finalIntegrity != snapshotIntegrity(
            replayData.finalParticipants, replayData.enemy,
            replayData.finalInventory, replayData.won,
            replayData.randomCalls, replayData.committed,
            replayData.settlementRandomBegin)) {
        result.error = "replay final snapshot mismatch";
        return result;
    }
    state = replayData.finalParticipants;
    inventory = replayData.finalInventory;

    bool finished = false;
    bool won = false;
    updateFinishedState(replayData.enemy, state, finished, won);
    if (!finished || won != replayData.won) {
        result.error = "replay result mismatch";
        return result;
    }
    result.valid = true;
    result.won = won;
    result.actions = replayData.actions.size();
    result.randomCalls = replayData.randomCalls;
    result.participants = std::move(state);
    result.inventory = std::move(inventory);
    return result;
}

}
