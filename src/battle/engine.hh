#pragma once

#include "action.hh"
#include "battle_participant.hh"
#include "random.hh"

#include <cstddef>
#include <string>
#include <vector>

namespace hojy::battle {

enum class EngineStatus {
    Idle,
    Active,
    Finished,
    Faulted,
};

struct BattleSetup {
    std::vector<BattleParticipant *> participants;
    std::vector<bool> enemy;
    RandomSource *random = nullptr;
    InventorySnapshot inventory;
};

struct ActionRecord {
    BattleAction action;
    std::size_t randomBegin = 0;
    std::size_t randomEnd = 0;
    std::vector<::hojy::world::state::CharacterData> participants;
    InventorySnapshot inventory;
    std::uint64_t integrity = 0;
};

struct BattleReplay {
    std::vector<::hojy::world::state::CharacterData> initialParticipants;
    std::vector<bool> enemy;
    InventorySnapshot initialInventory;
    std::vector<ActionRecord> actions;
    std::vector<RandomCall> randomCalls;
    bool won = false;
    std::vector<::hojy::world::state::CharacterData> finalParticipants;
    InventorySnapshot finalInventory;
    std::uint64_t finalIntegrity = 0;
    std::uint64_t initialIntegrity = 0;
    bool committed = false;
    std::size_t settlementRandomBegin = 0;
};

struct BattleSnapshot {
    EngineStatus status = EngineStatus::Idle;
    bool won = false;
    std::size_t actions = 0;
    std::vector<::hojy::world::state::CharacterData> participants;
    InventorySnapshot inventory;
    std::vector<ActionRecord> actionLog;
    std::vector<RandomCall> randomCalls;
};

struct ReplayResult {
    bool valid = false;
    bool won = false;
    std::size_t actions = 0;
    std::vector<::hojy::world::state::CharacterData> participants;
    InventorySnapshot inventory;
    std::vector<RandomCall> randomCalls;
    std::string error;
};

struct BattleResult {
    bool committed = false;
    bool won = false;
    std::size_t actions = 0;
    BattleReplay replay;
};

class BattleEngine final {
public:
    BattleEngine() = default;
    ~BattleEngine();
    BattleEngine(const BattleEngine &) = delete;
    BattleEngine &operator=(const BattleEngine &) = delete;
    BattleEngine(BattleEngine &&) = delete;
    BattleEngine &operator=(BattleEngine &&) = delete;

    bool begin(BattleSetup setup);
    bool record(const BattleAction &action, InventorySnapshot inventory);
    bool reconcile();
    bool reconcile(InventorySnapshot inventory);
    [[nodiscard]] BattleSnapshot snapshot() const;
    BattleResult finish(bool commit);
    void abort() noexcept;

    [[nodiscard]] EngineStatus status() const noexcept { return status_; }
    [[nodiscard]] static ReplayResult replay(const BattleReplay &replay);

private:
    bool validSlot(std::size_t index) const noexcept;
    bool validateAction(
        const BattleAction &action,
        const std::vector<::hojy::world::state::CharacterData> &state) const;
    static bool validateAction(
        const BattleAction &action,
        const std::vector<bool> &enemy,
        const std::vector<::hojy::world::state::CharacterData> &state);
    static bool validateInventory(const InventorySnapshot &inventory);
    static void updateFinishedState(
        const std::vector<bool> &enemy,
        const std::vector<::hojy::world::state::CharacterData> &participants,
        bool &finished, bool &won) noexcept;
    void updateFinishedState() noexcept;
    std::vector<::hojy::world::state::CharacterData> participantState() const;
    const std::vector<RandomCall> *sourceRandomCalls() const noexcept;
    void captureRandomCalls();
    void restoreParticipants(
        const std::vector<::hojy::world::state::CharacterData> &state) noexcept;
    void discardParticipants() noexcept;

    BattleSetup setup_;
    std::vector<::hojy::world::state::CharacterData> initialParticipants_;
    InventorySnapshot initialInventory_;
    InventorySnapshot inventory_;
    std::vector<ActionRecord> actions_;
    std::vector<RandomCall> randomCalls_;
    std::size_t sourceRandomCursor_ = 0;
    EngineStatus status_ = EngineStatus::Idle;
    bool won_ = false;
};

}
