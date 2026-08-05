#pragma once

#include "logic/presentation.hh"

#include <cstdint>
#include <memory>
#include <vector>

namespace hojy::world::state {
struct CharacterData;
}

namespace hojy::scene {

struct CharacterListSource final {
    std::int16_t characterId = -1;
    bool emphasized = false;
};

class CharacterListProjection {
public:
    virtual ~CharacterListProjection() = default;
    [[nodiscard]] virtual const std::wstring &heading() const noexcept = 0;
    [[nodiscard]] virtual bool includes(
        const ::hojy::world::state::CharacterData &character) const noexcept = 0;
    [[nodiscard]] virtual std::wstring format(
        const ::hojy::world::state::CharacterData &character) const = 0;
};

using CharacterListProjectionPtr =
    std::shared_ptr<const CharacterListProjection>;

[[nodiscard]] CharacterListProjectionPtr levelProjection();
[[nodiscard]] CharacterListProjectionPtr healthProjection();
[[nodiscard]] CharacterListProjectionPtr maximumHealthProjection();
[[nodiscard]] CharacterListProjectionPtr magicProjection();
[[nodiscard]] CharacterListProjectionPtr maximumMagicProjection();
[[nodiscard]] CharacterListProjectionPtr medicProjection(
    std::int16_t minimum = 0);
[[nodiscard]] CharacterListProjectionPtr depoisonProjection(
    std::int16_t minimum = 0);
[[nodiscard]] CharacterListProjectionPtr poisonedProjection();

[[nodiscard]] std::vector<CharacterListSource> teamCharacterSources();

[[nodiscard]] CharacterListSnapshot buildCharacterListSnapshot(
    std::vector<std::wstring> title,
    const std::vector<CharacterListSource> &characters,
    const std::vector<CharacterListProjectionPtr> &projections);

}
