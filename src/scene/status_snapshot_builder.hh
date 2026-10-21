#pragma once

#include "status_snapshot.hh"

#include "world/character.hh"
#include "world/iteminfo.hh"

#include <cstdint>
#include <optional>

namespace hojy::scene {

/** Build a status snapshot from the current fixed-logic world state. */
[[nodiscard]] std::optional<CharacterStatusSnapshot>
buildCharacterStatusSnapshot(std::int16_t charId, bool simpleMode,
                             bool showPotential);

/** Build a snapshot from a fixed-logic character value and item table. */
[[nodiscard]] std::optional<CharacterStatusSnapshot>
buildCharacterStatusSnapshot(
    const ::hojy::world::state::CharacterData &character,
    const ::hojy::world::state::ItemInfo &items,
    bool simpleMode, bool showPotential);

}
