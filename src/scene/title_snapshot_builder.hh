#pragma once

#include "logic/title_snapshot.hh"

#include <cstdint>
#include <string>

namespace hojy::world::state {
struct CharacterData;
}

namespace hojy::scene {

[[nodiscard]] TitleNameEntrySnapshot buildTitleNameEntrySnapshot(
    std::wstring name);

[[nodiscard]] TitlePreviewSnapshot buildTitlePreviewSnapshot(
    const std::wstring &name,
    const ::hojy::world::state::CharacterData &character,
    bool showPotential,
    int windowBorder,
    int confirmationIndex,
    std::uint64_t generation);

}
