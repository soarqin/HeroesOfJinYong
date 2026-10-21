#include "character_list_snapshot_builder.hh"

#include "world/savedata.hh"
#include "world/strings.hh"

#include <fmt/xchar.h>

#include <functional>
#include <iterator>
#include <limits>
#include <optional>
#include <utility>

namespace hojy::scene {
namespace {

using CharacterData = ::hojy::world::state::CharacterData;
using CharacterValue = std::int16_t CharacterData::*;

class ScalarProjection final : public CharacterListProjection {
public:
    ScalarProjection(std::wstring heading, CharacterValue value,
                     std::optional<std::int16_t> minimum = std::nullopt)
        : heading_(std::move(heading)), value_(value), minimum_(minimum) {}

    const std::wstring &heading() const noexcept override { return heading_; }

    bool includes(const CharacterData &character) const noexcept override {
        return !minimum_ || character.*value_ >= *minimum_;
    }

    std::wstring format(const CharacterData &character) const override {
        return fmt::format(L"{:>3}", character.*value_);
    }

private:
    std::wstring heading_;
    CharacterValue value_;
    std::optional<std::int16_t> minimum_;
};

class RangeProjection final : public CharacterListProjection {
public:
    RangeProjection(std::wstring heading, CharacterValue current,
                    CharacterValue maximum)
        : heading_(std::move(heading)), current_(current), maximum_(maximum) {}

    const std::wstring &heading() const noexcept override { return heading_; }

    bool includes(const CharacterData &) const noexcept override { return true; }

    std::wstring format(const CharacterData &character) const override {
        return fmt::format(L"{:>3}/{:>3}",
                           character.*current_, character.*maximum_);
    }

private:
    std::wstring heading_;
    CharacterValue current_;
    CharacterValue maximum_;
};

CharacterListProjectionPtr scalar(
        std::wstring heading, CharacterValue value,
        std::optional<std::int16_t> minimum = std::nullopt) {
    return std::make_shared<ScalarProjection>(
        std::move(heading), value, minimum);
}

CharacterListProjectionPtr range(
        std::wstring heading, CharacterValue current, CharacterValue maximum) {
    return std::make_shared<RangeProjection>(
        std::move(heading), current, maximum);
}

}

CharacterListProjectionPtr levelProjection() {
    return scalar(GETTEXT(24), &CharacterData::level);
}

CharacterListProjectionPtr healthProjection() {
    return range(GETTEXT(1), &CharacterData::hp, &CharacterData::maxHp);
}

CharacterListProjectionPtr maximumHealthProjection() {
    return scalar(GETTEXT(2), &CharacterData::maxHp);
}

CharacterListProjectionPtr magicProjection() {
    return range(GETTEXT(6), &CharacterData::mp, &CharacterData::maxMp);
}

CharacterListProjectionPtr maximumMagicProjection() {
    return scalar(GETTEXT(7), &CharacterData::maxMp);
}

CharacterListProjectionPtr medicProjection(std::int16_t minimum) {
    return scalar(GETTEXT(11), &CharacterData::medic,
                  minimum > std::numeric_limits<std::int16_t>::min()
                      ? std::optional<std::int16_t>(minimum) : std::nullopt);
}

CharacterListProjectionPtr depoisonProjection(std::int16_t minimum) {
    return scalar(GETTEXT(13), &CharacterData::depoison,
                  minimum > std::numeric_limits<std::int16_t>::min()
                      ? std::optional<std::int16_t>(minimum) : std::nullopt);
}

CharacterListProjectionPtr poisonedProjection() {
    return scalar(GETTEXT(3), &CharacterData::poisoned);
}

std::vector<CharacterListSource> teamCharacterSources() {
    std::vector<CharacterListSource> result;
    const auto *base =
        ::hojy::world::state::gSaveData.baseInfo.operator->();
    if (!base) { return result; }
    result.reserve(std::size(base->members));
    for (const auto id: base->members) {
        if (id >= 0) {
            CharacterListSource source;
            source.characterId = id;
            result.push_back(source);
        }
    }
    return result;
}

CharacterListSnapshot buildCharacterListSnapshot(
        std::vector<std::wstring> title,
        const std::vector<CharacterListSource> &characters,
        const std::vector<CharacterListProjectionPtr> &projections) {
    CharacterListSnapshot result;
    result.title = std::move(title);
    for (const auto &projection: projections) {
        if (!projection) { continue; }
        if (!result.columnTitle.empty()) { result.columnTitle += L'|'; }
        result.columnTitle += projection->heading();
    }
    result.rows.reserve(characters.size());
    for (const auto source: characters) {
        if (source.characterId < 0
            || static_cast<std::size_t>(source.characterId)
                >= ::hojy::world::state::gSaveData.charInfo.size()) {
            continue;
        }
        const auto *character =
            ::hojy::world::state::gSaveData.charInfo[source.characterId];
        if (!character) { continue; }
        bool included = true;
        for (const auto &projection: projections) {
            if (projection && !projection->includes(*character)) {
                included = false;
                break;
            }
        }
        if (!included) { continue; }
        CharacterListRowSnapshot row;
        row.characterId = source.characterId;
        row.emphasized = source.emphasized;
        row.name = GETCHARNAME(source.characterId);
        if (source.emphasized) { row.name.insert(row.name.begin(), L'\x0F'); }
        for (const auto &projection: projections) {
            if (!projection) { continue; }
            if (!row.valueText.empty()) { row.valueText += L'|'; }
            row.valueText += projection->format(*character);
        }
        result.rows.push_back(std::move(row));
    }
    return result;
}

}
