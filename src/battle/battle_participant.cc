#include "battle_participant.hh"

#include <cstring>
#include <type_traits>

namespace hojy::battle {

BattleParticipant::BattleParticipant(::hojy::world::state::CharacterData &persistent) noexcept:
    BattleParticipant(&persistent, persistent) {
}

BattleParticipant::BattleParticipant(
    ::hojy::world::state::CharacterData *persistent,
    const ::hojy::world::state::CharacterData &initial) noexcept:
    persistent_(persistent), baseline_(initial), working_(initial) {
    static_assert(std::is_trivially_copyable<::hojy::world::state::CharacterData>::value,
                  "CharacterData must remain a wire-compatible value type");
}

bool BattleParticipant::changed() const noexcept {
    return std::memcmp(&working_, &baseline_, sizeof(working_)) != 0;
}

void BattleParticipant::discard() noexcept {
    working_ = baseline_;
    staged_.reset();
}

void BattleParticipant::stageCommit(
    const ::hojy::world::state::CharacterData &candidate) noexcept {
    staged_ = candidate;
}

void BattleParticipant::commit() noexcept {
    const auto &candidate = staged_.has_value() ? *staged_ : working_;
    if (persistent_) {
        *persistent_ = candidate;
    }
    baseline_ = candidate;
    working_ = candidate;
    staged_.reset();
}

}
