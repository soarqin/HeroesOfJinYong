#pragma once

#include "world/character.hh"

#include <cstddef>
#include <optional>

namespace hojy::battle {

// Transactional adapter around the DOS character wire struct.  Battle code
// mutates the private working copy and explicitly commits only after the
// battle result is accepted.
class BattleParticipant final {
public:
    explicit BattleParticipant(::hojy::world::state::CharacterData &persistent) noexcept;
    BattleParticipant(::hojy::world::state::CharacterData *persistent,
                      const ::hojy::world::state::CharacterData &initial) noexcept;
    BattleParticipant(const BattleParticipant &) = delete;
    BattleParticipant &operator=(const BattleParticipant &) = delete;
    BattleParticipant(BattleParticipant &&) = delete;
    BattleParticipant &operator=(BattleParticipant &&) = delete;

    [[nodiscard]] ::hojy::world::state::CharacterData &state() noexcept { return working_; }
    [[nodiscard]] const ::hojy::world::state::CharacterData &state() const noexcept { return working_; }
    [[nodiscard]] const ::hojy::world::state::CharacterData &baseline() const noexcept { return baseline_; }
    [[nodiscard]] bool changed() const noexcept;
    [[nodiscard]] bool hasStagedCommit() const noexcept { return staged_.has_value(); }

    void discard() noexcept;
    void stageCommit(const ::hojy::world::state::CharacterData &candidate) noexcept;
    void commit() noexcept;

private:
    ::hojy::world::state::CharacterData *persistent_ = nullptr;
    ::hojy::world::state::CharacterData baseline_{};
    ::hojy::world::state::CharacterData working_{};
    std::optional<::hojy::world::state::CharacterData> staged_;
};

}
