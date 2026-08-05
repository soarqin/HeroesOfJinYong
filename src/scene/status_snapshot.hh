#pragma once

#include "content/constants.hh"

#include <array>
#include <cstdint>
#include <string>
#include <tuple>

namespace hojy::scene {

/**
 * Immutable value data prepared by fixed logic for StatusView.
 *
 * The view deliberately does not retain a CharacterData pointer or an item
 * table reference.  Names and derived values are copied while the world
 * snapshot is valid, so a later load or scene transition cannot change what a
 * pending render observes.
 */
struct CharacterStatusSnapshot final {
    std::int16_t id = -1;
    std::int16_t headId = -1;
    std::wstring name;

    std::int16_t level = 0;
    std::uint16_t exp = 0;
    std::uint16_t expForLevelUp = 0;
    std::int16_t hp = 0;
    std::int16_t maxHp = 0;
    std::int16_t hurt = 0;
    std::int16_t poisoned = 0;
    std::int16_t stamina = 0;
    std::int16_t mpType = 0;
    std::int16_t mp = 0;
    std::int16_t maxMp = 0;
    std::int16_t attack = 0;
    std::int16_t speed = 0;
    std::int16_t defence = 0;
    std::int16_t medic = 0;
    std::int16_t poison = 0;
    std::int16_t depoison = 0;
    std::int16_t fist = 0;
    std::int16_t sword = 0;
    std::int16_t blade = 0;
    std::int16_t special = 0;
    std::int16_t throwing = 0;
    std::int16_t knowledge = 0;
    std::int16_t integrity = 0;
    std::int16_t reputation = 0;
    std::int16_t potential = 0;

    std::array<std::int16_t, 2> equip{{-1, -1}};
    std::array<std::wstring, 2> equipNames{};
    std::int16_t learningItem = -1;
    std::wstring learningItemName;
    std::uint16_t expForItem = 0;
    std::int16_t learningSkillId = -1;
    std::int16_t learningLevel = 0;
    std::uint16_t expForSkillLearn = 0;

    std::array<std::int16_t, ::hojy::content::LearnSkillCount> skillId{};
    std::array<std::int16_t, ::hojy::content::LearnSkillCount> skillLevel{};
    std::array<std::wstring, ::hojy::content::LearnSkillCount> skillNames{};

    bool simpleMode = false;
    bool showPotential = false;
    std::uint8_t mpColorR = 252;
    std::uint8_t mpColorG = 252;
    std::uint8_t mpColorB = 252;

    [[nodiscard]] const std::wstring &text(std::int16_t id) const noexcept {
        static const std::wstring empty;
        if (id < 0 || static_cast<std::size_t>(id) >= labels.size()) {
            return empty;
        }
        return labels[static_cast<std::size_t>(id)];
    }

    std::array<std::wstring, 138> labels{};
};

}
